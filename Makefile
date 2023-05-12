SHELL = sh
USE_CLANG = y

ifeq ($(USE_CLANG), y)
CC = clang -target x86_64-pc-linux-gnu
LD = ld.lld
OBJCOPY = llvm-objcopy
OBJDUMP = llvm-objdump
else
CC = gcc
LD = ld
OBJCOPY = objcopy
OBJDUMP = objdump
endif

CFLAGS = 	-ggdb \
					-mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
					-mcmodel=large \
					-ffreestanding -fno-stack-protector -fno-pic \
					-Iinclude \
					-Wall
ASFLAGS = -ggdb -mcmodel=large -Iinclude

NCPU = 4
QEMU = qemu-system-x86_64
QEMUOPTS = -serial mon:stdio -smp $(NCPU),sockets=$(NCPU),cores=1,threads=1 -m 512

default: xv6m.iso

#defined in user/user.mk
UPROGS =

# build kernel.elf
include kernel/kernel.mk
# build $(UPROGS)
include user/user.mk

xv6.img: bootblock kernel.elf
	dd if=/dev/zero of=$@ count=10000
	dd if=bootblock of=$@ conv=notrunc
	dd if=kernel.elf of=$@ seek=1 conv=notrunc

xv6m.img: bootblock kernelm.elf
	dd if=/dev/zero of=$@ count=10000
	dd if=bootblock of=$@ conv=notrunc
	dd if=kernelm.elf of=$@ seek=1 conv=notrunc

mkfs: mkfs.c include/xv6/fs.h
	$(if $(USE_CLANG), clang, gcc) -DMKFS -Werror -Wall -Iinclude -o $@ mkfs.c

fs.img: mkfs README $(UPROGS)
	mkdir fs
	cp README $(UPROGS) fs
	cd fs && ../mkfs ../fs.img README $(patsubst user/%, %, $(UPROGS))
	rm -r fs

xv6.iso: kernel.elf grub.cfg
	mkdir -p isodir/boot/grub
	cp $< isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $@ isodir
	rm -r isodir

xv6m.iso: kernelm.elf grub.cfg
	mkdir -p isodir/boot/grub
	cp $< isodir/boot/kernel.elf
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $@ isodir
	rm -r isodir

clean:
	find \
		-iname '*.d' -o \
		-iname '*.img' -o \
		-iname '*.out' -o \
		-iname '*.iso' -o \
		-iname '*.elf' -o \
		-iname '*.o' \
		| xargs rm -f
	rm -f kernel/entryother kernel/initcode kernel/vectors.S bootblock mkfs user/_*

format:
	find . -iname "*.c" -o -iname ".h" | xargs clang-format -i

# qemu ("-accel kvm" does not work for IDE)
q: xv6.iso fs.img
	$(QEMU) $(QEMUOPTS) \
	-drive file=fs.img,index=1,media=disk,format=raw \
	-cdrom $<

# qemu debug
qd: xv6.iso fs.img
	$(QEMU) $(QEMUOPTS) \
	-drive file=fs.img,index=1,media=disk,format=raw \
	-cdrom $< \
	-S -s

# qemu uefi
qe: xv6.iso fs.img
	$(QEMU) $(QEMUOPTS) \
	-drive file=fs.img,index=1,media=disk,format=raw \
	-bios /usr/share/OVMF/x64/OVMF.fd \
	-cdrom $<

# qemu memfs
qm: xv6m.iso
	$(QEMU) $(QEMUOPTS) \
		-accel kvm \
		-cdrom $<

# qemu memfs debug
qmd: xv6m.iso
	$(QEMU) $(QEMUOPTS) \
		-cdrom $< \
		-S -s

# qemu memfs uefi
qme: xv6m.iso
	$(QEMU) $(QEMUOPTS) \
		-bios /usr/share/OVMF/x64/OVMF.fd \
		-accel kvm \
		-cdrom $<

# gdb debugging for non-memfs kernel
gdb:
	gdb -ex "target remote :1234" -ex "symbol-file kernelm.elf"

# gdb debugging for memfs kernel
gdbm:
	gdb -ex "target remote :1234" -ex "symbol-file kernelm.elf"

# Used for try-and-error.
tmp: xv6m.iso
	qemu-system-x86_64 -smp 4,sockets=4,cores=1,threads=1 -m 256M \
		-monitor stdio \
    -cdrom xv6m.iso -s -S \
		-icount shift=auto \
		-d int \
		| tee log.txt
	rm log.txt
