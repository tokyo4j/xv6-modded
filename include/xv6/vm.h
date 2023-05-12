#pragma once

#include <xv6/fs.h>
#include <xv6/mmu.h>
#include <xv6/proc.h>
#include <xv6/types.h>

// In exec.c
int exec(char *, char **);

// In vm.c
void seginit(void);
void kvmalloc(void);
pte_t *setupkvm(void);
ulong uva2ka(pte_t *pml4, ulong uva);
int allocuvm(pte_t *pml4, ulong oldsz, ulong newsz, ulong perm);
int deallocuvm(pte_t *pml4, ulong oldsz, ulong newsz);
void freevm(pte_t *pml4);
void inituvm(pte_t *pml4, char *init, ulong sz);
int loaduvm(pte_t *pml4, ulong addr, struct inode *ip, uint offset, uint sz);
pte_t *copyuvm(pte_t *pml4, ulong sz);
void switchuvm(struct proc *p);
void switchkvm(void);
int copyout(pte_t *pml4, ulong va, ulong p, ulong len);
void clearpteu(pte_t *pml4, ulong uva);