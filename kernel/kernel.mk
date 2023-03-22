KERN_SRCS := $(wildcard kernel/*.c kernel/*.S)
ifeq ($(filter kernel/vectors.S, $(KERN_SRCS)),)
	KERN_SRCS += kernel/vectors.S
endif

KERN_OBJS := $(patsubst %.c, %.o,\
							$(patsubst %.S, %.o,\
					      $(KERN_SRCS)))

# make sure kernel/entry.o comes first
KERN_OBJS := kernel/entry.o $(filter-out kernel/entry.o, $(KERN_OBJS))

KERN_OBJS_IDEFS := $(KERN_OBJS) kernel/ide/ide.o
KERN_OBJS_MEMFS := $(KERN_OBJS) kernel/ide/memide.o

kernel.elf: $(KERN_OBJS_IDEFS) kernel/entryother kernel/initcode kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(KERN_OBJS_IDEFS) -b binary kernel/initcode kernel/entryother

kernelmemfs.elf: $(KERN_OBJS_MEMFS) kernel/entryother kernel/initcode kernel/kernel.ld fs.img
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $@ $(KERN_OBJS_MEMFS) -b binary kernel/initcode kernel/entryother fs.img

kernel/initcode: kernel/bin/initcode.out
	$(OBJCOPY) -S -O binary $< $@

kernel/bin/initcode.out: kernel/bin/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $@ $<

kernel/entryother: kernel/bin/entryother.out
	$(OBJCOPY) -S -O binary -j .text $< $@

kernel/bin/entryother.out: kernel/bin/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o $@ $<

kernel/vectors.S: kernel/vectors.pl
	./$< > $@