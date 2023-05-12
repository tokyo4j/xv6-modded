#pragma once

// clang-format off
/*
[Layout of Segment Descriptor]

  G: Granularity flag. If set, limit scales by 4KB.
  DB: Size flag. If set, represents 32 bit mode segment. For long-mode code segment, this should be cleared.
  L: Long-mode code flag.
        +----+----+----+----+
        | G  | DB | L  | -  |  P: Present
        +----+----+----+----+  DPL: Privilege level
                    ^          S: Descriptor type: If set, represents code/data segment, system segment otherwise.
                    |          [code/data segment (S=1)]
                    |          E: Executable. If set, represents code segment, data segment otherwise.
                    |          DC: (for data) Direction. If set, segment grows down.
                    |              (for code) Comforming. If set, segment can be executed from lower/equal ring.
                    |          RW: (for code) Readable. Note that write is always prohibited.
                    |              (for data) Writable. Note that read is always allowed.
                    |          A: Access. Set by CPU when accessed.
                    |          [system segment (S=0 and the CPU is in long mode)]
                    |          Type: 0x9->TSS(available)
                    |                            +---+--------+---+---+----+----+---+
                    +-------+                    | P |  DPL   | S | E | DC | RW | A |
                            |                    +---+--------+---+---+----+----+---+
                            |                                   ^ \----Type(S=0)----/
                            |                                   |
  +----------------------+------------+--------------+----------------------+----------------------+
  |      base[31:24]     | flags[3:0] | limit[19:16] |     access[7:0]      |     base[23:16]      |
  +----------------------+------------+--------------+----------------------+----------------------+
  |                 base[15:0]                       |                   limit[15:0]               |
  +--------------------------------------------------+---------------------------------------------+
                                                        \--> The actual size of the segment is (limit+1) << (G ? 12 : 0)
*/

#ifndef __ASSEMBLER__
#include <xv6/types.h>
#include <xv6/x86.h>
// Segment Descriptor
struct segdesc {
  union {
    struct {
      ushort lim_15_0;
      ushort base_15_0;
      uchar base_23_16;
      uchar access;
      uchar flags_and_lim_19_16;
      uchar base_31_24;
    };

    // for long mode system segment descriptor
    struct {
      uint base_63_32;
      uint reserved;
    };
  };
};

#define SEGDESC(_base, _limit, _access, _flags) \
  (struct segdesc) { \
    .lim_15_0 = (ulong)(_limit)&0xffff, \
    .base_15_0 = (ulong)(_base)&0xffff, \
    .base_23_16 = ((ulong)(_base) >> 16) & 0xff, \
    .access = (_access), \
    .flags_and_lim_19_16 = ((_flags) << 4 ) | (((ulong)(_limit) >> 16) & 0xf), \
    .base_31_24 = ((ulong)(_base) >> 24) & 0xff \
  }

#define SEGDESC_NULL \
  (struct segdesc) { 0 }

#define TSSDESC_LO(_base) \
  SEGDESC(_base, sizeof(struct taskstate)-1, \
    /* [7]present, [6:5]DPL=0, [4]system segment, [3:0]64-bit TSS(Available) */ \
    0x89, \
    /* flags cleared */ \
    0x0 \
   )
#define TSSDESC_HI(_base) \
  (struct segdesc) { .base_63_32 = (ulong)(_base) >> 32 }

#else

#define SEGDESC(_base, _limit, _access, _flags) \
  .word (_limit) & 0xffff; \
  .word (_base) & 0xffff; \
  .byte ((_base) >> 16) & 0xff; \
  .byte (_access); \
  .byte ((_flags) << 4) | (((_limit) >> 16) & 0xf); \
  .byte ((_base) >> 24) & 0xff;
#define SEGDESC_NULL \
  .quad 0x0000000000000000;
#define GDTDESC32(_size, _offset) \
  .word (_size-1); \
  .long (_offset);
#define GDTDESC64(_size, _offset) \
  .word (_size-1); \
  .quad (_offset);

#endif

#define SEGDESC_FLAT(_access, _flags) \
  SEGDESC(0x00000000, 0x000fffff, _access, _flags)

#define SEGDESC32_CODE(_dpl) \
  SEGDESC_FLAT( \
    /* [7]present, [6:5]DPL=_dpl, [4]code/data segment \
       [3]code segment, [2]not conforming, [1]readable */ \
    0b10011010 | ((_dpl) << 5), \
    /* [3] limit scale=4k, [2] 32-bit segment, [1] not long mode */ \
    0b1100 \
  )
#define SEGDESC32_DATA(_dpl) \
  SEGDESC_FLAT( \
    /* [7]present, [6:5]DPL=_dpl, [4]code/data segment \
       [3]data segment, [2]grows up, [1]readable */ \
    0b10010010 | ((_dpl) << 5), \
    /* [3] limit scale=4k, [2] 32-bit segment, [1] not long mode */ \
    0b1100 \
  )
#define SEGDESC64_CODE(_dpl) \
  SEGDESC_FLAT( \
    /* [7]present, [6:5]DPL=_dpl, [4]code/data segment \
       [3]code segment, [2]not conforming, [1]readable */ \
    0b10011010 | ((_dpl) << 5), \
    /* [3] limit scale=4k, [2] (cleared), [1]long mode */ \
    0b1010 \
  )

#define NSEGS 7
#define SEG_KCODE  1 // kernel code
#define SEG_KDATA  2 // kernel data+stack
#define SEG_UCODE  3 // user code
#define SEG_UDATA  4 // user data+stack
#define SEG_TSS    5 // this process's task state (lower 8 bytes)
#define SEG_TSS_HI 6 // this process's task state (higher 8 bytes)

#ifndef __ASSEMBLER__

// Task state segment format
struct taskstate {
  uint reserved1;
  ulong rsp0;
  ulong rsp1;
  ulong rsp2;
  ulong reserved2;
  ulong ist1;
  ulong ist2;
  ulong ist3;
  ulong ist4;
  ulong ist5;
  ulong ist6;
  ulong ist7;
  ulong reserved3;
  uint reserved4;
  uint iopb;
} __attribute__((packed));

#endif