
#include		"gdebug.h"
#include		"system.h"

void _sys_dump_cpu(pushed_regs regs){
	
	
	_sys_printf(
		"\n"
		"(A):\n"
		"ebx: %x ecx: %x edx: %x\n"
		"esi: %x edi: %x ebp: %x\n"
		"eax: %x\n"
		"ds: %x es: %x\n",
		regs.ebx, regs.ecx, regs.edx,
		regs.esi, regs.edi, regs.ebp,
		regs.eax,
		regs.ds, regs.es
	);
	
}


void _sys_pushed_cpu(pushed_regs *regs){
	
	
	_sys_printf(
		"\n"
		"(B):\n"
		"ebx: %x ecx: %x edx: %x\n"
		"esi: %x edi: %x ebp: %x\n"
		"eax: %x\n"
		"ds: %x es: %x\n",
		regs->ebx, regs->ecx, regs->edx,
		regs->esi, regs->edi, regs->ebp,
		regs->eax,
		regs->ds, regs->es
	);
	
}


int test_trap(){
	
	return 0x3;
}
