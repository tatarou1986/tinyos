
#ifndef __MISC_H__
#define __MISC_H__

#include "kernel.h"

typedef struct {
    DWORD a, b, c, d;
} cpuid_regs;

void sysenter_setup(void);
//void start_usercodechunk_for_sysenter(void);
void start_ring3_codechunk_for_sysenter(void);

void cpuid(DWORD eax, cpuid_regs *regs);
DWORD read_msr(DWORD msr);
void write_msr(DWORD msr, 
               DWORD low, DWORD high);

#endif
