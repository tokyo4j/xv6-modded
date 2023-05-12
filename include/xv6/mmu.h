// This file contains definitions for the
// x86 memory management unit (MMU).
#pragma once

#ifndef __ASSEMBLER__
#include <xv6/types.h>
// virtual address to index within PML*
#define PML1X(va) (((ulong)(va) >> PML1XSHIFT) & 0x1ff)
#define PML2X(va) (((ulong)(va) >> PML2XSHIFT) & 0x1ff)
#define PML3X(va) (((ulong)(va) >> PML3XSHIFT) & 0x1ff)
#define PML4X(va) (((ulong)(va) >> PML4XSHIFT) & 0x1ff)
// PML5 actually doesn't exist
#define PML5X(va) (((ulong)(va) >> PML5XSHIFT) & 0x1ff)
#endif

// Page directory and page table constants.
#define NR_PTE 512  // # PTEs per page table
#define PGSIZE 4096 // bytes mapped by a page

#define PML1XSHIFT 12
#define PML2XSHIFT 21
#define PML3XSHIFT 30
#define PML4XSHIFT 39
#define PML5XSHIFT 48

// virtual frame number shifted by PML1XSHIFT
#define VPN(x) ((x) & (((1UL << PML5XSHIFT) - 1) | (PGSIZE - 1)))

#define PGROUNDUP(sz)  (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) ((a) & ~(PGSIZE - 1))
#define PGALIGNED(a)   (!((a) & (PGSIZE - 1)))

// Page table/directory entry flags.
#define PTE_P  0x001UL // Present
#define PTE_W  0x002UL // Writeable
#define PTE_U  0x004UL // User
#define PTE_PS 0x080UL // Page Size
#define PTE_XD (1UL << 63)

#ifndef __ASSEMBLER__
#include <xv6/types.h>
// Address in page table or page directory entry
#define PTE_ADDR(pte)  ((ulong)(pte)&0x0000FFFFFFFFF000UL)
#define PTE_FLAGS(pte) ((ulong)(pte) & (PTE_XD | 0xFFFUL))

typedef ulong pte_t;
#endif
