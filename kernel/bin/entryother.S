#include <xv6/seg.h>
#include <xv6/x86.h>

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must sit
# at an address in the low 2^16 bytes.
#
# Startothers (in main.c) sends the STARTUPs one at a time.
# It copies this code (start) at 0x7000.  It puts the address of
# a newly allocated per-core stack in start-4,the address of the
# place to jump to (mpenter) in start-8, and the physical address
# of entrypgdir in start-12.
#
# This code combines elements of bootasm.S and entry.S.

.code16
.globl start
start:
  cli
  xorw    %ax,%ax
  movw    %ax,%ds

  // enter protected mode
  lgdt    gdtdesc32
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

  # Set up the protected-mode data segment registers
  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  ljmpl    $(SEG_KCODE<<3), $(start32)

.code32
start32:
  # Use entry_pml4 as our initial page table
  movl    (start-24), %eax
  movl    %eax, %cr3

  # Enable PAE
  movl    %cr4, %eax
  or      $(CR4_PAE), %eax
  movl    %eax, %cr4

  # enter ia32e mode
  movl    $(IA32_EFER), %ecx
  rdmsr
  orl     $(EFER_LME | EFER_NXE), %eax
  wrmsr

  # Turn on paging (w/ write protect) and enter long mode
  movl    %cr0, %eax
  orl     $(CR0_PG | CR0_WP), %eax
  movl    %eax, %cr0

  # reload segments (long mode bit in code segment descriptor is set)
  lgdt    gdtdesc64
  movw    $(SEG_KDATA<<3), %ax
  movw    %ax, %ds
  movw    %ax, %ss
  movw    %ax, %es
  movw    %ax, %fs
  movw    %ax, %gs
  ljmp    $(SEG_KCODE<<3), $(start64)

.code64
start64:
  # Switch to the stack allocated by startothers()
  movq    (start-8), %rsp
  # Call mpenter()
  movq    (start-16), %rax
  call	 *%rax

1:
  jmp     1b

.p2align 2
// TODO: 32-bit GDT is unnecessary
gdt32:
  SEGDESC_NULL
  SEGDESC32_CODE(0)
  SEGDESC32_DATA(0)
gdtdesc32:
  GDTDESC32(gdtdesc32-gdt32, gdt32)
gdt64:
  SEGDESC_NULL
  SEGDESC64_CODE(0)
  SEGDESC32_DATA(0)
gdtdesc64:
  GDTDESC64(gdtdesc64-gdt64, gdt64)