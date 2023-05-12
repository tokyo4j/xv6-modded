#pragma once

#include <xv6/types.h>

struct ioapic {
  uint reg;
  uint pad[3];
  uint data;
};

// ioapic.c
void ioapicenable(int irq, int cpu);
extern volatile struct ioapic *ioapic;
void ioapicinit(void);

// lapic.c
int lapicid(void);
extern volatile uint *lapic;
void lapiceoi(void);
void lapicinit(void);
void lapicstartap(uint apicid, uint addr);
void microdelay(int);
