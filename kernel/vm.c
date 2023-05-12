#include <xv6/console.h>
#include <xv6/fs.h>
#include <xv6/kalloc.h>
#include <xv6/memlayout.h>
#include <xv6/misc.h>
#include <xv6/mmu.h>
#include <xv6/proc.h>
#include <xv6/seg.h>
#include <xv6/string.h>
#include <xv6/types.h>
#include <xv6/vm.h>

pte_t *kpml4;

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void) {
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEGDESC64_CODE(0);
  c->gdt[SEG_KDATA] = SEGDESC32_DATA(0);
  c->gdt[SEG_UCODE] = SEGDESC64_CODE(3);
  c->gdt[SEG_UDATA] = SEGDESC32_DATA(3);
  lgdt(c->gdt, sizeof(c->gdt));
  asm volatile("movabsq %0, %%rax\n"
               "pushq %0\n"
               "movabsq $1f, %%rax\n"
               "pushq %%rax\n"
               "lretq\n"
               "1:\n"
               "movw %1, %%ax\n"
               "movw %%ax, %%ds\n"
               "movw %%ax, %%es\n"
               "movw %%ax, %%ss\n"
               "movw $0, %%ax\n"
               "movw %%ax, %%fs\n"
               "movw %%ax, %%gs\n"

               ::"i"((ulong)(SEG_KCODE << 3)),
               "i"(SEG_KDATA << 3)
               : "rax");
}

static pte_t *get_pml1(pte_t *pml2, ulong va, int alloc) {
  pte_t *pml1;
  pte_t *pml2e = &pml2[PML2X(va)];

  if (*pml2e & PTE_P)
    return (pte_t *)P2V(PTE_ADDR(*pml2e));
  else {
    if (!alloc || (pml1 = (pte_t *)kalloc()) == 0)
      return 0;
    memset(pml1, 0, PGSIZE);
    *pml2e = V2P((ulong)pml1) | PTE_P | PTE_W | PTE_U;
    return pml1;
  }
}

static pte_t *get_pml2(pte_t *pml3, ulong va, int alloc) {
  pte_t *pml2;
  pte_t *pml3e = &pml3[PML3X(va)];

  if (*pml3e & PTE_P)
    return (pte_t *)P2V(PTE_ADDR(*pml3e));
  else {
    if (!alloc || (pml2 = (pte_t *)kalloc()) == 0)
      return 0;
    memset(pml2, 0, PGSIZE);
    *pml3e = V2P((ulong)pml2) | PTE_P | PTE_W | PTE_U;
    return pml2;
  }
}

static pte_t *get_pml3(pte_t *pml4, ulong va, int alloc) {
  pte_t *pml3;
  pte_t *pml4e = &pml4[PML4X(va)];

  if (*pml4e & PTE_P)
    return (pte_t *)P2V(PTE_ADDR(*pml4e));
  else {
    if (!alloc || (pml3 = (pte_t *)kalloc()) == 0)
      return 0;
    memset(pml3, 0, PGSIZE);
    *pml4e = V2P((ulong)pml3) | PTE_P | PTE_W | PTE_U;
    return pml3;
  }
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *walkpml4(pte_t *pml4, ulong va, int alloc) {
  pte_t *pml1, *pml2, *pml3;
  if (!(pml3 = get_pml3(pml4, va, alloc)) ||
      !(pml2 = get_pml2(pml3, va, alloc)) ||
      !(pml1 = get_pml1(pml2, va, alloc)))
    return 0;

  return &pml1[PML1X(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int mappages(pte_t *pml4, ulong va, ulong size, ulong pa, ulong perm) {
  pte_t *pml1e;

  ulong a = PGROUNDDOWN(va);
  ulong last = PGROUNDDOWN(va + size - 1);
  for (;;) {
    if ((pml1e = walkpml4(pml4, a, 1)) == 0)
      return -1;
    if (*pml1e & PTE_P)
      panic("remap");
    *pml1e = pa | perm | PTE_P;
    if (a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// Set up kernel part of a page table.
pte_t *setupkvm(void) {
  pte_t *pml4;
  if ((pml4 = (pte_t *)kalloc()) == 0)
    return 0;
  memset(pml4, 0, PGSIZE);
  for (struct kmap *k = &kmap[0]; k->phys_start || k->phys_end; k++)
    if (mappages(pml4, P2V(k->phys_start), k->phys_end - k->phys_start,
                 k->phys_start, k->perm) < 0) {
      freevm(pml4);
      return 0;
    }

  return pml4;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void kvmalloc(void) {
  kpml4 = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void switchkvm(void) {
  lcr3((ulong)V2P(kpml4)); // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void switchuvm(struct proc *p) {
  if (p == 0)
    panic("switchuvm: no process");
  if (p->kstack == 0)
    panic("switchuvm: no kstack");
  if (p->pml4 == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = TSSDESC_LO(&mycpu()->ts);
  mycpu()->gdt[SEG_TSS_HI] = TSSDESC_HI(&mycpu()->ts);

  mycpu()->ts.rsp0 = (ulong)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iopb = 0xFFFFFFFFU;
  ltr(SEG_TSS << 3);
  lcr3((ulong)V2P(p->pml4)); // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void inituvm(pte_t *pml4, char *init, ulong sz) {
  char *mem;

  if (sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pml4, 0, PGSIZE, V2P((ulong)mem), PTE_W | PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pte_t *pml4, ulong addr, struct inode *ip, uint offset, uint sz) {
  ulong i, n;
  ulong pa;
  pte_t *pml1e;

  if (addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for (i = 0; i < sz; i += PGSIZE) {
    if ((pml1e = walkpml4(pml4, addr + i, 0)) == 0 || !(*pml1e & PTE_P))
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pml1e);
    if (sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if (readi(ip, (char *)P2V(pa), offset + i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pte_t *pml4, ulong oldsz, ulong newsz, ulong perm) {
  char *mem;
  ulong a;

  if (newsz >= KERNBASE)
    return 0;
  if (newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for (; a < newsz; a += PGSIZE) {
    mem = kalloc();
    if (mem == 0) {
      cprintf("allocuvm out of memory\n");
      deallocuvm(pml4, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if (mappages(pml4, a, PGSIZE, V2P((ulong)mem), perm | PTE_U) < 0) {
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pml4, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}

static void free_range_1(pte_t *pml1, ulong start, ulong end) {
  if (PML2X(start) != PML2X(end - 1))
    panic("tried to free memory across different PML1s");
  for (; start < end; start += PGSIZE) {
    pte_t *pml1e = &pml1[PML1X(start)];
    if (*pml1e & PTE_P) {
      kfree((char *)P2V(PTE_ADDR(*pml1e)));
      *pml1e = 0;
    }
  }
}

static void free_range_2(pte_t *pml2, ulong start, ulong end) {
  if (PML3X(start) != PML3X(end - 1))
    panic("tried to free memory across different PML2s");
  ulong start1 = start;
  ulong end1;
  while (start1 < end) {
    pte_t *pml2e = &pml2[PML2X(start1)];
    ulong next_2m_boundary =
        ALIGN(start1 + (1UL << PML2XSHIFT), 1UL << PML2XSHIFT);
    if (*pml2e & PTE_P) {
      end1 = MIN(end, next_2m_boundary);
      free_range_1((pte_t *)P2V(PTE_ADDR(*pml2e)), start1, end1);
      start1 = end1;
    } else {
      start1 = next_2m_boundary;
    }
  }
}

static void free_range_3(pte_t *pml3, ulong start, ulong end) {
  if (PML4X(start) != PML4X(end - 1))
    panic("tried to free memory across different PML3s");
  ulong start1 = start;
  ulong end1;
  while (start1 < end) {
    pte_t *pml3e = &pml3[PML3X(start1)];
    ulong next_1g_boundary =
        ALIGN(start1 + (1UL << PML3XSHIFT), 1UL << PML3XSHIFT);
    if (*pml3e & PTE_P) {
      end1 = MIN(end, next_1g_boundary);
      free_range_2((pte_t *)P2V(PTE_ADDR(*pml3e)), start1, end1);
      start1 = end1;
    } else {
      start1 = next_1g_boundary;
    }
  }
}

static void free_range_4(pte_t *pml4, ulong start, ulong end) {
  if (PML5X(start) != PML5X(end - 1))
    panic("tried to free memory across different PML4s");
  ulong start1 = start;
  ulong end1;
  while (start1 < end) {
    pte_t *pml4e = &pml4[PML4X(start1)];
    ulong next_512g_boundary =
        ALIGN(start1 + (1UL << PML4XSHIFT), 1UL << PML4XSHIFT);
    if (*pml4e & PTE_P) {
      end1 = MIN(end, next_512g_boundary);
      free_range_3((pte_t *)P2V(PTE_ADDR(*pml4e)), start1, end1);
      start1 = end1;
    } else {
      start1 = next_512g_boundary;
    }
  }
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pte_t *pml4, ulong oldsz, ulong newsz) {
  if (newsz >= oldsz)
    return oldsz;

  free_range_4(pml4, VPN(PGROUNDUP(newsz)), VPN(PGROUNDUP(oldsz)));

  return newsz;
}

static void free_pml2(pte_t *pml2) {
  if (pml2 == 0)
    panic("freevm: no pml2");
  for (int i = 0; i < NR_PTE; i++)
    if (pml2[i] & PTE_P)
      kfree((void *)P2V(PTE_ADDR(pml2[i])));
  kfree(pml2);
}

static void free_pml3(pte_t *pml3) {
  if (pml3 == 0)
    panic("freevm: no pml3");
  for (int i = 0; i < NR_PTE; i++)
    if (pml3[i] & PTE_P)
      free_pml2((pte_t *)P2V(PTE_ADDR(pml3[i])));
  kfree(pml3);
}

static void free_pml4(pte_t *pml4) {
  if (pml4 == 0)
    panic("freevm: no pml4");
  for (int i = 0; i < NR_PTE; i++)
    if (pml4[i] & PTE_P)
      free_pml3((pte_t *)P2V(PTE_ADDR(pml4[i])));
  kfree(pml4);
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pte_t *pml4) {
  deallocuvm(pml4, KERNBASE, 0);
  free_pml4(pml4);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pte_t *pml4, ulong uva) {
  pte_t *pte;

  pte = walkpml4(pml4, uva, 0);
  if (pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pte_t *copyuvm(pte_t *pml4, ulong sz) {
  pte_t *d;
  pte_t *pte;
  ulong pa, i;
  ulong flags;
  char *mem;

  if ((d = setupkvm()) == 0)
    return 0;
  for (i = 0; i < sz; i += PGSIZE) {
    if ((pte = walkpml4(pml4, i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if ((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char *)P2V(pa), PGSIZE);
    if (mappages(d, i, PGSIZE, (ulong)V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//  Map user virtual address to kernel address.
ulong uva2ka(pte_t *pml4, ulong uva) {
  pte_t *pte;

  pte = walkpml4(pml4, uva, 0);
  if ((*pte & PTE_P) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  return P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pte_t *pml4, ulong va, ulong p, ulong len) {
  char *buf;
  ulong n;
  ulong pa0, va0;

  buf = (char *)p;
  while (len > 0) {
    va0 = PGROUNDDOWN(va);
    pa0 = uva2ka(pml4, va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if (n > len)
      n = len;
    memmove((void *)(pa0 + (va - va0)), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}
