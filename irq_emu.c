
#include        "irq_emu.h"
#include        "irq.h"
#include        "io.h"
#include        "idt.h"
#include        "interrupt_emu.h"
#include        "gdebug.h"
#include        "system.h"
#include        "vga.h"

static BYTE master_pic_irq_base = 0;
static BYTE slave_pic_irq_base = 0;

void regist_master_idt_base_addr(BYTE base_addr)
{
    master_pic_irq_base = base_addr;
}

void regist_slave_idt_base_addr(BYTE base_addr)
{
    slave_pic_irq_base = base_addr;
}

//void redirect_irq(DWORD irq_num, pushed_regs *regs, DWORD error_code, va_list arg){
void redirect_irq(DWORD irq_num, pushed_regs *regs, va_list arg)
{    
    DWORD      target_vect_num;
    irq_stack  *common_stack;
    intr_stack *stack;
    va_list    orig_list = arg;
    
    common_stack = &va_arg(arg, irq_stack);
    
    if (common_stack->eflags & 0x20000) {
        _sys_printf("not support v8086 irq\n");
        return;
    }
    
    if (common_stack->cs == _KERNEL_CS) {
        return;
    }else{
        stack = &va_arg(orig_list, intr_stack);
    }
    
    if (irq_num >= 8) {
        target_vect_num = slave_pic_irq_base + (irq_num - 8);
    } else {
        target_vect_num = master_pic_irq_base + irq_num;
    }
    
    /* _sys_printf("idt: %d, irq: %d\n", target_vect_num, irq_num); */
    
    /* jmp_v_idtr(regs, &stack, target_vect_num, 1, error_code); */
    jmp_v_idtr(regs, stack, target_vect_num, 0, 0);
    
}
