
#include "tss.h"
#include "gdt.h"
#include "memory.h"
#include "gdebug.h"
#include "system.h"
#include "i8259A_emu.h"
#include "device.h"
#include "system.h"

#define ENTRY_TYPE_TSS       0x89
#define ENTRY_TYPE_TSSbusy   0x8B
#define ENTRY_TYPE_LDT       0x82

#define SMALL_ENTRY          0x00
#define LARGE_ENTRY          0x80

#define TSS_IOMAP_SIZE       8139

/*
static gdtr_table    *gdtr;
static segment_entry *segs;
*/

/* 唯一のTSS領域 */
static TSS_IO        tss0;
static segment_entry ldt0[2];

static void _sys_set_io_map(TSS_IO *tss);
static void _sys_enable_io_num(TSS_IO *tss_io, WORD io_port_num);
static void _sys_disable_io_num(TSS_IO *tss_io, WORD io_port_num);

static void _sys_set_TSS_esp_ss(
    TSS     *tss,
    WORD    ss2,
    DWORD   esp2,
    WORD    ss1,
    DWORD   esp1,
    WORD    ss0,
    DWORD   esp0
);

//LDT不必要、一個だけ用意して、中は0x0でパディング
/*
void _sys_init_TSS(WORD now_ss, DWORD now_esp)
{    
    DWORD   tss_addr        = (DWORD)&tss0;
    DWORD   tss_size        = sizeof(TSS);
    
    tss_addr = liner2Physic(tss_addr);
    
    _sys_printf("TSS setting...\n");
    _sys_printf(" TSS entry base address = %x\n", tss_addr);
    
    _sys_memset32((TSS*)&tss0, 0, sizeof(TSS));
    
    _sys_set_TSS_esp_ss(&tss0, 0x0, 0x0, 0x0, 0x0, now_ss, now_esp);
    
    gdtr = (gdtr_table*)GDTR_ADDR;
    segs = (segment_entry*)GDT_SEGMENT_ADDR;
    
    segs[TSS_ENTRY_NUM].limitL          = (WORD)tss_size;
    segs[TSS_ENTRY_NUM].baseL           = (WORD)tss_addr;
    segs[TSS_ENTRY_NUM].baseM           = (BYTE)(tss_addr >> 16);
    segs[TSS_ENTRY_NUM].baseH           = (BYTE)(tss_addr >> 24);
    segs[TSS_ENTRY_NUM].typeL           = ENTRY_TYPE_TSS;
    segs[TSS_ENTRY_NUM].limitH_typeH    = (BYTE)((tss_size >> 16) | SMALL_ENTRY);
    
    _sys_set_TR(_DEFAULT_TSS_INDEX);
    
}
*/

void _sys_init_TSS(void)
{    
    DWORD         tss_addr = (DWORD)&tss0;
    DWORD         tss_size = sizeof(TSS_IO);
    WORD          i;
    gdtr_table    *gdtr = (gdtr_table*)GDTR_ADDR;
    segment_entry *segs = (segment_entry*)GDT_SEGMENT_ADDR;
    
    tss_addr = liner2Physic(tss_addr);
    
    _sys_printf("TSS setting...\n");
    _sys_printf(" TSS entry base address = %x\n", tss_addr);
    
    _sys_memset32((TSS*)&tss0, 0, sizeof(TSS_IO));
    
    _sys_set_TSS_esp_ss(&tss0.tss, 0x0, 0x0, 0x0, 0x0, _TSS_SS, _TSS_BASE_ESP);
    
    segs[TSS_ENTRY_NUM].limitL        = (WORD)tss_size;
    segs[TSS_ENTRY_NUM].baseL         = (WORD)tss_addr;
    segs[TSS_ENTRY_NUM].baseM         = (BYTE)(tss_addr >> 16);
    segs[TSS_ENTRY_NUM].typeL         = ENTRY_TYPE_TSS;
    segs[TSS_ENTRY_NUM].limitH_typeH  = (BYTE)((tss_size >> 16) | SMALL_ENTRY);
    segs[TSS_ENTRY_NUM].baseH         = (BYTE)(tss_addr >> 24);
    
    _sys_set_io_map(&tss0);    
    _sys_set_TR(_DEFAULT_TSS_INDEX);    
}

static void _sys_set_io_map(TSS_IO *tss_io)
{    
    tss_io->tss.iobase = (WORD)((void*)(tss_io->iomap) - (void*)(tss_io));    
    /* すべて許可 */
    _sys_memset32(tss_io->iomap, 0x0, TSS_IOMAP_SIZE);
    tss_io->iomap[8192] = 0xff;    
}


static void _sys_enable_io_num(TSS_IO *tss_io, WORD io_port_num)
{
    tss_io->iomap[io_port_num / 8] &= ~(1 << (io_port_num % 8));
    
}

static void _sys_disable_io_num(TSS_IO *tss_io, WORD io_port_num)
{    
    tss_io->iomap[io_port_num / 8] |= 1 << (io_port_num % 8);    
}

int start_listen_io(emulation_handler *handler)
{    
    device_node     *new_node = NULL;
    
    new_node = _sys_kmalloc(sizeof(device_node));
    
    if (new_node == NULL) {
        return 0;
    }
    
    new_node->handler   = handler;
    new_node->left = new_node->right = NULL;
    insert_resource(new_node);
    _sys_disable_io_num(&tss0,  handler->io_address);
    
    return 1;
    
}

int stop_listen_io(emulation_handler *handler)
{    
    _sys_enable_io_num(&tss0, handler->io_address);    
    return 1;    
}


//ちょいとここにテスト
void _sys_make_ldt(void)
{    
    DWORD         ldt_size    = (sizeof(segment_entry) * 2) - 1;
    /* LDTは物理アドレスではなくて、セレクタ値：オフセット値であらわす */
    DWORD         ldt_addr    = (DWORD)&ldt0;
    DWORD         base_addr   = kernel_virtual_memsize;
    segment_entry *segs       = (segment_entry*)GDT_SEGMENT_ADDR;
    
    _sys_memset32((segment_entry*)ldt0, 0, sizeof(segment_entry));
    
    segs[LDT_ENTRY_NUM].limitL       = (WORD)ldt_size;
    segs[LDT_ENTRY_NUM].baseL        = (WORD)ldt_addr;
    segs[LDT_ENTRY_NUM].baseM        = (BYTE)(ldt_addr >> 16);
    segs[LDT_ENTRY_NUM].baseH        = (BYTE)(ldt_addr >> 24);
    segs[LDT_ENTRY_NUM].typeL        = 0x82;
    segs[LDT_ENTRY_NUM].limitH_typeH  = (BYTE)((ldt_size >> 16) | 0x00);
    
    //_sys_set_LDTR(_DEFAULT_LDT);
    _sys_set_LDTR(0x0);    
}


static void _sys_set_TSS_esp_ss(TSS    *tss,
                                WORD   ss2,
                                DWORD  esp2,
                                WORD   ss1,
                                DWORD  esp1,
                                WORD   ss0,
                                DWORD  esp0)
{
    tss->ss2    = ss2;
    tss->esp2   = esp2;
    
    tss->ss1    = ss1;
    tss->esp1   = esp1;
    
    tss->ss0    = ss0;
    tss->esp0   = esp0;
    
    tss->ldt = 0x0;
    tss->iobase = 0x0;
}

/*
void _sys_set_tss_ss_esp_0(WORD ss, DWORD esp){
    
    tss0.ss0 = ss;
    tss0.esp0 = esp;
    
}


void _sys_set_tss_ss_esp_1(WORD ss, DWORD esp){
    
    tss0.ss1 = ss;
    tss0.esp1 = esp;
    
}

void _sys_set_tss_ss_esp_2(WORD ss, DWORD esp){
    
    tss0.ss2 = ss;
    tss0.esp2 = esp;
    
}
*/

