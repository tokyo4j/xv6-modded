SHELL = bash
TOOLPREFIX = i386-elf-

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
CFLAGS = -fno-pic -fno-pie -no-pie\
					-static -O2 -ggdb -m32 -nostdinc\
					-fno-builtin -fno-strict-aliasing -fno-omit-frame-pointer -fno-stack-protector\
					-Iinclude\
					-Wall
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide -Iinclude
LDFLAGS += -m elf_i386

QEMU = qemu-system-i386
QEMUOPTS = -serial mon:stdio -smp 2,sockets=2,cores=1,threads=1 -m 512

default: xv6.img

#defined in user/user.mk
UPROGS =

# build bootblock
include boot/boot.mk
# build kernel.elf
include kernel/kernel.mk
# build $(UPROGS)
include user/user.mk

xv6.img: bootblock kernel.elf
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=kernel.elf of=xv6.img seek=1 conv=notrunc

xv6memfs.img: bootblock kernelmemfs
	dd if=/dev/zero of=xv6memfs.img count=10000
	dd if=bootblock of=xv6memfs.img conv=notrunc
	dd if=kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

mkfs: mkfs.c include/fs.h
	gcc -Werror -Wall -o $@ mkfs.c

fs.img: mkfs README $(UPROGS)
	mkdir fs
	cp README $(UPROGS) fs
	cd fs && ../mkfs ../fs.img README $(patsubst user/%, %, $(UPROGS))
	rm -r fs

clean:
	find \
		-name '*.d' -o \
		-name '*.img' -o \
		-name '*.out' -o \
		-name '*.o' \
		| xargs rm -f
	rm -f kernel/entryother kernel/initcode kernel/vectors.S kernel.elf bootblock mkfs kernelmemfs user/_*

qemu: fs.img xv6.img
	$(QEMU) $(QEMUOPTS)\
		-drive file=fs.img,index=1,media=disk,format=raw\
		-drive file=xv6.img,index=0,media=disk,format=raw

qemu-memfs: xv6memfs.img
	$(QEMU) $(QEMUOPTS)\
		-drive file=xv6memfs.img,index=0,media=disk,format=raw

qemu-nox: fs.img xv6.img
	$(QEMU) $(QEMUOPTS)\
		-drive file=fs.img,index=1,media=disk,format=raw\
		-drive file=xv6.img,index=0,media=disk,format=raw\
		-nographic

qemu-gdb: fs.img xv6.img
	$(QEMU) $(QEMUOPTS)\
		-drive file=fs.img,index=1,media=disk,format=raw\
		-drive file=xv6.img,index=0,media=disk,format=raw\
		-S -s

qemu-nox-gdb: fs.img xv6.img
	$(QEMU) $(QEMUOPTS)\
		-drive file=fs.img,index=1,media=disk,format=raw\
		-drive file=xv6.img,index=0,media=disk,format=raw\
		-nographic\
		-S -s
