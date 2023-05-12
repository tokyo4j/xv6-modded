// Routines to let C code use special x86 instructions.

#pragma once

#ifndef __ASSEMBLER__

#include <xv6/types.h>

static inline uchar inb(ushort port) {
  uchar data;

  asm volatile("inb %1,%0" : "=a"(data) : "d"(port));
  return data;
}

static inline void insl(ushort port, void *addr, int cnt) {
  asm volatile("cld; rep insl"
               : "=D"(addr), "=c"(cnt)
               : "d"(port), "0"(addr), "1"(cnt)
               : "memory", "cc");
}

static inline void outb(ushort port, uchar data) {
  asm volatile("outb %0,%1" : : "a"(data), "d"(port));
}

static inline void outw(ushort port, ushort data) {
  asm volatile("outw %0,%1" : : "a"(data), "d"(port));
}

static inline void outsl(ushort port, const void *addr, int cnt) {
  asm volatile("cld; rep outsl"
               : "=S"(addr), "=c"(cnt)
               : "d"(port), "0"(addr), "1"(cnt)
               : "cc");
}

static inline void stosb(void *addr, int data, int cnt) {
  asm volatile("cld; rep stosb"
               : "=D"(addr), "=c"(cnt)
               : "0"(addr), "1"(cnt), "a"(data)
               : "memory", "cc");
}

static inline void stosl(void *addr, int data, int cnt) {
  asm volatile("cld; rep stosl"
               : "=D"(addr), "=c"(cnt)
               : "0"(addr), "1"(cnt), "a"(data)
               : "memory", "cc");
}

struct segdesc;

static inline void lgdt(struct segdesc *p, ushort size) {
  volatile struct {
    ushort size;
    ulong ptr;
  } __attribute__((packed)) pd = {.size = size - 1, .ptr = (ulong)p};

  asm volatile("lgdt %0" : : "m"(pd));
}

struct gatedesc;

static inline void lidt(struct gatedesc *p, int size) {
  volatile struct {
    ushort size;
    ulong ptr;
  } __attribute__((packed)) pd = {.size = size - 1, .ptr = (ulong)p};

  asm volatile("lidt %0" : : "m"(pd));
}

static inline void ltr(ushort sel) { asm volatile("ltr %0" : : "r"(sel)); }

static inline ulong readrflags(void) {
  ulong rflags;
  asm volatile("pushfq; popq %0" : "=r"(rflags));
  return rflags;
}

static inline void loadgs(ushort v) {
  asm volatile("movw %0, %%gs" : : "r"(v));
}

static inline void cli(void) { asm volatile("cli"); }

static inline void sti(void) { asm volatile("sti"); }

static inline uint xchg(volatile uint *addr, uint newval) {
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1"
               : "+m"(*addr), "=a"(result)
               : "1"(newval)
               : "cc");
  return result;
}

static inline ulong rcr2(void) {
  ulong val;
  asm volatile("movq %%cr2,%0" : "=r"(val));
  return val;
}

static inline void lcr3(ulong val) {
  asm volatile("movq %0,%%cr3" : : "r"(val));
}

#endif

#define DPL_USER 3

// Eflags register
#define FL_IF 0x00000200 // Interrupt Enable

// Control Register flags
#define CR0_PE 0x00000001 // Protection Enable
#define CR0_WP 0x00010000 // Write Protect
#define CR0_PG 0x80000000 // Paging

#define CR4_PSE 0x00000010 // Page Size Extension
#define CR4_PAE 0x00000020 // Page Address Extension

#define IA32_EFER 0xC0000080
#define EFER_LME  0x100
#define EFER_NXE  0x800
