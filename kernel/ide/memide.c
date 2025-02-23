// Fake IDE disk; stores blocks in memory.
// Useful for running kernel without scratch disk.

#include <xv6/bio.h>
#include <xv6/console.h>
#include <xv6/misc.h>
#include <xv6/string.h>
#include <xv6/types.h>

static int disksize;
static uchar *memdisk;

void ideinit(void) {
  memdisk = (uchar *)_binary_fs_img_start;
  disksize = (uint)((ulong)_binary_fs_img_size / BSIZE);
}

// Interrupt handler.
void ideintr(void) {
  // no-op
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void iderw(struct buf *b) {
  uchar *p;

  if (!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if (b->dev != 1)
    panic("iderw: request not for disk 1");
  if (b->blockno >= disksize)
    panic("iderw: block out of range");

  p = memdisk + b->blockno * BSIZE;

  if (b->flags & B_DIRTY) {
    b->flags &= ~B_DIRTY;
    memmove(p, b->data, BSIZE);
  } else
    memmove(b->data, p, BSIZE);
  b->flags |= B_VALID;
}
