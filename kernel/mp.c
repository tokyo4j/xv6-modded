// Multiprocessor support
// Search memory for MADT (Multiple APIC Description Table).

#include <xv6/apic.h>
#include <xv6/console.h>
#include <xv6/kalloc.h>
#include <xv6/memlayout.h>
#include <xv6/mmu.h>
#include <xv6/mp.h>
#include <xv6/param.h>
#include <xv6/proc.h>
#include <xv6/string.h>
#include <xv6/types.h>

struct cpu cpus[NCPU];
int ncpu;

static uchar sum(uchar *addr, int len) {
  int i, sum;

  sum = 0;
  for (i = 0; i < len; i++)
    sum += addr[i];
  return sum;
}

static struct madt *find_madt(struct rsdp_desc *rsdp_desc) {
  struct rsdt *rsdt = (struct rsdt *)P2V((ulong)(rsdp_desc->rsdt_addr));

  int nr_other_sdts = (rsdt->h.len - sizeof(struct sdt_header)) >> 2;

  for (int i = 0; i < nr_other_sdts; i++) {
    struct sdt_header *header =
        (struct sdt_header *)P2V((ulong)rsdt->children[i]);
    if (memcmp(header->signature, "APIC", 4) == 0 &&
        sum((uchar *)header, header->len) == 0)
      return (struct madt *)header;
  }

  return 0;
}

void mpinit(struct rsdp_desc *rsdp_desc) {
  struct madt *madt = find_madt(rsdp_desc);
  if (!madt)
    panic("MADT not found");

  lapic = (uint *)P2V((ulong)madt->lapic_addr);
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
          cprintf("CPU[%d]: APIC PID: %d APIC ID: %d FLAGS: 0x%x\n", ncpu,
                  lapic_entry->apic_pid, lapic_entry->apic_id,
                  lapic_entry->flags);
          cpus[ncpu].apicid = lapic_entry->apic_id;
          ncpu++;
        }
        break;
      }
      // IO APIC
      case 1: {
        struct madt_ioapic *ioapic_entry = (struct madt_ioapic *)header;
        cprintf("IOAPIC: ID:%d GSI:%d at 0x%x", ioapic_entry->ioapic_id,
                ioapic_entry->gsi_base, ioapic_entry->ioapic_addr);
        if (ioapic_entry->gsi_base != 0) {
          cprintf(" - ignored\n");
          break;
        } else
          cprintf("\n");

        ioapic = (struct ioapic *)P2V((ulong)ioapic_entry->ioapic_addr);
        break;
      }
    }
  }

  kmap_add(0xfec00000, 0x100000000, PTE_W, 0);
}
