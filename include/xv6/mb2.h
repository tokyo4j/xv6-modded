#pragma once

#ifndef __ASSEMBLER__

#include <xv6/types.h>
extern uint mb2_info_ptr;
void mb2_init(void);

#else

#define MB2_MAGIC 0x36d76289

#endif