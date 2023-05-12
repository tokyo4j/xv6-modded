KERN_SRCS := $(wildcard kernel/*.c kernel/*.S)
ifeq ($(filter kernel/vectors.S, $(KERN_SRCS)),)
	KERN_SRCS += kernel/vectors.S
endif

KERN_OBJS := $(patsubst %.c, %.o,\
							$(patsubst %.S, %.o,\
					      $(KERN_SRCS)))

KERN_OBJS_IDEFS := $(KERN_OBJS) kernel/ide/ide.o
KERN_OBJS_MEMFS := $(KERN_OBJS) kernel/ide/memide.o

kernel.elf: $(KERN_OBJS_IDEFS) kernel/bin/initcode kernel/bin/entryother kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(KERN_OBJS_IDEFS) \
		-b binary kernel/bin/initcode kernel/bin/entryother kernel/bin/zap-light16.psf

kernelm.elf: $(KERN_OBJS_MEMFS) kernel/bin/initcode kernel/bin/entryother kernel/kernel.ld fs.img
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(KERN_OBJS_MEMFS) \
		-b binary kernel/bin/initcode kernel/bin/entryother kernel/bin/zap-light16.psf fs.img

kernel/bin/initcode: kernel/bin/initcode.out
	$(OBJCOPY) -j .text -S -O binary $< $@
kernel/bin/initcode.out: kernel/bin/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $@ $<

kernel/bin/entryother: kernel/bin/entryother.out
	$(OBJCOPY) -j .text -S -O binary -j .text $< $@
kernel/bin/entryother.out: kernel/bin/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $@ $<

kernel/vectors.S: kernel/vectors.pl
	./$< > $@