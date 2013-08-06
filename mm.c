
#include    "asm-i386.h"
#include    "system.h"
#include    "vga.h"
#include    "mm.h"

static void print_mem_type(int type);

void _sys_setup_mem(void)
{   
    _e820map *mmmap = P_E820_ENTRY;
    DWORD   i;
    DWORD       high_mem = 0;
    DWORD       mem_addr;
    
    _sys_printf("main memory check\n");
    
    for (i = 0 ; i < mmmap->length ; i++) {        
        mem_addr = mmmap->entry[i].base_addr_low +  mmmap->entry[i].length_low;
        _sys_printf(" %d: %x .. %x ", 
            i,
            mmmap->entry[i].base_addr_low,
            mem_addr
        );
        
        print_mem_type(mmmap->entry[i].type);
        _sys_putchar('\n');
        
        if (high_mem < mem_addr) {
            high_mem = mem_addr;
        }
    }
    _sys_printf("total memory: %d [kbytes]\n", mem_addr / 1024);
}

static void print_mem_type(int type)
{    
    _sys_putchar('(');    
    switch (type) {        
        case 1:
            _sys_printf("usable");
            break;
        case 2:
            _sys_printf("unusable");
            break;
        case 3:
            _sys_printf("ACPI");
            break;
        case 4:
            _sys_printf("ACPI NVS");
            break;
    }
    _sys_putchar(')');
}
