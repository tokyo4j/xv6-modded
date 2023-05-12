// Memory layout

#pragma once

// Key addresses for address space layout

// First kernel virtual address
#define KERNBASE   0xffff800000000000UL
#define KERNOFFSET 0x100000
// Address where kernel is linked
#define KERNLINK (KERNBASE + KERNOFFSET)

#ifndef __ASSEMBLER__
#define V2P(a) ((typeof(a))(((ulong)(a)) - KERNBASE))
#define P2V(a) ((typeof(a))(((ulong)(a)) + KERNBASE))
#else
#define V2P_WO(x) ((x)-KERNBASE)   // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE) // same as P2V, but without casts
#endif