 
#include <gdt.h>

void make_gdt_value(DWORD base_addr, DWORD limit, BYTE typeL, 
                    BYTE typeH, BYTE dpl, segment_entry *entry)
{   
    entry->limitL        = (WORD)limit;
    entry->baseL         = (WORD)base_addr;
    entry->baseM         = (BYTE)((base_addr >> 16) & 0xff);
    entry->typeL         = typeL | ((dpl & 0x3) << 5);
    entry->limitH_typeH  = typeH | ((limit >> 16) & 0xf);
    entry->baseH         = (BYTE)((base_addr >> 24) & 0xff);
}

void _sys_loadGDT(seg_table *p)
{    
    asm volatile (
        "lgdt (%0)" 
    : /* no output */ : "q" (p));
}
