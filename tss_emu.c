
#include		"tss_emu.h"
#include		"tss.h"
#include		"gdebug.h"
#include		"system.h"
#include		"ctrl_reg_emu.h"
#include		"cpuseg_emu.h"
#include		"stack.h"
#include		"memory.h"



static void itr_emu_high_0xD(vm_emu_datas *status, BYTE low_code);
static void itr_emu_high_0x1(vm_emu_datas *status, BYTE low_code);
static void itr_emu_high_0x5(vm_emu_datas *status, BYTE low_code);

static void write_tr_reg(WORD selector);


static WORD		v_tr = 0x0;
static DWORD	(*gen_reg_func[])(vm_emu_datas*) = {
	&ret_eax,
	&ret_ecx,
	&ret_edx,
	&ret_ebx,
	&ret_esi,
	&ret_ebp,
	&ret_esi,
	&ret_edi
};


void cpu_emu_ltr(BYTE modr_m, vm_emu_datas *status){
	
	switch((modr_m >> 4) & 0xf){
		
		case 0xD:
			itr_emu_high_0xD(status, modr_m & 0xf);
			break;
		
		case 0x1:
			itr_emu_high_0x1(status, modr_m & 0xf);
			break;
			
		case 0x5:
			itr_emu_high_0x5(status, modr_m & 0xf);
			break;
		
	}
	
	
}


void cpu_emu_clts(vm_emu_datas *status){
	
	vm_clear_flag_crn(0, 3);
	
	cpu_emu_do_clts();
	
}


void cpu_emu_do_clts(){
	
	asm volatile ("clts");
	
}


void cpu_emu_get_esp_ss_from_tss(int level, WORD *ss, DWORD *esp){
	
	segment_entry		tss_entry;
	TSS					_guest_tss;
	void				*tss_addr;
	
	tss_entry = cpu_emu_get_guest_seg_value(v_tr);
	tss_addr = (void*)((tss_entry.baseH << 24) | (tss_entry.baseM << 16) | tss_entry.baseL);
	
	_sys_physic_mem_read(&_guest_tss, tss_addr, sizeof(TSS));
	
	switch(level){
		
		case 0:
			*ss		= _guest_tss.ss0;
			*esp	= _guest_tss.esp0;
			break;
			
		case 1:
			*ss		= _guest_tss.ss1;
			*esp	= _guest_tss.esp1;
			break;
			
		case 2:
			*ss		= _guest_tss.ss2;
			*esp	= _guest_tss.esp2;
			break;
	}
	
}


static void itr_emu_high_0x5(vm_emu_datas *status, BYTE low_code){
	
	DWORD	value;
		
	if(status->opcode_prefix){
		value = get_user_dword_value(status->regs->ds, status->regs->ebp & 0xffff);
	}else{
		value = get_user_dword_value(status->regs->ds, status->regs->ebp);
	}
	
	/* ‹ó“Ç‚Ý */
	get_userseg_byte(status->code_addr);
	
	write_tr_reg((WORD)value);
	
}


static void itr_emu_high_0xD(vm_emu_datas *status, BYTE low_code){
	
	write_tr_reg(gen_reg_func[low_code - 0x8](status));
	
}

static void itr_emu_high_0x1(vm_emu_datas *status, BYTE low_code){
	
	DWORD		value;
	
	
	if(get_vm_mode_status()){
		if(status->opcode_prefix){
			switch(low_code){
				case 0xC:
					value = status->regs->esi & 0xffff;
					break;
				case 0xD:
					value = status->regs->edi & 0xffff;
					break;
				case 0xF:
					value = status->regs->ebx & 0xffff;
					break;
			}
		}else{
			switch(low_code){
				case 0xC:
					get_userseg_byte(status->code_addr);
					value = status->stack.sys_stack->esp;
					break;
				case 0xD:
					value = get_userseg_dword(status->code_addr);
					break;
				default:
					value = gen_reg_func[low_code - 0x8](status);
					break;
			}
		}
	}else{
		if(status->opcode_prefix){
			switch(low_code){
				case 0xC:
					get_userseg_byte(status->code_addr);
					value = status->stack.sys_stack->esp;
					break;
				default:
					value = gen_reg_func[low_code - 0x8](status);
					break;
			}
		}else{
			switch(low_code){
				case 0xC:
					value = status->regs->esi & 0xffff;
					break;		
				case 0xD:
					value = status->regs->edi & 0xffff;
					break;
				case 0xE:
					value = get_userseg_word(status->code_addr);
					break;
				case 0xF:
					value = status->regs->ebx & 0xffff;
					break;
				
			}
			
		}
		
	}
	
	value = get_user_word_value(status->regs->ds, value);
	write_tr_reg(value);
	
}


static void write_tr_reg(WORD selector){
	
	//_sys_printf("tr load: %x\n", selector);
	
	v_tr = selector;
	
}



