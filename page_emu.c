
#include "page_emu.h"
#include "CPUemu.h"
#include "page.h"
#include "system.h"
#include "page.h"
#include "memory.h"
#include "gdebug.h"

static DWORD *vm_tlb_buffer;

/* 
 * 0: 4kbyte only
 * 1: 4k or 4M 
 */
static DWORD PSE_flag = 0x0;

extern DWORD *pg1_PDE_entry;

static void traversal_PDE_entry(DWORD pde_physic_addr);
static void _check_4k_page_entry(DWORD pde_physic_addr);
static void _check_4m_page_entry(DWORD pde_physic_addr);

static DWORD _get_page_value(DWORD physic_addr);

void cpu_init_page_emu(void)
{    
    /*
    vm_tlb_buffer = _sys_kmalloc(0x10000 * sizeof(DWORD) + 0x1000);
    
    if(vm_tlb_buffer==NULL){
        _sys_freeze("fatal error: cannot alloc TLB buffer\n");
    }
    
    _sys_memset32(vm_tlb_buffer, 0, 0x10000 * sizeof(DWORD) + 0x1000);
    */
    
    //_sys_printf(" TLB buffer...%x\n", vm_tlb_buffer);
    
}

void cpu_emu_flush_tlb(DWORD page_dir_base_addr)
{    
    DWORD virtual_memsize = *P_VIRTUAL_MEMSIZE;
    DWORD pde_physic_addr = (DWORD)pg1_PDE_entry;
    
    pde_physic_addr += virtual_memsize;
    
    traversal_PDE_entry(page_dir_base_addr);
    
    /* ñ{CPUÇ…Ç‡tlbïœçXÇÃé|Çì`Ç¶ÇÈ */
    _sys_set_cr3(pde_physic_addr);
    
}


static void traversal_PDE_entry(DWORD pde_physic_addr)
{    
    if (PSE_flag) {
        /* 4k or 4m */
        _check_4m_page_entry(pde_physic_addr);
    } else {
        /* 4k only */
        _check_4k_page_entry(pde_physic_addr);
    }
}

static void _check_4k_page_entry(DWORD pde_physic_addr)
{    
    int   i;
    DWORD PDE_value;
    DWORD *addr = (DWORD*)pde_physic_addr;

    for (i = 0 ; i < 1024 ; i++){
        PDE_value = _get_page_value((DWORD)(addr + i));
        
        /* îOÇÃÇΩÇﬂÉNÉäÉA */
        //PDE_value &= ~PAGE_PS_FLAG;
        
        if (PDE_value != 0x0) {
            write_to_vm_page_entry(i, PDE_value);
            //_sys_printf(" %x ", PDE_value);
        }
    }
}

static void _check_4m_page_entry(DWORD pde_physic_addr)
{    
    int   i;
    DWORD PDE_value;
    DWORD *addr = (DWORD*)pde_physic_addr;
    
    for (i = 0 ; i < 1024 ; i++) {
        PDE_value = _get_page_value((DWORD)(addr + i));
        if (PDE_value) {
            write_to_vm_page_entry(i, PDE_value);
            //_sys_printf(" %x ", PDE_value);
        }
    }
}

static DWORD _get_page_value(DWORD physic_addr)
{    
    WORD    data_segment = _USER_DS;
    DWORD   ret;
    
    asm volatile (
        "pushw          %%gs                    \n"
        "movw           %%cx, %%gs              \n"
        "movl           %%gs:(%%ebx), %%eax     \n"
        "popw           %%gs                    \n"
    : "=a" (ret) :
    "c" (data_segment),
    "b" (physic_addr)
    );
    
    return ret;
}

void change_PSE_mode(void)
{
    PSE_flag = (~PSE_flag) & 0x1;
}

void write_to_vm_page_entry(int page_num, DWORD value)
{
    pg1_PDE_entry[page_num] = value;
}
