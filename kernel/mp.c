// Multiprocessor support
// Search memory for MADT (Multiple APIC Description Table).

#include <xv6/defs.h>
#include <xv6/memlayout.h>
#include <xv6/mmu.h>
#include <xv6/mp.h>
#include <xv6/param.h>
#include <xv6/proc.h>
#include <xv6/types.h>
#include <xv6/x86.h>

struct cpu cpus[NCPU];
int ncpu;
uchar ioapicid;

static uchar sum(uchar *addr, int len) {
  int i, sum;

  sum = 0;
  for (i = 0; i < len; i++)
    sum += addr[i];
  return sum;
}

static struct rsdp_desc *find_rsdp_desc() {
  ushort ebda_base = *(ushort *)0x40e << 4;

  for (uchar *p = (uchar *)(uint)ebda_base; p < (uchar *)(uint)ebda_base;
       p += 16)
    if (memcmp(p, "RSD PTR ", 8) == 0 && sum(p, sizeof(struct rsdp_desc)) == 0)
      return (struct rsdp_desc *)p;

  for (uchar *p = (uchar *)0xe0000; p < (uchar *)0x100000; p += 16)
    if (memcmp(p, "RSD PTR ", 8) == 0 && sum(p, sizeof(struct rsdp_desc)) == 0)
      return (struct rsdp_desc *)p;
  return 0;
}

// We cannot dereference physical 2GB~2GB+4MB because virtual 2GB~2GB+4MB is
// mapped to physical 0~4MB
#define IS_DIRECT_MAPPED(pa)                                                   \
  ((uint)(pa) < 0x80000000 || (uint)(pa) >= 0x80400000)

static struct madt *find_madt() {
  struct rsdp_desc *rsdp_desc = find_rsdp_desc();
  if (!rsdp_desc)
    return 0;

  if (!IS_DIRECT_MAPPED(rsdp_desc->rsdt_addr) ||
      !IS_DIRECT_MAPPED(rsdp_desc->rsdt_addr))
    return 0;

  struct rsdt *rsdt = (struct rsdt *)(rsdp_desc->rsdt_addr);
  if (memcmp(rsdt->h.signature, "RSDT", 4) != 0 ||
      sum((uchar *)rsdt, rsdt->h.len) != 0)
    return 0;

  int nr_other_sdts = (rsdt->h.len - sizeof(struct sdt_header)) >> 2;

  for (int i = 0; i < nr_other_sdts; i++) {
    struct sdt_header *header = (struct sdt_header *)rsdt->children[i];
    if (memcmp(header->signature, "APIC", 4) == 0 &&
        sum((uchar *)header, header->len) == 0)
      return (struct madt *)header;
  }

  return 0;
}

void mpinit(void) {
  struct madt *madt = find_madt();
  if (!madt)
    panic("MADT not found");

  lapic = (uint *)madt->lapic_addr;
  cprintf("LAPIC: found at 0x%x\n", madt->lapic_addr);

  char *madt_end = (char *)madt + madt->h.len;
  for (struct madt_header *header = (struct madt_header *)(madt + 1);
       header < (struct madt_header *)madt_end;
       header = (struct madt_header *)((char *)header + header->len)) {
    switch (header->type) {
      // LAPIC
      case 0: {
        struct madt_lapic *lapic_entry = (struct madt_lapic *)header;
        if (ncpu < NCPU && lapic_entry->flags & 1U) {
          cprintf("CPU[%d]: APIC PID: %d APIC ID: %d FLAGS: 0x%x\n",
                  ncpu,
                  lapic_entry->apic_pid,
                  lapic_entry->apic_id,
                  lapic_entry->flags);
          cpus[ncpu].apicid = lapic_entry->apic_id;
          ncpu++;
        }
        break;
      }
      // IO APIC
      case 1: {
        struct madt_ioapic *ioapic_entry = (struct madt_ioapic *)header;
        cprintf("IOAPIC: ID:%d GSI:%d at 0x%x",
                ioapic_entry->ioapic_id,
                ioapic_entry->gsi_base,
                ioapic_entry->ioapic_addr);
        if (ioapic_entry->gsi_base != 0) {
          cprintf(" - ignored\n");
          break;
        } else
          cprintf("\n");

        ioapicid = ioapic_entry->ioapic_id;
        ioapic = (struct ioapic *)ioapic_entry->ioapic_addr;
        break;
      }
    }
  }
}