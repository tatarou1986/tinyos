
#include		"kernel.h"
#include		"stack.h"
#include		"system.h"

void regist_master_idt_base_addr(BYTE base_addr);

void regist_slave_idt_base_addr(BYTE base_addr);

/* void redirect_irq(DWORD irq_num, pushed_regs *regs, DWORD error_code, intr_stack *stack); */

//void redirect_irq(DWORD irq_num, pushed_regs *regs, DWORD error_code, va_list arg);

void redirect_irq(DWORD irq_num, pushed_regs *regs, va_list arg);


