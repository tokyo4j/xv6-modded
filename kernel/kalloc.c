// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include <xv6/console.h>
#include <xv6/kalloc.h>
#include <xv6/memlayout.h>
#include <xv6/misc.h>
#include <xv6/mmu.h>
#include <xv6/spinlock.h>
#include <xv6/string.h>
#include <xv6/types.h>

void freerange(ulong vstart, ulong vend);

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

struct kmap kmap[KMAP_MAX_SIZE];

void kmap_add(ulong pa_start, ulong pa_end, ulong perm, int heap) {
  static int kmap_cur = 0;
  if (kmap_cur >= KMAP_MAX_SIZE)
    panic("Too many kmap entries");
  struct kmap *km = &kmap[kmap_cur++];

  km->phys_start = pa_start;
  km->phys_end = pa_end;
  km->perm = perm;
  km->heap = heap;
}

void kinit1(void) {
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  for (struct kmap *km = kmap; km->phys_start || km->phys_end; km++)
    if (km->heap)
      freerange(P2V(km->phys_start), P2V(km->phys_end));
}

void kinit2(void) { kmem.use_lock = 1; }

void freerange(ulong vstart, ulong vend) {
  char *p;
  p = (char *)PGROUNDUP(vstart);
  for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
    kfree(p);
}

//  Free the page of physical memory pointed at by v,
//  which normally should have been returned by a
//  call to kalloc().  (The exception is when
//  initializing the allocator; see kinit above.)
void kfree(void *va) {
  struct run *r;
  ulong v = (ulong)va;

  // if (PGALIGNED((ulong)v) || v < end || V2P(v) >= PHYSTOP) TODO
  if (!PGALIGNED(v) || v < (ulong)end)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset((void *)v, 1, PGSIZE);

  if (kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run *)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if (kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r;

  if (kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  if (kmem.use_lock)
    release(&kmem.lock);
  return (void *)r;
}
