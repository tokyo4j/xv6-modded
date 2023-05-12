#pragma once

#include <xv6/types.h>

int argint(int n, int *ip);
int arglong(int n, ulong *ip);
int argptr(int n, char **pp, int size);
int argstr(int n, char **pp);
int fetchlong(ulong addr, ulong *ip);
int fetchint(ulong addr, int *ip);
int fetchstr(ulong addr, char **pp);
void syscall(void);