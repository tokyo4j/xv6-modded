#pragma once

#include <stdarg.h>
#include <xv6/mmu.h>
#include <xv6/types.h>

// (.text) etext (.rodata) data (.data) edata (.bss) end
// Page aligned
extern char etext[];
extern char data[];
extern char edata[];
extern char end[];

extern char _binary_kernel_bin_initcode_start[],
    _binary_kernel_bin_initcode_size[];
extern char _binary_kernel_bin_entryother_start[],
    _binary_kernel_bin_entryother_size[];
extern char _binary_fs_img_start[], _binary_fs_img_size[];
extern char _binary_kernel_bin_zap_light16_psf_start[],
    _binary_kernel_bin_zap_light16_psf_size[];

// number of elements in fixed-size array
#define NELEM(x)         (sizeof(x) / sizeof((x)[0]))
#define ALIGN(x, size)   ((x) & ~((size)-1))
#define ALIGNUP(x, size) (((x) + (size)-1) & ~((size)-1))
#define MIN(x, y)        ((x) < (y) ? (x) : (y))
#define MAX(x, y)        ((x) > (y) ? (x) : (y))