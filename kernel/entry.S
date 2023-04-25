# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.

# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs.elf to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include <xv6/asm.h>
#include <xv6/memlayout.h>
#include <xv6/mmu.h>
#include <xv6/param.h>

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)


# Entering xv6 on boot processor, with paging off.

# _start is the virtual address of the ELF entry point.
# When booted from xv6's bootloader (not 3rd party bootloaders like GRUB), eip actually points to _start - KERNBASE.
.globl _start
_start:
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4

  # Set page directory
  # We will map virtual 0~4GB to physical 0~4GB for MADT discovery, except for virtual 4GB~4GB+4MB where the kernel and early heap resides.

  # 0~4GB (V) -> 0~4GB (P)
  movl    $(V2P_WO(entrypgdir)), %edx
  movl    $0, %ecx
1:
  movl    $(PTE_P | PTE_W | PTE_PS), %eax
  orl     %ecx, %eax
  movl    %eax, (%edx)

  addl    $(4 * 1024 * 1024), %ecx
  addl    $4, %edx
  cmp     $(V2P_WO(entrypgdir) + 4096), %edx
  jne     1b

  # 2GB~2GB+4MB (V) -> 0~4MB (P)
  movl    $(V2P_WO(entrypgdir) + 2048), %edx
  movl    $(PTE_P | PTE_W | PTE_PS), (%edx)

  # finally set cr3
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3

  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.
  movl $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  mov $main, %eax
  jmp *%eax

.comm stack, KSTACKSIZE
