#include <stdarg.h>

enum format_type { CHAR = 0, DECIMAL = 1, HEX = 2, STRING = 3, PERCENT = 4 };

struct spec {
  int zero_padding;
  int width;
  enum format_type type;
};

static int copy_str(char *dst, const char *src) {
  int read = 0;
  while ((*dst++ = *src++) != 0)
    if (++read > 256)
      break;
  return read;
}

static int long2str(char *dst, long src, int width, int zero_padding) {
  char buf[20];
  buf[19] = 0;
  int i;
  for (i = 18; src; i--) {
    buf[i] = '0' + src % 10;
    src /= 10;
  }
  if (i == 18)
    buf[i--] = '0';
  if (width) {
    char c = zero_padding ? '0' : ' ';
    for (; 18 - i < width; i--) {
      buf[i] = c;
    }
  }
  return copy_str(dst, buf + i + 1);
}

static int ulong2hexstr(char *dst, unsigned long src, int width,
                        int zero_padding) {
  char buf[17];
  buf[16] = 0;
  int i;
  for (i = 15; src; i--) {
    char nibble = src & 0xf;
    buf[i] = nibble < 10 ? nibble + '0' : nibble - 10 + 'a';
    src >>= 4;
  }
  if (i == 15)
    buf[i--] = '0';
  if (width) {
    char c = zero_padding ? '0' : ' ';
    for (; 15 - i < width; i--) {
      buf[i] = c;
    }
  }
  return copy_str(dst, buf + i + 1);
}

static int str2int(int *dst, const char *src) {
  int w = 0;
  int read = 0;
  char c;
  while (1) {
    c = *src++;
    if (!('0' <= c && c <= '9'))
      break;
    read++;
    w *= 10;
    w += c - '0';
  }
  *dst = w;
  return read;
}

static int read_spec(struct spec *spec, const char *src) {
  int read = 0;

  spec->zero_padding = 0;
  spec->width = 0;
  if (*src == '0') {
    spec->zero_padding = 1;
    src++;
    read++;
  }
  if ('1' <= *src && *src <= '9') {
    int r = str2int(&spec->width, src);
    src += r;
    read += r;
  }
  switch (*src) {
    case '%':
      spec->type = PERCENT;
      break;
    case 'c':
      spec->type = CHAR;
      break;
    case 's':
      spec->type = STRING;
      break;
    case 'd':
      spec->type = DECIMAL;
      break;
    case 'x':
    case 'p':
      spec->type = HEX;
      break;
  }

  return ++read;
}

int vsprintf(char *dst, const char *fmt, va_list arg) {
  char c;
  char *s = dst;

  while ((c = *fmt++)) {
    if (c != '%') {
      *s++ = c;
      continue;
    }

    struct spec spec;
    fmt += read_spec(&spec, fmt);

    switch (spec.type) {
      case PERCENT:
        *s++ = '%';
        break;
      case CHAR:
        *s++ = va_arg(arg, int);
        break;
      case STRING:
        s += copy_str(s, va_arg(arg, char *));
        break;
      case DECIMAL:
        s += long2str(s, va_arg(arg, int), spec.width, spec.zero_padding);
        break;
      case HEX:
        s += ulong2hexstr(s, va_arg(arg, long), spec.width, spec.zero_padding);
        break;
    }
  }
  *s++ = 0;

  return s - dst;
}

void sprintf(char *dst, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsprintf(dst, fmt, ap);
  va_end(ap);
}