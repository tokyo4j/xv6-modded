SHELL = bash
USE_CLANG = y

ifeq ($(USE_CLANG), y)
CC = clang -target i386-pc-linux-gnu
LD = ld.lld
OBJCOPY = llvm-objcopy
OBJDUMP = llvm-objdump
else
TOOLPREFIX = i386-elf-
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
endif

CFLAGS = -static -ggdb -nostdinc -m32 -mno-sse\
					-fno-builtin -fno-strict-aliasing -fno-omit-frame-pointer -fno-stack-protector -fno-pic -fno-pie\
					-Iinclude\
					-Wall
ASFLAGS = -m32 -gdwarf-2 -Iinclude
LDFLAGS += -m elf_i386

NCPU = 4
QEMU = qemu-system-i386
QEMUOPTS = -serial mon:stdio -smp $(NCPU),sockets=$(NCPU),cores=1,threads=1 -m 512

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
	dd if=/dev/zero of=$@ count=10000
	dd if=bootblock of=$@ conv=notrunc
	dd if=kernel.elf of=$@ seek=1 conv=notrunc

xv6memfs.img: bootblock kernelmemfs.elf
	dd if=/dev/zero of=$@ count=10000
	dd if=bootblock of=$@ conv=notrunc
	dd if=kernelmemfs.elf of=$@ seek=1 conv=notrunc

mkfs: mkfs.c include/xv6/fs.h
	gcc -Werror -Wall -Iinclude -o $@ mkfs.c

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
	rm -f kernel/entryother kernel/initcode kernel/vectors.S kernel.elf bootblock mkfs kernelmemfs.elf user/_*

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
