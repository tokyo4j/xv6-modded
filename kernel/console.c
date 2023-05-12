// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include <stdarg.h>
#include <xv6/apic.h>
#include <xv6/console.h>
#include <xv6/fb.h>
#include <xv6/file.h>
#include <xv6/kbd.h>
#include <xv6/proc.h>
#include <xv6/spinlock.h>
#include <xv6/string.h>
#include <xv6/traptbl.h>
#include <xv6/types.h>
#include <xv6/uart.h>

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

void cprintf(const char *fmt, ...) {
  va_list ap;
  char buf[512];

  int locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);

  char *p = buf;
  char c;
  while ((c = *p++))
    consputc(c);

  fb_commit();

  if (locking)
    release(&cons.lock);
}

void panic(const char *s) {
  int i;
  ulong pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

// Doesn't commit framebuffer
void consputc(int c) {
  if (panicked) {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE) {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
    fb_putc('\b');
    fb_putc(' ');
    fb_putc('\b');
  } else {
    uartputc(c);
    fb_putc(c);
  }
}

// should be power of two
#define INPUT_BUF 128

struct {
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;

void consoleintr(int (*getc)(void)) {
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while ((c = getc()) >= 0) {
    switch (c) {
      case C('P'): // Process listing.
        // procdump() locks cons.lock indirectly; invoke later
        doprocdump = 1;
        break;
      case C('U'): // Kill line.
        while (input.e != input.w &&
               input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
          input.e--;
          consputc(BACKSPACE);
        }
        break;
      case C('H'):
      case '\x7f': // Backspace
        if (input.e != input.w) {
          input.e--;
          consputc(BACKSPACE);
        }
        break;
      default:
        if (c != 0 && input.e - input.r < INPUT_BUF) {
          c = (c == '\r') ? '\n' : c;
          input.buf[input.e++ % INPUT_BUF] = c;
          consputc(c);
          if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
            input.w = input.e;
            wakeup(&input.r);
          }
        }
        break;
    }
  }
  fb_commit();
  release(&cons.lock);
  if (doprocdump) {
    procdump(); // now call procdump() wo. cons.lock held
  }
}

static int consoleread(struct inode *ip, char *dst, int n) {
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while (n > 0) {
    while (input.r == input.w) {
      if (myproc()->killed) {
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D')) { // EOF
      if (n < target) {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

static int consolewrite(struct inode *ip, const char *buf, int n) {
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i]);
  fb_commit();
  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void) {
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
