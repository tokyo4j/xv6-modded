#include <xv6/stat.h>
#include <xv6/types.h>
#include <xv6/user.h>

#include <stdarg.h>

static void putc(int fd, char c) { write(fd, &c, 1); }

static void printint(int fd, int xx, int base, int sgn) {
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if (sgn && xx < 0) {
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  if (neg)
    buf[i++] = '-';

  while (--i >= 0)
    putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void printf(int fd, const char *fmt, ...) {
  int c, i, state;
  va_list ap;
  va_start(ap, fmt);

  state = 0;
  for (i = 0; fmt[i]; i++) {
    c = fmt[i] & 0xff;
    if (state == 0) {
      if (c == '%') {
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if (state == '%') {
      if (c == 'd') {
        int i = va_arg(ap, int);
        printint(fd, i, 10, 1);
      } else if (c == 'x' || c == 'p') {
        int i = (int)va_arg(ap, int);
        printint(fd, i, 16, 0);
      } else if (c == 's') {
        char *s = va_arg(ap, char *);
        if (s == 0)
          s = "(null)";
        while (*s != 0) {
          putc(fd, *s);
          s++;
        }
      } else if (c == 'c') {
        char c = va_arg(ap, int);
        putc(fd, c);
      } else if (c == '%') {
        putc(fd, c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        putc(fd, '%');
        putc(fd, c);
      }
      state = 0;
    }
  }
}
