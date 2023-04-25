// References:
// https://wiki.osdev.org/RSDP
// https://wiki.osdev.org/RSDT
// https://wiki.osdev.org/MADT

#pragma once

#include <xv6/types.h>

// clang-format off
/*
Example:

  +-rsdp_desc-+
  | signature |< signature == "RSD PTR "
  |    ...    |
  | rsdt_addr |----->+------rsdt------+
  +-----------+      | h (sdt_header) |< h.signature == "RSDT"
                     |   children[0]  |
                     |   children[1]  |----->+------madt-------+
                     |   children[2]  |      |  h (sdt_header) |< h.signature == "APIC"
                     |   children[3]  |      |    lapic_addr   |
                     +----------------+      |      flags      |
                                             +---madt_lapic----+
                                             | h (madt_header) |< h.type == 0
                                             |     apic_pid    |
                                             |     apic_id     |
                                             |      flags      |
                                             +---madt_lapic----+
                                             |       ...       |
                                             +---madt_ioapic---+
                                             | h (madt_header) |< h.type == 1
                                             |    ioapic_id    |
                                             |     reserved    |
                                             |   ioapic_addr   |
                                             |     gsi_base    |
                                             +---madt_ioapic---+
                                             |       ...       |
                                             +-----------------+
*/
// clang-format on

struct rsdp_desc {
  char signature[8];
  uchar checksum;
  char oem_id[6];
  uchar rev;
  uint rsdt_addr;
} __attribute__((packed));

struct sdt_header {
  char signature[4];
  uint len;
  uchar rev;
  uchar checksum;
  char oem_id[6];
  char oem_tbl_id[8];
  uint oem_rev;
  uint creator_id;
  uint creator_rev;
};

struct rsdt {
  struct sdt_header h;
  uint children[];
};

struct madt {
  struct sdt_header h;
  uint lapic_addr;
  uint flags;
};

struct madt_header {
  uchar type;
  uchar len;
};

// madt_header::type == 0
struct madt_lapic {
  struct madt_header h;
  uchar apic_pid;
  uchar apic_id;
  uint flags; // bit 0 is set when the CPU can be enabled
};

// madt_header::type == 1
struct madt_ioapic {
  struct madt_header h;
  uchar ioapic_id;
  uchar reserved;
  uint ioapic_addr;
  uint gsi_base;
};
