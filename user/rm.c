#include <xv6/stat.h>
#include <xv6/types.h>
#include <xv6/user.h>

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    printf(2, "Usage: rm files...\n");
    exit();
  }

  for (i = 1; i < argc; i++) {
    if (unlink(argv[i]) < 0) {
      printf(2, "rm: %s failed to delete\n", argv[i]);
      break;
    }
  }

  exit();
}
