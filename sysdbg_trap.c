
#include "sysdbg_trap.h"
#include "sysdbg_trap"

void init_dbg_reg(void)
{
}

void init_trap_ss_esp(int dr_num, WORD ss, DWORD esp)
{   
}

void set_break_point(int dr_num, DWORD liner_addr)
{   
    switch(dr_num){
    case 0:
        asm volatile (
            "movl   %%eax, %%dr0    \n"
            : /* no output */ : "a"(liner_addr));
        break;
            
    case 1:
        asm volatile (
            "movl   %%eax, %%dr1, \n"
            : /* no output */ : "a"(liner_addr));
        break;
            
    case 2:
        asm volatile (
            "movl   %%eax, %%dr2,   \n"
            : /* no output */ : "a"(liner_addr));
        break;
            
    case 3:
        asm volatile (
            "movl   %%eax, %%dr3    \n"
            : /* no output */ : "a" (liner_addr));
        break;
    }
}

void enable_break_point(int dr_num, BYTE break_point_type, BYTE len)
{    
    DWORD   value = 0x0;
    
    value |= ((DWORD)break_point_type) << (16 + ((dr_num & 3) * 4));
    value |= ((DWORD)len) << (18 + ((dr_num & 3) * 4));
    value |= len;    
    
    asm volatile (
        "movl       %%eax, %%dr7    \n"
        : /* no output */ : "a"(value));    
}
