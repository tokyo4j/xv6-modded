#include <xv6/seg.h>

  # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # saved registers
  pushq %rbx
  pushq %rbp
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  # syscall number
  pushq %rax
  # parameter registers
  pushq %rdi
  pushq %rsi
  pushq %rdx
  pushq %rcx
  pushq %r8
  pushq %r9
  # segment registers
  xorq  %rax, %rax
  movw  %ds, %ax
  pushq %rax
  movw  %es, %ax
  pushq %rax
  movw  %fs, %ax
  pushq %rax
  movw  %gs, %ax
  pushq %rax

  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es
  # we have to update ss manually in x64
  movw %ax, %ss

  # Call trap(tf), where tf=%esp
  movq %rsp, %rdi
  call trap

  # Return falls through to trapret...
.globl trapret
trapret:
  popq %rax
  movw %ax, %gs
  popq %rax
  movw %ax, %fs
  popq %rax
  movw %ax, %es
  popq %rax
  movw %ax, %ds

  popq %r9
  popq %r8
  popq %rcx
  popq %rdx
  popq %rsi
  popq %rdi

  popq %rax

  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbp
  popq %rbx

  addq $16, %rsp  # trapno and errcode
  iretq
