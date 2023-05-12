// PC keyboard interface constants

#pragma once

#include <xv6/types.h>

// C('A') == Control-A
#define C(x) (x - '@')

void kbdintr(void);
