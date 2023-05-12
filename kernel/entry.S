# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.

#include <xv6/memlayout.h>
#include <xv6/param.h>
#include <xv6/seg.h>
#include <xv6/x86.h>
#include <xv6/mb2.h>

# Entering xv6 on boot processor, with paging off.

# _start is the virtual address of the ELF entry point.
# When booted from xv6's bootloader (not 3rd party bootloaders like GRUB), eip actually points to _start - KERNBASE.
.code32
.globl _start
_start:
  cmpl    $(MB2_MAGIC), %eax
  jne 1f
# defined in multiboot2.c
.extern mb2_magic
.extern mb2_info
  movl    %eax, (V2P_WO(mb2_magic))
  movl    %ebx, (V2P_WO(mb2_info_ptr))
1:

  # defined in main.c
  .extern entry_pml4
  .extern entry_pml3 # a.k.a. page directory pointer table

  # early mappings:
  #                 Virtual                     ->                      Physical
  # 0x0000_0000_0000_0000-0x0000_0080_0000_0000 -> 0x0000_0000_0000_0000-0x0000_0080_0000_0000 [512G]
  # 0xffff_8000_0000_0000-0xffff_8080_0000_0000 -> 0x0000_0000_0000_0000-0x0000_0080_0000_0000 [512G]

  # setup PML4
  movl    $(V2P_WO(entry_pml3)), %eax
  orl     $3, %eax
  movl    %eax, (V2P_WO(entry_pml4))
  movl    %eax, (V2P_WO(entry_pml4) + 2048)

  # setup PML3 (PS enabled)
  # %eax stores lower 32 bit of PML3 entry and %ebx stores higher 32 bit
  # %edx stores the address of PML3 entry
  # 0x83 in entries means page-sized, writable, present
  xorl    %eax, %eax
  xorl    %ebx, %ebx
  orl     $0x83, %eax
  movl    $(V2P_WO(entry_pml3)), %edx
1:
  movl    %eax, (%edx)
  movl    %ebx, 4(%edx)
  addl    $(1 * 1024 * 1024 * 1024), %eax
  addl    $8, %edx
  cmpl    $0x83, %eax
  jne     2f
  addl    $1, %ebx
2:
  cmpl    $(V2P_WO(entry_pml3) + 4096), %edx
  jl      1b

  # set cr3
  movl    $(V2P_WO(entry_pml4)), %eax
  movl    %eax, %cr3

  # enable PAE
  movl    %cr4, %eax
  or      $(CR4_PAE), %eax
  movl    %eax, %cr4

  # enter ia32e mode (IA32_EFER.LME = IA32_EFER.NXE(No-Execute Enable) = 1)
  movl    $(IA32_EFER), %ecx
  rdmsr
  orl     $(EFER_LME | EFER_NXE) , %eax
  wrmsr

  # Turn on paging (w/ write protect)
  movl    %cr0, %eax
  orl     $(CR0_PG | CR0_WP), %eax
  movl    %eax, %cr0

  # reload segments (long mode bit in code segment descriptor is set)
  lgdt    (V2P_WO(entry_gdtdesc))
  movw    $(SEG_KDATA<<3), %ax
  movw    %ax, %ds
  movw    %ax, %ss
  movw    %ax, %es
  movw    %ax, %fs
  movw    %ax, %gs
  ljmp    $(SEG_KCODE<<3), $(V2P_WO(long_mode_entry))

.code64
long_mode_entry:
  # Set up the stack pointer.
  movabsq $(stack + KSTACKSIZE), %rsp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  movabsq $main, %rax
  jmpq *%rax

entry_gdt:
  SEGDESC_NULL
  SEGDESC64_CODE(0)
  SEGDESC32_DATA(0)
entry_gdtdesc:
  GDTDESC64(entry_gdtdesc-entry_gdt, V2P_WO(entry_gdt))

.comm stack, KSTACKSIZE
