
#include		"kernel.h"
#include		"stack.h"
#include		"CPUemu.h"

typedef struct _v_jmp_data {
	DWORD		new_esp;
	DWORD		emued_ss;
	DWORD		eip;
	DWORD		raw_stack_cs;
	DWORD		eflags;
	DWORD		esp;
	DWORD		raw_stac_ss;
} v_jmp_data;


void cpu_emu_sti(vm_emu_datas *status);

void cpu_emu_cli(vm_emu_datas *status);

void cpu_emu_iret(vm_emu_datas *status);

void regist_v_idtr(DWORD addr);

void jmp_v_idtr(pushed_regs *regs, intr_stack *stack, DWORD idt_num, int use_err_code, DWORD error_code);

void get_guest_idt_handler(DWORD vect_num, WORD *cs_value, DWORD *eip_value);

/* —áŠO‚ð‚Â‚Â‚Þ */

/*
	“ÁŒ ƒŒƒxƒ‹‚Í–¾‚ç‚©‚É•Ï‚í‚é‚Å‚µ‚å‚¤
*/

/* 0 */
void _sys_wrap_divide_error_fault(pushed_regs regs, intr_stack stack);

/* 1 */
void _sys_wrap_debug_fault(pushed_regs regs, intr_stack stack);

/* 2 */
void _sys_wrap_nmi_fault(pushed_regs regs, intr_stack stack);

/* 3 */
void _sys_wrap_break_point(pushed_regs regs, intr_stack stack);

/* 4 */
void _sys_wrap_overflow_fault(pushed_regs regs, intr_stack stack);

/* 5 */
void _sys_wrap_bound_fault(pushed_regs regs, intr_stack stack);

/* 6 */
void _sys_wrap_invalid_opcode(pushed_regs regs, intr_stack stack);

/* 7 */
void _sys_wrap_device_not_available(pushed_regs regs, intr_stack stack);

/* 8 */
void _sys_wrap_double_fault(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 9 */
void _sys_wrap_cop_seg_overflow(pushed_regs regs, intr_stack stack);

/* 10 */
void _sys_wrap_invalid_tss(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 11 */
void _sys_wrap_seg_not_present(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 12 */
void _sys_wrap_stack_exception(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 13 */
/* void _sys_wrap_general_protection(pushed_regs regs, DWORD error_code, intr_stack stack); */
void _sys_redirect_general_protect(vm_emu_datas *status);

/* 14 */
void _sys_wrap_page_fault(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 16 */
void _sys_wrap_floating_point_error(pushed_regs regs, intr_stack stack);

/* 17 */
void _sys_wrap_alignment_check_error(pushed_regs regs, DWORD error_code, intr_stack stack);

/* 18 */
void _sys_wrap_machine_check_error(pushed_regs regs, intr_stack stack);

/* 19 */
void _sys_wrap_simd_float_error(pushed_regs regs, intr_stack stack);


void _sys_wrap_ignore_int(pushed_regs regs, DWORD idt_num, intr_stack stack);


