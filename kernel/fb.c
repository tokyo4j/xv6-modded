#include <xv6/console.h>
#include <xv6/fb.h>
#include <xv6/kalloc.h>
#include <xv6/memlayout.h>
#include <xv6/misc.h>
#include <xv6/mmu.h>
#include <xv6/string.h>
#include <xv6/types.h>

struct fb_info {
  char *fb;
  ulong size;
  char *buf;
  int width;
  int height;
  int pitch;
  int cwidth;
  int cheight;
  int cx;
  int cy;
  int dirty_x1;
  int dirty_y1;
  int dirty_x2;
  int dirty_y2;
  int dirty;
} fb_info;

#define FHEIGHT         16
#define FWIDTH          8
#define FB_OFFSET(x, y) ((y)*fb_info.pitch + (x)*FB_DEPTH)
#define FB_CUR_OFFSET(x, y, u, v) \
  (((y)*FHEIGHT + (v)) * fb_info.pitch + ((x)*FWIDTH + (u)) * FB_DEPTH)

// PSF1 header is 4 bytes long
uchar (*font_data)[FHEIGHT] =
    (uchar(*)[FHEIGHT])(_binary_kernel_bin_zap_light16_psf_start + 4);

static void mark_dirty(int x1, int y1, int x2, int y2) {
  if (!fb_info.dirty) {
    fb_info.dirty_x1 = x1;
    fb_info.dirty_y1 = y1;
    fb_info.dirty_x2 = x2;
    fb_info.dirty_y2 = y2;
    fb_info.dirty = 1;
  } else {
    fb_info.dirty_x1 = MIN(fb_info.dirty_x1, x1);
    fb_info.dirty_y1 = MIN(fb_info.dirty_y1, y1);
    fb_info.dirty_x2 = MAX(fb_info.dirty_x2, x2);
    fb_info.dirty_y2 = MAX(fb_info.dirty_y2, y2);
  }
}

static void fb_write_char(char chr, int x, int y) {
  uchar *font = font_data[(uchar)chr];

  mark_dirty(x * FWIDTH, y * FHEIGHT, (x + 1) * FWIDTH, (y + 1) * FHEIGHT);

  for (int v = 0; v < FHEIGHT; v++) {
    ushort row = font[v];
    for (int u = 0; u < FWIDTH; u++)
      if (row & (1 << (FWIDTH - 1 - u)))
        *(uint *)(&fb_info.buf[FB_CUR_OFFSET(x, y, u, v)]) = 0xffffffffU;
      else
        *(uint *)(&fb_info.buf[FB_CUR_OFFSET(x, y, u, v)]) = 0x00000000U;
  }
}

static void scroll_down(void) {
  mark_dirty(0, 0, fb_info.width, fb_info.height);

  ulong row_diff = fb_info.pitch * FHEIGHT;
  ulong p = (ulong)fb_info.buf;
  ulong q = (ulong)fb_info.buf + row_diff;
  ulong end = (ulong)fb_info.buf + row_diff * fb_info.cheight;
  for (; q < (ulong)end; p += row_diff, q += row_diff)
    memcpy_fast((void *)p, (void *)q, row_diff);
  memset((void *)p, 0, row_diff);
}

static void move_backward(void) {
  fb_info.cx--;
  if (fb_info.cx < 0) {
    fb_info.cx = fb_info.cwidth - 1;
    fb_info.cy--;
    if (fb_info.cy < 0)
      fb_info.cx = fb_info.cy = 0;
  }
}

static void move_forward(void) {
  fb_info.cx++;
  if (fb_info.cx >= fb_info.cwidth) {
    fb_info.cx = 0;
    fb_info.cy++;
    if (fb_info.cy >= fb_info.cheight) {
      fb_info.cy = fb_info.cheight - 1;
      scroll_down();
    }
  }
}

static void move_down(void) {
  fb_info.cx = 0;
  fb_info.cy++;
  if (fb_info.cy >= fb_info.cheight) {
    fb_info.cy = fb_info.cheight - 1;
    scroll_down();
  }
}

void fb_putc(int c) {
  if (!c)
    return;
  if (c == '\n')
    move_down();
  else if (c == '\b') {
    move_backward();
  } else {
    fb_write_char(c, fb_info.cx, fb_info.cy);
    move_forward();
  }
}

void fb_commit(void) {
  if (fb_info.buf == fb_info.fb)
    return;
  if (!fb_info.dirty)
    return;

  for (int y = fb_info.dirty_y1; y < fb_info.dirty_y2; y++) {
    int offset_start = FB_OFFSET(fb_info.dirty_x1, y);
    int offset_end = FB_OFFSET(fb_info.dirty_x2, y);
    memcpy_fast(fb_info.fb + offset_start, fb_info.buf + offset_start,
                offset_end - offset_start);
  }

  fb_info.dirty = 0;
}

void fb_init(ulong fb_pa, int w, int h, int depth, int pitch) {
  if (depth != FB_DEPTH << 3) {
    for (int i = 0; i < pitch * h; i++)
      ((char *)P2V(fb_pa))[i] = i % 255;
    asm("hlt");
  }

  fb_info = (struct fb_info){
      .fb = (char *)P2V(fb_pa),
      .buf = (char *)P2V(fb_pa),
      .size = pitch * h,
      .width = w,
      .height = h,
      .pitch = pitch,
      .cwidth = w / FWIDTH,
      .cheight = h / FHEIGHT,
      .cx = 0,
      .cy = 0,
      .dirty = 0,
  };
  cprintf("Found framebuffer at 0x%x-0x%x\n", fb_pa, fb_pa + pitch * h);
  kmap_add(fb_pa, fb_pa + pitch * h, PTE_W, 0);
}

void fb_setup_double_buffering(void) {
  for (struct kmap *km = kmap; km->phys_start || km->phys_end; km++) {
    if (km->heap && km->phys_end - km->phys_start > fb_info.size) {
      ulong start = PGROUNDUP(km->phys_start);
      ulong end = PGROUNDUP(start + fb_info.size);
      km->phys_start = end;
      kmap_add(start, end, PTE_W, 0);
      fb_info.buf = (char *)P2V(start);
      memmove(fb_info.buf, fb_info.fb, fb_info.size);
      cprintf("Using framebuffer buffer at 0x%x-0x%x\n", start, end);
      return;
    }
  }
  cprintf("Framebuffer buffer not found\n");
}