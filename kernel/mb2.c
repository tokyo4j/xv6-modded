#include <multiboot2.h>
#include <xv6/console.h>
#include <xv6/fb.h>
#include <xv6/kalloc.h>
#include <xv6/memlayout.h>
#include <xv6/misc.h>
#include <xv6/mmu.h>
#include <xv6/mp.h>
#include <xv6/types.h>

__attribute__((section(".multiboot2"))) struct {
  struct multiboot_header header __attribute__((aligned(8)));

  struct multiboot_header_tag_framebuffer fb_req __attribute__((aligned(8)));

  struct multiboot_header_tag_information_request info_req
      __attribute__((aligned(8)));
  multiboot_uint32_t tag_types[1];

  struct multiboot_header_tag null_tag __attribute__((aligned(8)));
} mb2_header = {
    .header =
        {
            .magic = MULTIBOOT2_HEADER_MAGIC,
            .architecture = MULTIBOOT_ARCHITECTURE_I386,
            .header_length = sizeof(mb2_header.header),
            .checksum = 0x100000000 -
                        (MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 +
                         sizeof(mb2_header.header)),
        },
    .fb_req =
        {
            .type = MULTIBOOT_HEADER_TAG_FRAMEBUFFER,
            .size = sizeof(mb2_header.fb_req),
            .width = 1920,
            .height = 1080,
            .depth = FB_DEPTH << 3,
        },
    .info_req =
        {
            .type = MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST,
            .size = sizeof(mb2_header.info_req) + sizeof(mb2_header.tag_types),
        },
    .tag_types = {MULTIBOOT_TAG_TYPE_FRAMEBUFFER},
};

uint mb2_magic;
char *mb2_info_ptr;
char saved_mmap[1024];

#define ALIGNUP8(a) (typeof(a))(((ulong)(a) + 7) & ~7U)

static void print_mmap_entry(struct multiboot_mmap_entry *entry) {
  static int i = 0;
  cprintf(
      "[%2d] 0x%016x-0x%016x %s\n", i++, entry->addr, entry->addr + entry->len,
      entry->type == MULTIBOOT_MEMORY_AVAILABLE          ? "available"
      : entry->type == MULTIBOOT_MEMORY_RESERVED         ? "reserved"
      : entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE ? "ACPI reclaimable"
      : entry->type == MULTIBOOT_MEMORY_NVS              ? "NVS"
      : entry->type == MULTIBOOT_MEMORY_BADRAM           ? "bad RAM"
                                                         : "other");
}

// Needs direct mapped memory
void mb2_init(void) {
  if (!mb2_info_ptr)
    asm("hlt");

  struct multiboot_tag_framebuffer *fb_tag = 0;
  struct multiboot_tag_mmap *mmap_tag = 0;
  struct multiboot_tag_new_acpi *new_acpi_tag = 0;
  struct multiboot_tag_old_acpi *old_acpi_tag = 0;

  // first 4 byte contains total size of MBI (multiboot information structure)
  uint mb2_info_size = *(uint *)mb2_info_ptr;

  for (struct multiboot_tag *p = (struct multiboot_tag *)(mb2_info_ptr + 8);
       p < (struct multiboot_tag *)(mb2_info_ptr + mb2_info_size);
       p = (struct multiboot_tag *)ALIGNUP8((char *)p + p->size)) {
    switch (p->type) {
      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
        fb_tag = (struct multiboot_tag_framebuffer *)p;
        break;
      case MULTIBOOT_TAG_TYPE_MMAP:
        mmap_tag = (struct multiboot_tag_mmap *)p;
        break;
      case MULTIBOOT_TAG_TYPE_ACPI_NEW:
        new_acpi_tag = (struct multiboot_tag_new_acpi *)p;
        break;
      case MULTIBOOT_TAG_TYPE_ACPI_OLD:
        old_acpi_tag = (struct multiboot_tag_old_acpi *)p;
        break;
    }
  }

  if (!fb_tag)
    asm("hlt");
  else {
    fb_init(fb_tag->common.framebuffer_addr, fb_tag->common.framebuffer_width,
            fb_tag->common.framebuffer_height, fb_tag->common.framebuffer_bpp,
            fb_tag->common.framebuffer_pitch);
  }

  if (new_acpi_tag)
    mpinit((struct rsdp_desc *)P2V((ulong)new_acpi_tag->rsdp));
  else if (old_acpi_tag)
    mpinit((struct rsdp_desc *)P2V((ulong)old_acpi_tag->rsdp));
  else
    panic("RSDP not found");

  if (!mmap_tag)
    panic("memory map not found");
  for (struct multiboot_mmap_entry *entry = mmap_tag->entries;
       entry <
       (struct multiboot_mmap_entry *)((char *)mmap_tag + mmap_tag->size);
       entry = (struct multiboot_mmap_entry *)((char *)entry +
                                               mmap_tag->entry_size)) {
    print_mmap_entry(entry);
    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
      // Exclude kernel program and area below 1MB
      ulong pa_start = entry->addr;
      ulong pa_end = entry->addr + entry->len;
      if (pa_start < V2P((ulong)end))
        pa_start = V2P((ulong)end);
      if (pa_start >= pa_end)
        continue;
      kmap_add(pa_start, pa_end, PTE_XD | PTE_W, 1);
    }
  }

  // Now that memory map is filled, we can give a continuous chunk of memory to
  // the framebuffer to use for double buffering.
  fb_setup_double_buffering();

  // Add memory map of kernel itself
  kmap_add(KERNOFFSET, V2P((ulong)etext), 0, 0);
  kmap_add(V2P((ulong)etext), V2P((ulong)data), PTE_XD, 0);
  kmap_add(V2P((ulong)data), V2P((ulong)end), PTE_XD | PTE_W, 0);

  // Add memory map of below 1MB (used in startothers())
  kmap_add(0, KERNOFFSET, PTE_XD | PTE_W, 0);
}
