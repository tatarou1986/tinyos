
#include		"ctrl_reg_emu.h"
#include		"cpuseg_emu.h"
#include		"system.h"
#include		"CPUemu.h"
#include		"gdebug.h"
#include		"gdt.h"
#include		"page_emu.h"


#define			DEFAULT_CR0_VALUE		0x00000010
#define			DEFAULT_CR1_VALUE		0x00000000
#define			DEFAULT_CR2_VALUE		0x00000000
#define			DEFAULT_CR3_VALUE		0x00000000
#define			DEFAULT_CR4_VALUE		0x00000000


static void refresh_cr_reg(DWORD reg_num);

static void write_cr0(DWORD value, vm_emu_datas *status);
static void write_cr1(DWORD value, vm_emu_datas *status);
static void write_cr2(DWORD value, vm_emu_datas *status);
static void write_cr3(DWORD value, vm_emu_datas *stauts);
static void write_cr4(DWORD value, vm_emu_datas *status);

static DWORD read_cr0();
static DWORD read_cr1();
static DWORD read_cr2();
static DWORD read_cr3();
static DWORD read_cr4();

/* cr0 */
static void alt_PE_flag(vm_emu_datas*);
static void alt_MP_flag(vm_emu_datas*);
static void alt_EM_flag(vm_emu_datas*);
static void alt_TS_flag(vm_emu_datas*);
static void alt_ET_flag(vm_emu_datas*);
static void alt_NE_flag(vm_emu_datas*);
static void alt_WP_flag(vm_emu_datas*);
static void alt_AM_flag(vm_emu_datas*);
static void alt_NW_flag(vm_emu_datas*);
static void alt_CD_flag(vm_emu_datas*);
static void alt_PG_flag(vm_emu_datas*);

/* cr3 */
static void alt_PWT_flag(vm_emu_datas*);
static void alt_PCD_flag(vm_emu_datas*);

/* cr4 */
static void alt_VME_flag(vm_emu_datas*);
static void alt_PVI_flag(vm_emu_datas*);
static void alt_TSD_flag(vm_emu_datas*);
static void alt_DE_flag(vm_emu_datas*);
static void alt_PSE_flag(vm_emu_datas*);
static void alt_PAE_flag(vm_emu_datas*);
static void alt_MCE_flag(vm_emu_datas*);
static void alt_PGE_flag(vm_emu_datas*);
static void alt_PCE_flag(vm_emu_datas*);
static void alt_OSFXSR_flag(vm_emu_datas*);
static void alt_OSXMMEXCPT_flag(vm_emu_datas*);

/* PEフラグ時に使う */
static void make_tmp_16bit_seg(vm_emu_datas* status);

static DWORD	emu_cr_reg[5] = {0};

static DWORD	(*func_table[])(vm_emu_datas*) = {
	&ret_eax,
	&ret_ecx,
	&ret_edx,
	&ret_ebx,
	NULL,
	&ret_ebp,
	&ret_esi,
	&ret_edi
};

static int	bit16[] = {6, 7, 5, 3};

static void (*cr0_func_table[])(vm_emu_datas*) = {
	&alt_PE_flag,
	&alt_MP_flag,
	&alt_EM_flag,
	&alt_TS_flag,
	&alt_ET_flag,
	&alt_NE_flag,
	&alt_WP_flag,
	&alt_AM_flag,
	&alt_NW_flag,
	&alt_CD_flag,
	&alt_PG_flag
};


static void (*cr4_func_table[])(vm_emu_datas*) = {
	&alt_VME_flag,
	&alt_PVI_flag,
	&alt_TSD_flag,
	&alt_DE_flag,
	&alt_PSE_flag,
	&alt_PAE_flag,
	&alt_MCE_flag,
	&alt_PGE_flag,
	&alt_PCE_flag,
	&alt_OSFXSR_flag,
	&alt_OSXMMEXCPT_flag
};


void cpu_init_ctrlreg_emu(){
	
	emu_cr_reg[0] = DEFAULT_CR0_VALUE;
	emu_cr_reg[1] = DEFAULT_CR1_VALUE;
	emu_cr_reg[2] = DEFAULT_CR2_VALUE;
	emu_cr_reg[3] = DEFAULT_CR3_VALUE;
	emu_cr_reg[4] = DEFAULT_CR4_VALUE;
	
}


static void write_cr0(DWORD value, vm_emu_datas *status){
	
	DWORD	pattern = emu_cr_reg[0] ^ value;
	DWORD	tmp;
	DWORD	i 		= 0;
	
	emu_cr_reg[0] = value;
	
	if(pattern & PE_FLAG){
		cr0_func_table[0](status);
	}else if(pattern & MP_FLAG){
		cr0_func_table[1](status);
	}else if(pattern & EM_FLAG){
		cr0_func_table[2](status);
	}else if(pattern & TS_FLAG){
		cr0_func_table[3](status);
	}else if(pattern & ET_FLAG){
		cr0_func_table[4](status);
	}else if(pattern & NE_FLAG){
		cr0_func_table[5](status);
	}else if(pattern & WP_FLAG){
		cr0_func_table[6](status);
	}else if(pattern & AM_FLAG){
		cr0_func_table[7](status);
	}else if(pattern & NW_FLAG){
		cr0_func_table[8](status);
	}else if(pattern & CD_FLAG){
		cr0_func_table[9](status);
	}else if(pattern & PG_FLAG){
		cr0_func_table[10](status);
	}
	
}


static void write_cr1(DWORD value, vm_emu_datas *status){
	
	emu_cr_reg[1] = value;
	
	asm volatile (
		"movl	%%eax, %%cr1"
	:: "a"(value));
	
}


static void write_cr2(DWORD value, vm_emu_datas *status){
	
	emu_cr_reg[2] = value;
	
	refresh_cr_reg(2);
	
}


static void write_cr3(DWORD value, vm_emu_datas *status){
	
	DWORD	tmp = emu_cr_reg[3] ^ value;
	
	if(tmp & 0x8){
		alt_PWT_flag(status);
	}else if(tmp & 0x10){
		alt_PCD_flag(status);
	}
	
	cpu_emu_flush_tlb(value & 0xfffff000);
	
	emu_cr_reg[3] = value;
	
}


static void write_cr4(DWORD value, vm_emu_datas *status){
	
	DWORD	pattern = emu_cr_reg[4] ^ value;
	DWORD	i 		= 0;
	DWORD	tmp;
	
	do{
		tmp = pattern >> i;
		
		if(tmp & 0x1){
			cr4_func_table[i](status);
		}
		
		++i;
	}while(tmp);
	
	emu_cr_reg[4] = value;
	
}


static DWORD read_cr0(){
	return emu_cr_reg[0];
}


static DWORD read_cr1(){
	return emu_cr_reg[1];
}


static DWORD read_cr2(){
	return emu_cr_reg[2];
}

static DWORD read_cr3(){
	return emu_cr_reg[3];
}


static DWORD read_cr4(){
	return emu_cr_reg[4];
}


static void refresh_cr_reg(DWORD reg_num){
	
	switch(reg_num){
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
	}
	
	return;
}


/* 0x22 */
void emu_mov_crn_reg(vm_emu_datas *status){
	
	BYTE	modr_m	= get_userseg_byte(status->code_addr);
	DWORD	tmp		= modr_m & 0xf;
	DWORD	value;
	
	switch(modr_m >> 4){
		case 0xC:
			if(tmp >= 8){
				value = func_table[tmp - 8](status);
			}else{
				value = func_table[tmp](status);
				//_sys_printf("write! %x\n", value);
				write_cr0(value, status);
			}
			break;
		case 0xD:
			if(tmp >= 8){
				value = func_table[tmp - 8](status);
				write_cr3(value, status);
			}else{
				value = func_table[tmp](status);
				write_cr2(value, status);
			}
			break;
		case 0xE:
			value = func_table[tmp](status);
			write_cr4(value, status);
			break;
	}
	
}


/* 0x20 */
void emu_mov_reg_crn(vm_emu_datas *status){

	BYTE	modr_m	= get_userseg_byte(status->code_addr);
	DWORD	tmp		= modr_m & 0xf;
	DWORD	*reg_table[] = {
		&status->regs->eax,
		&status->regs->ecx,
		&status->regs->edx,
		&status->regs->ebx,
		NULL,
		&status->regs->ebp,
		&status->regs->esi,
		&status->regs->edi
	};
	
	switch(modr_m >> 4){
		case 0xC:
			if(tmp >= 8){
				*(reg_table[tmp - 8]) = read_cr1();
			}else{
				*(reg_table[tmp]) = read_cr0();
			}
			break;
		case 0xD:
			if(tmp >= 8){
				*(reg_table[tmp - 8]) = read_cr3();
			}else{
				*(reg_table[tmp]) = read_cr2();
			}
			break;
		case 0xE:
			*(reg_table[tmp]) = read_cr4();
			break;
	}

}

void emu_lmsw(BYTE modr_m, vm_emu_datas *status){
	
	DWORD	cr0_value 	= read_cr0() & 0xffff0000;
	
	switch((modr_m >> 4) & 0xf){
		
		case 0xf:
			cr0_value |= func_table[modr_m & 0xf](status) & 0xffff;
			write_cr0(cr0_value, status);
			break;
			
		case 0x3:
			cr0_value |= func_table[bit16[(modr_m & 0xf) - 4]](status) & 0xffff;
			write_cr0(cr0_value, status);
			break;
			
		case 0x7:
			cr0_value |= func_table[bit16[(modr_m & 0xf) - 4]](status) & 0xffff;
			write_cr0(cr0_value, status);
			
			get_userseg_byte(status->code_addr);
			break;
			
	}
	
	
}


static void alt_PE_flag(vm_emu_datas *status){
	
	if(get_vm_mode_status()){
		/* protect_mode -> real_mode */
		/* 今のところは実装しない */
	}else{
		/* real_mode -> protect_mode */
		
		make_tmp_16bit_seg(status);
		
		status->stack.sys_stack->cs = (VM_GDT_ENTRY_V_CS << 3) | 0x3;
		status->stack.sys_stack->ss = (VM_GDT_ENTRY_V_SS << 3) | 0x3;
		
		status->regs->ds = (VM_GDT_ENTRY_V_DS << 3) | 0x3;
		
		/* vm-flagを解除 */
		status->stack.sys_stack->eflags &= 0xfffd7fd7;
		
	}
	
	/* protect_mode or real_mode*/
	//change_vm_mode_status();
	
	/*
	_sys_printf("\nchange mode: %d\n", get_vm_mode_status());
	_sys_printf("cs=%x ss=%x ds=%x\n", status->stack.sys_stack->cs, status->stack.sys_stack->ss, status->regs->ds);
	*/
	
}





static void alt_MP_flag(vm_emu_datas *status){
	
	
	
}


static void alt_EM_flag(vm_emu_datas *status){
	
	
}


static void alt_TS_flag(vm_emu_datas *status){
	
	
	
}


static void alt_ET_flag(vm_emu_datas *status){
	
	
}


static void alt_NE_flag(vm_emu_datas *status){
	
	
}


static void alt_WP_flag(vm_emu_datas *status){
	
	
}


static void alt_AM_flag(vm_emu_datas *status){
	
	
}


static void alt_NW_flag(vm_emu_datas *status){
	
	
}


static void alt_CD_flag(vm_emu_datas *status){
	
	
}


static void alt_PG_flag(vm_emu_datas *status){
	
	
}

/* cr3 */
static void alt_PWT_flag(vm_emu_datas *status){
	
	
}


static void alt_PCD_flag(vm_emu_datas *status){
	
	
}


/* cr4 */
static void alt_VME_flag(vm_emu_datas *status){
	
	
}


static void alt_PVI_flag(vm_emu_datas *status){
	
	
}


static void alt_TSD_flag(vm_emu_datas *status){
	
	
}


static void alt_DE_flag(vm_emu_datas *status){
	
	
}


static void alt_PSE_flag(vm_emu_datas *status){
	
	DWORD		value = read_cr3();
	
	change_PSE_mode();
	
	cpu_emu_flush_tlb(value & 0xfffff000);
	
}


static void alt_PAE_flag(vm_emu_datas *status){
	
	
}


static void alt_MCE_flag(vm_emu_datas *status){
	
	
}


static void alt_PGE_flag(vm_emu_datas *status){
	
	
}


static void alt_PCE_flag(vm_emu_datas *status){
	
	
}


static void alt_OSFXSR_flag(vm_emu_datas *status){
	
	
}


static void alt_OSXMMEXCPT_flag(vm_emu_datas *status){
	
	
}


/* real_mode only */
static void make_tmp_16bit_seg(vm_emu_datas* status){
	
	segment_entry		entry;
	DWORD				base_addr;
	DWORD				limit;
	
	//最低限ds, csだけね
	
	base_addr 	= status->stack.real_mode->cs << 4;
	limit		= 0xfffff;
	
	/* dplを1に設定してみるか... */
	
	/* 16bit csディスクリプタ */
	make_gdt_value(base_addr, limit, GDT_SEG_TYPE_5 | GDT_TYPE_P_FLAG | GDT_TYPE_S_FLAG, 
		0x0, 3, &entry);
	
	write_raw_vm_segment(VM_GDT_ENTRY_V_CS, &entry);
	
	/* ds */
	base_addr	= status->stack.real_mode->ds << 4;
	limit		= 0xfffff;
	
	/* 16bit dsディスクリプタ */
	make_gdt_value(base_addr, limit, GDT_SEG_TYPE_1 | GDT_TYPE_P_FLAG | GDT_TYPE_S_FLAG, 
		0x0, 3, &entry);
		
	write_raw_vm_segment(VM_GDT_ENTRY_V_DS, &entry);
	
	
	/* ss */
	base_addr 	= status->stack.real_mode->ss << 4;
	limit		= 0xfffff;
	
	/* 16bit ssディスクリプタ */
	make_gdt_value(base_addr, limit, GDT_SEG_TYPE_3 | GDT_TYPE_P_FLAG | GDT_TYPE_S_FLAG, 
		0x0, 3, &entry);
		
	write_raw_vm_segment(VM_GDT_ENTRY_V_SS, &entry);
	
	
	//gs, fsはとりあえず保留
	
}


void vm_set_flag_crn(int reg_num, int bit_num){
	
	DWORD	value = 0x1;
	
	value <<= bit_num;
	
	emu_cr_reg[reg_num] |= value;
	
}


void vm_clear_flag_crn(int reg_num, int bit_num){
	
	DWORD	value = 0x1;
	
	value <<= bit_num;
	
	emu_cr_reg[reg_num] &= ~value;
	
}


