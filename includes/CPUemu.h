
#ifndef CPUemuH
#define CPUemuH

#include		"kernel.h"
#include		"stack.h"

typedef struct _vm_emu_datas {
	void				**code_addr;
	unsigned int		addr_prefix;
	unsigned int		opcode_prefix;
	unsigned int		vm_status;
	pushed_regs			*regs;
	
	union {
		common_stacks		*sys_stack;
		v8086_intr_stack	*real_mode;
		intr_stack			*protect_mode;
	} stack;
	
} vm_emu_datas;

DWORD get_vm_mode_status();
void change_vm_mode_status();

/* ユーザ空間からデータを引っ張ってくる */
BYTE get_userseg_byte(void **addr);
WORD get_userseg_word(void **addr);
DWORD get_userseg_dword(void **addr);

DWORD get_user_dword_value(WORD segment, DWORD ptr);
WORD get_user_word_value(WORD segment, DWORD ptr);

DWORD ret_ebx(vm_emu_datas *status);
DWORD ret_ecx(vm_emu_datas *status);
DWORD ret_edx(vm_emu_datas *status);
DWORD ret_esi(vm_emu_datas *status);
DWORD ret_edi(vm_emu_datas *status);
DWORD ret_ebp(vm_emu_datas *status);
DWORD ret_eax(vm_emu_datas *status);
DWORD ret_value(vm_emu_datas *status);

#endif

