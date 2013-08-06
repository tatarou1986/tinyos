
#include "io.h"
#include "idt.h"
#include "vga.h"
#include "system.h"
#include "ATAdrv.h"
#include "gdebug.h"
#include "irq_emu.h"
#include "ignore_int.h"

/*
 * きたねぇ...マクロで何とかならんもんか...
 * 本体はidts.sにある
 */
extern void _sys_intrruptIRQ0(void);
extern void _sys_intrruptIRQ1(void);
extern void _sys_intrruptIRQ2(void);
extern void _sys_intrruptIRQ3(void);
extern void _sys_intrruptIRQ4(void);
extern void _sys_intrruptIRQ5(void);
extern void _sys_intrruptIRQ6(void);
extern void _sys_intrruptIRQ7(void);
extern void _sys_intrruptIRQ8(void);
extern void _sys_intrruptIRQ9(void);
extern void _sys_intrruptIRQ10(void);
extern void _sys_intrruptIRQ11(void);
extern void _sys_intrruptIRQ12(void);
extern void _sys_intrruptIRQ13(void);
extern void _sys_intrruptIRQ14(void);
extern void _sys_intrruptIRQ15(void);

extern void _sys_ignore_int(void);
extern void _sys_simd_float_error(void);
extern void _sys_machine_check_error(void);
extern void _sys_alignment_check_error(void);
extern void _sys_floating_point_error(void);
extern void _sys_page_fault(void);
extern void _sys_general_protection(void);
extern void _sys_stack_exception(void);
extern void _sys_seg_not_present(void);
extern void _sys_invalid_tss(void);
extern void _sys_cop_seg_overflow(void);
extern void _sys_double_fault(void);
extern void _sys_device_not_available(void);
extern void _sys_invalid_opcode(void);
extern void _sys_bound_fault(void);
extern void _sys_overflow_fault(void);
extern void _sys_break_point(void);
extern void _sys_nmi_fault(void);
extern void _sys_debug_fault(void);
extern void _sys_divide_error_fault(void);

/* 割り込みベクタ */
static IntGateDisc  intvec[IDTNUM] = {{0,0,0,0}};

/* 割り込みベクタテーブル */
static DescTblPtr  InitTable;
static IRQdatas    IRQ_table[MAX_IRQ_NUM];

/*
 * 割り込み処理
 * これも本体はidts.sにある
 */
static void _missing_IRQ_handler(void);
static void _sys_loadIDT(DescTblPtr *ptr);
static void _sys_SetVector(void);
static void enable_NMI(void);
static void disable_NMI(void);

static void _sys_set_gatedisc(
                IntGateDisc         *GateDisc,
                void                *offset,
                unsigned short      type,
                unsigned short      segment_select
            );

static void _sys_loadIDT(DescTblPtr *ptr)
{    
    asm volatile (
        "lidt   (%0)" 
    : /* no output */ : "q" (ptr));
}

void _sys_SetUpIDT(void)
{
    _sys_printk("IDT Setting...\n");
    
    /* 割り込み仮設定 */
    _sys_SetVector();
    
    /* IRQテーブル初期化 */
    _sys_init_IRQn();
    
    InitTable.limit = sizeof(IntGateDisc) * IDTNUM - 1;
    InitTable.base  = liner2Physic((DWORD)intvec);
    
    _sys_printf(" IDT limit size = %x [bytes]\n", InitTable.limit);
    _sys_printf(" IDT base address = %x\n", InitTable.base);
    
    _sys_loadIDT(&InitTable);
    //_sys_printf(" IDT base address = %x\n", (DWORD)intvec);
    
    /* NMI enable */
    enable_NMI();
    
    /* 割り込み有効化 */
    ENABLE_INT();
    
    return;
}

void _sys_SetVector(void)
{    
    /* idtハンドラを全部埋める */
    _sys_fill_idt_handler();
    
    _sys_set_intrgate(0, &_sys_divide_error_fault);
    _sys_set_intrgate(1, &_sys_debug_fault);
    _sys_set_intrgate(2, &_sys_nmi_fault);
    _sys_set_intrgate(3, &_sys_break_point);
    _sys_set_intrgate(4, &_sys_overflow_fault);
    _sys_set_intrgate(5, &_sys_bound_fault);
    _sys_set_intrgate(6, &_sys_invalid_opcode);
    _sys_set_intrgate(7, &_sys_device_not_available);
    _sys_set_intrgate(8, &_sys_double_fault);
    _sys_set_intrgate(9, &_sys_cop_seg_overflow);
    _sys_set_intrgate(10, &_sys_invalid_tss);
    _sys_set_intrgate(11, &_sys_seg_not_present);
    _sys_set_intrgate(12, &_sys_stack_exception);
    
    _sys_set_intrgate(13, &_sys_general_protection);
    /* これを割り込みゲートにすると...ちょっと */
    /* _sys_set_trapgate(13, &_sys_general_protection); */
    
    _sys_set_intrgate(14, &_sys_page_fault);
    _sys_set_intrgate(16, &_sys_floating_point_error);
    _sys_set_intrgate(17, &_sys_alignment_check_error);
    _sys_set_intrgate(18, &_sys_machine_check_error);
    _sys_set_intrgate(19, &_sys_simd_float_error);
    
    /* IRQ群をすべて登録 */
    _sys_set_intrgate(32, &_sys_intrruptIRQ0);
    _sys_set_intrgate(33, &_sys_intrruptIRQ1);
    _sys_set_intrgate(34, &_sys_intrruptIRQ2);
    _sys_set_intrgate(35, &_sys_intrruptIRQ3);
    _sys_set_intrgate(36, &_sys_intrruptIRQ4);
    _sys_set_intrgate(37, &_sys_intrruptIRQ5);
    _sys_set_intrgate(38, &_sys_intrruptIRQ6);
    _sys_set_intrgate(39, &_sys_intrruptIRQ7);
    _sys_set_intrgate(40, &_sys_intrruptIRQ8);
    _sys_set_intrgate(41, &_sys_intrruptIRQ9);
    _sys_set_intrgate(42, &_sys_intrruptIRQ10);
    _sys_set_intrgate(43, &_sys_intrruptIRQ11);
    _sys_set_intrgate(44, &_sys_intrruptIRQ12);
    _sys_set_intrgate(45, &_sys_intrruptIRQ13);
    _sys_set_intrgate(46, &_sys_intrruptIRQ14);
    _sys_set_intrgate(47, &_sys_intrruptIRQ15);
}

void _sys_init_IRQn(void)
{    
    register int i = 0;
    
    for ( ; i < MAX_IRQ_NUM ; i++) {
        IRQ_table[i].status         = IRQ_DISABLE;
        IRQ_table[i].pic_func       = &pic_handlers;
        IRQ_table[i].action_handler = &_missing_IRQ_handler;
        IRQ_table[i].process_type   = IRQ_PROC_TYPE_1;
    }
}


void _sys_set_irq_handle(DWORD irq_num, void *handle_func)
{
    IRQ_table[irq_num].status         = IRQ_VM_RESERVE;
    IRQ_table[irq_num].action_handler = handle_func;
}


void _sys_vm_reserve_irq_line(DWORD irq_num)
{
    IRQ_table[irq_num].status  = IRQ_VM_RESERVE;
}

void _sys_release_irq_line(DWORD irq_num)
{
    IRQ_table[irq_num].status   = IRQ_RELEASE;
}


void _sys_enable_irq_line(DWORD irq_num)
{
    IRQ_table[irq_num].status = IRQ_VM_RESERVE;
    IRQ_table[irq_num].pic_func->enable(irq_num);
}

void _sys_disable_irq_line(DWORD irq_num)
{
    IRQ_table[irq_num].pic_func->ack(irq_num);
}

void _sys_change_irq_proc_type(DWORD irq_num, BYTE type)
{
    IRQ_table[irq_num].process_type = type;
}

static void _sys_set_gatedisc(IntGateDisc *GateDisc, void *offset,
                              unsigned short type,
                              unsigned short segment_select)
{
    GateDisc->offset_L       = (unsigned int)(offset) & 0x0000ffff;         /* Lowをぶち込む */
    GateDisc->offset_H       = ((unsigned int)(offset) & 0xffff0000) >> 16; /* Highをぶち込む */
    GateDisc->type           = type;
    GateDisc->segment_select = segment_select;
    
    return;
}

void _sys_set_intrgate(DWORD vector_num, void *intrr_handle)
{
    //_sys_set_gatedisc(&intvec[vector_num], intrr_handle, (0x8000 | 0xE00), KERNEL_CS);
    _sys_set_gatedisc(&intvec[vector_num], intrr_handle, 0xEE00, _KERNEL_CS);
}


void _sys_set_trapgate(DWORD vector_num, void *trap_handle)
{
    //_sys_set_gatedisc(&intvec[vector_num], trap_handle, (0x8000 | 0xF00), KERNEL_CS);
    _sys_set_gatedisc(&intvec[vector_num], trap_handle, 0xEF00, _KERNEL_CS);
}


void manual_eoi(DWORD irq_num)
{
    IRQ_table[irq_num].pic_func->end(irq_num);
}


void _IRQ_handler(pushed_regs regs, DWORD IRQ_num, ...)
{    
    va_list     arg;
    
    switch (IRQ_table[IRQ_num].status) {
        case IRQ_VM_RESERVE:
            switch(IRQ_table[IRQ_num].process_type){
                case IRQ_PROC_TYPE_1:
                    /* VMが使用する */
                    IRQ_table[IRQ_num].status = IRQ_INPROGRESS;
                    IRQ_table[IRQ_num].pic_func->ack(IRQ_num);
                    
                    /* メインのハンドラを呼び出す */
                    IRQ_table[IRQ_num].action_handler();
                    IRQ_table[IRQ_num].status = IRQ_VM_RESERVE;
                    IRQ_table[IRQ_num].pic_func->enable(IRQ_num);
                    IRQ_table[IRQ_num].pic_func->end(IRQ_num);
                    
                    break;
                
                case IRQ_PROC_TYPE_3:
                    /* メインのハンドラを呼び出す */
                    IRQ_table[IRQ_num].action_handler();
                    break;
                    
                case IRQ_PROC_TYPE_2:
                    /* VMが使用する */
                    IRQ_table[IRQ_num].status = IRQ_INPROGRESS;
                    IRQ_table[IRQ_num].pic_func->ack(IRQ_num);
                    IRQ_table[IRQ_num].action_handler();
                    
                    break;
                    
            }
            break;
            
        case IRQ_RELEASE:
            /* IRQ_table[IRQ_num].status = IRQ_GUEST_INPROGRESS; */
            
            IRQ_table[IRQ_num].pic_func->ack(IRQ_num);
            
            va_start(arg, IRQ_num);
            
            /* GusetOSにリダイレクションする */
            redirect_irq(IRQ_num, &regs, arg);
            
            IRQ_table[IRQ_num].pic_func->enable(IRQ_num);
            IRQ_table[IRQ_num].pic_func->end(IRQ_num);
            
            /* 
             * 本来ならここに戻ってくることはないが、割り込みが
             * マスクされていたりして、戻ってきた場合は後始末をVMが行う
             */
            break;
            
        case IRQ_INPROGRESS:
            break;
            
        default:
            break;
    }
}

static void _missing_IRQ_handler(void)
{
    _sys_printk("Fatal Error: Missing IRQ handler!!\n");
}


static void enable_NMI(void)
{
    BYTE tmp = inb(0x70) & 0x7F;
    outb(0x70, tmp);
    delay(2);
}

static void disable_NMI(void)
{    
    BYTE tmp = inb(0x70) | 0x80;    
    outb(0x70, tmp);
    delay(2);
}

/*
static void _sys_invalid_opcode(void){
    
    _sys_printk("invalid opcode\n");
    
    SEND_MASTER_EOI();
    
    asm volatile (
        "leave      \n"
        "iret       \n"
    :: );
    
}

static void _sys_test(void){
    
    _sys_printk("key push\n");
    
    SEND_MASTER_EOI();
    
    asm volatile (
        "leave      \n"
        "iret       \n"
    :: );
    
}

*/
            
