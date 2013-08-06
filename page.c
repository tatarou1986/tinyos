
#define     _DEBUG_MODE

#include "page.h"
#include "system.h"

#ifdef _DEBUG_MODE
    #include        "gdebug.h"
#endif

/* 0x9f000からページエントリ配置 */
DWORD   *pg0_PDE_entry;
DWORD   *pg0_PTE_entry;

DWORD   *pg1_PDE_entry;

static DWORD    pg1_PDE_physic_addr;

static void _sys_make_PTEentry(DWORD, DWORD, DWORD*, BYTE);
static void _sys_make_PDEentry(DWORD, DWORD, DWORD*, BYTE);
static void _sys_make_4M_PDEentry(DWORD, DWORD, DWORD*, BYTE);

void _sys_set_pg0_paging(void)
{ 
    register int i = 0;
    DWORD        virtual_memsize;
    DWORD        PDE_physic_addr, PTE_physic_addr;
    PagingInfo   info;
    
    virtual_memsize = *P_VIRTUAL_MEMSIZE;
    PDE_physic_addr = (virtual_memsize + PAGE_ENTRY_POINT) & 0xfffff000;
    PTE_physic_addr = PDE_physic_addr + (4 << 10);      //4byteのページエントリが1024個ある
    
    //適当にページ位置を決定
    //pg0_PDE_entry = (DWORD*)PAGE_SHIFT(PAGE_ENTRY_POINT);
    pg0_PDE_entry = (DWORD*)(PDE_physic_addr - virtual_memsize);
    //PTEエントリはPDEエントリの真上に置く
    pg0_PTE_entry = (DWORD*)((DWORD)pg0_PDE_entry + (4 << 10));
    
    //デバッグ用
    //__DEBUG(virtual_memsize, (DWORD)pg0_PDE_entry, (DWORD)pg0_PTE_entry);
    
    //ページディレクトリおよびページエントリをすべて初期化
    for (i = 0 ; i < PAGE_4K_ENTRY_COUNT ; i++){
        pg0_PDE_entry[i] = pg0_PTE_entry[i] = 0x0;
    }
    
    info.PTE_physic_addr    = PTE_physic_addr;
    info.PTEentry_data      = pg0_PTE_entry;
    info.PDEentry_data      = pg0_PDE_entry;
    
    /*  
     * 第1段階ページング設定
     * ページエントリを作成する 
     * まず、0xC0000000 -> virtual_memsize (4kb * 0xA0個)
    */
    for(info.liner_addr     = virtual_memsize,
        info.physic_addr    = virtual_memsize;
        info.liner_addr < virtual_memsize + 0xA0000;
        info.liner_addr     += pg0_PAGEBLOCK_SIZE,
        info.physic_addr    += pg0_PAGEBLOCK_SIZE){
        
        info.flag               = PG_F_P | PG_F_RW | PG_F_US;
        
        _sys_link_physic2liner(&info);
        
    }
    
    /*
     * メモリホールをマッピング
     * 0xA0000～0x100000
     */
    for(info.liner_addr     = virtual_memsize + 0xA0000,
        info.physic_addr    = 0xA0000;
        info.liner_addr < virtual_memsize + 0x100000;
        info.liner_addr     += pg0_PAGEBLOCK_SIZE,
        info.physic_addr    += pg0_PAGEBLOCK_SIZE){
                
        _sys_link_physic2liner(&info);
    }
    
    /*
     * HMA領域0x10FFF0をマッピングする，これはDMA用
     */
    for(info.liner_addr     = virtual_memsize + 0x100000,
        info.physic_addr    = 0x100000;
        info.liner_addr < virtual_memsize + 0x10FFF0;
        info.liner_addr     += pg0_PAGEBLOCK_SIZE,
        info.physic_addr    += pg0_PAGEBLOCK_SIZE){
        
        _sys_link_physic2liner(&info);
    }
    
    _sys_set_cr3(PDE_physic_addr);
    _sys_enable_paging();
    
    return;
}

/*
 * 第二段階ページング、Guset空間用。4Mbyteページを使用します
 * 0x0 -> virtual_memsize
 */
void _sys_set_pg1_paging(void)
{    
    DWORD       virtual_memsize;
    DWORD       liner_addr, physic_addr;
    void        *PDE_physic_addr;
    
    virtual_memsize = *P_VIRTUAL_MEMSIZE;
    
    /* PDEは共用 */
    pg1_PDE_entry = pg0_PDE_entry;
    
    PDE_physic_addr = (void*)(pg1_PDE_entry) + virtual_memsize;
    
    for (liner_addr  = 0x0,
         physic_addr = 0x0;
         liner_addr < virtual_memsize - pg1_PAGEBLOCK_SIZE;
         liner_addr  += pg1_PAGEBLOCK_SIZE,
         physic_addr += pg1_PAGEBLOCK_SIZE){
      
        _sys_make_4M_PDEentry(liner_addr, physic_addr, pg1_PDE_entry, PG_F_RW | PG_F_US | PG_F_P);
    }
    
    /* 4Mbyteページ混在 */
    _sys_use_both_page();
    
    /* tlbフラッシュ */
    _sys_set_cr3((DWORD)PDE_physic_addr);
}

void _sys_link_physic2liner(PagingInfo *info)
{
    _sys_make_PDEentry(info->liner_addr, info->PTE_physic_addr, info->PDEentry_data, info->flag);
    _sys_make_PTEentry(info->liner_addr, info->physic_addr, info->PTEentry_data, info->flag);
}

static void _sys_make_PTEentry(DWORD liner_addr, DWORD physic_addr, 
                               DWORD *entry, BYTE flag)
{
    entry[PAGE_PTE_ENTRY_NUM((DWORD)liner_addr)]
        = (((DWORD)physic_addr & 0xfffff000) | flag);
}

static void _sys_make_PDEentry(DWORD liner_addr, DWORD PTE_entry_addr, 
                               DWORD *entry, BYTE flag)
{    
    entry[PAGE_PDE_ENTRY_NUM((DWORD)liner_addr)]
        = (((DWORD)PTE_entry_addr & 0xfffff000) | flag);        
}

static void _sys_make_4M_PDEentry(DWORD liner_addr, DWORD physic_addr, 
                                  DWORD *entry, BYTE flag)
{
    entry[PAGE_PDE_ENTRY_NUM(liner_addr)] = ((physic_addr & 0x3ff00000) | 0x80 | flag);
}

DWORD _sys_pg1_cr3_value(void)
{
    return pg1_PDE_physic_addr;
}


void _sys_set_cr3(DWORD PDEentry_phyis_addr)
{    
    DWORD       cr3_value = 0x0;       
    cr3_value   = (PDEentry_phyis_addr & 0xfffff000);    
    asm volatile (
        "movl       %0, %%cr3   \n"
    : /* no output */ : "r"(cr3_value));    
}

void _sys_enable_paging(void)
{    
    asm volatile (
        "movl       %%cr0, %%eax        \n"
        "orl        $0x80000000, %%eax  \n"
        "movl       %%eax, %%cr0        \n"
    : /* no output */ : /* no input */ : "%eax");    
}

void _sys_use_both_page(void)
{    
    asm volatile (
        "movl       %%cr4, %%eax        \n"
        "orl        $0x00000010, %%eax  \n"
        "movl       %%eax, %%cr4        \n"
    : /* no output */ : /* no input */ : "%eax");       
}

