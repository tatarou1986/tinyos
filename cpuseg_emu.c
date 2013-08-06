
#include		"cpuseg_emu.h"
#include		"memory.h"
#include		"system.h"
#include		"gdebug.h"
#include		"tss.h"
#include		"tss_emu.h"
#include		"interrupt_emu.h"
#include		"ctrl_reg_emu.h"
#include		"gdt.h"
#include		"vga.h"
#include		"ldt_emu.h"


#define			SEGMENT_REG_CNT				6
#define			_STACK_SIZE					(15 * 4)

#define			GET_PHYSIC_ADDR(segment, addr)		\
				(((segment) << 4) + (addr))			\
				

typedef struct _vm_pseudo_seg {
	WORD		vm_sys_seg;
	WORD		guset_seg;
} vm_pseudo_segment;

/*
static void cpu_emu_write_gdtr(DWORD addr);
static void cpu_emu_write_idtr(DWORD addr);
*/

extern void _sys_8086_to_protect(DWORD new_eip, DWORD new_cs, DWORD eflags, DWORD esp, DWORD ss);
extern void _general_iret(void);

static void cpu_emu_write_ds(DWORD ds_value, vm_emu_datas *status);
static void cpu_emu_write_fs(DWORD fs_value, vm_emu_datas *status);
static void cpu_emu_write_gs(DWORD gs_value, vm_emu_datas *status);
static void cpu_emu_write_es(DWORD es_value, vm_emu_datas *status);
static void cpu_emu_write_ss(DWORD ss_value, vm_emu_datas *status);

static WORD cpu_emu_lxs_high_0x0(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x1(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x2(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x3(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x4(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x5(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x6(vm_emu_datas *status, BYTE low_code);
static WORD cpu_emu_lxs_high_0x7(vm_emu_datas *status, BYTE low_code);

static WORD cpu_emu_lxs_common(vm_emu_datas *status, DWORD *first_oprand, DWORD addr);
void write_raw_vm_segment(DWORD segment_entry_num, segment_entry *info);
static void gdtr_null_load();

/*
void cpu_emu_cli();
void cpu_emu_sti();
void cpu_emu_lgdt();
void cpu_emu_lldt();
*/


static gdtr_table		v_gdtr;
static gdtr_table		*v_tss_table = NULL;

static segment_entry	*v_gdts = NULL;
static segment_entry	*v_idts = NULL;

static gdtr_table		*vm_sys_gdtr;
static segment_entry	*vm_sys_gdts;

static int				already_far_jmp = 0;

/* GuestOSがプロテクトモード時に使用 */
static vm_pseudo_segment v_segments[SEGMENT_REG_CNT] = {
		{0,0}
};

static BYTE level_table[] = {
	1, 2, 2, 3
};

static DWORD	(*func_table[])(vm_emu_datas*) = {
	ret_ebx,
	ret_ecx,
	ret_edx,
	ret_esi,
	ret_edi,
	ret_ebp,
	ret_eax,
	ret_value
};


static WORD (*emu_lxs_func[])(vm_emu_datas*, BYTE) = {
	&cpu_emu_lxs_high_0x0,
	&cpu_emu_lxs_high_0x1,
	&cpu_emu_lxs_high_0x2,
	&cpu_emu_lxs_high_0x3,
	&cpu_emu_lxs_high_0x4,
	&cpu_emu_lxs_high_0x5,
	&cpu_emu_lxs_high_0x6,
	&cpu_emu_lxs_high_0x7
};

static int 	bit32[] = {
	6, 1, 2, 0, -1, 7, 3, 4
};

static int	bit16[] = {
	3, 4, 5, 0
};


seg_type check_segment_type(segment_entry *entry){
	
	BYTE	value;
	
	value = entry->typeL;
	
	if(value & 0x10){
		/* ノーマルのセグメント */
		return SEG_TYPE_GD;
	}else{
		/* Systemのセグメント */
		switch(value & 0x7){
			case 0x6:
				return SEG_TYPE_INTRGATE;
			case 0x2:
				return SEG_TYPE_LDT;
			case 0x7:
				return SEG_TYPE_TRAPGATE;
			case 0x5:
				return SEG_TYPE_TSS;
		}
	}
	
	return SEG_TYPE_UNKNOW;
	
}

seg_type check_guest_segment_type(DWORD selector){
	
	segment_entry		entry;
	
	entry = cpu_emu_get_guest_seg_value(selector);
	
	return check_segment_type(&entry);
	
}


static gdtr_table *check_v_gdtr_value(){
	return &v_gdtr;
}

int gen_emu_level(int trust_level){
	
	return level_table[trust_level];
	
}

int get_xchg_level(int emu_level){
	
	BYTE	table[] = {
		/* 1 2 2 3 */
		0, 0, 1, 3
	};
	
	return table[emu_level];
	
}


void cpu_init_segment_emu(){
	
	DWORD		*addr = (DWORD*)(VIRTUAL_MEMSIZE_ADDR);
	
	vm_sys_gdtr = (gdtr_table*)GDTR_ADDR;
	/* baseは物理アドレス!" */
	vm_sys_gdts = (segment_entry*)(vm_sys_gdtr->base - (*addr));
	
	//_sys_memset32(vm_intrrupt_table, 0, sizeof(vm_intrrupt_table));
	
	return;
}


WORD cpu_emu_get_vm_segment_value(int segment_reg){
	
	return v_segments[segment_reg].vm_sys_seg;
	
}


segment_entry cpu_emu_get_guest_seg_value(WORD selector){
	
	segment_entry		entry;
	segment_entry		*target = (segment_entry*)(v_gdtr.base + (selector >> 3));
	
	_sys_physic_mem_read((void*)&entry, target, sizeof(segment_entry));
	
	return entry;
	
}

void cpu_emu_write_vm_segment(DWORD index, segment_entry *entry){
	
	vm_sys_gdts[index] = *entry;
	
	return;
	
}


/* vmのシステムセグメントのdplを求める */
int cpu_emu_get_vm_seg_dpl(DWORD selector){
	
	DWORD	index = selector >> 3;
	int		dpl;
	
	dpl = (vm_sys_gdts[index].typeL >> 5) & 0x3;
	
	return dpl;
	
}

int cpu_emu_get_segment_dpl(segment_entry *entry){
	
	return ((entry->typeL >> 5) & 0x3);
	
}


DWORD cpu_emu_get_seg_base(int segment_reg){
	
	int		index	= segment_reg + VM_GDT_ENTRY_ADD_BASE;
	DWORD	addr 	= 0x0;
	
	addr = (vm_sys_gdts[index].baseH << 24)
			| (vm_sys_gdts[index].baseM << 16)
			| vm_sys_gdts[index].baseL;
			
	return addr;
	
}


WORD vm_get_guset_selector(int segment_type){
	
	return (v_segments[segment_type].guset_seg);
	
}



/*
static void cpu_emu_write_gdtr(DWORD addr){
	
	v_gdtr = (gdtr_table*)addr;
	v_gdts = v_gdtr->base;
	
	return;
	
}


static void cpu_emu_write_idtr(DWORD addr){
	
	v_idtr = (idtr_table*)addr;	
	v_idts = v_idtr->base;
	
	return;
}
*/



/* csはやっぱり特殊だからこうなるよねぇ */
void cpu_emu_write_cs(WORD cs_value, DWORD addr, WORD *ret_cs, DWORD *new_eip){
	
	/* TI=1のときは考慮してないよ(LDTの場合) */
	
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	BYTE				tmp;
	segment_entry		dest;
	seg_type			type;
	
	if((cs_value >> 2) & 0x1){
		/* これはLDTです */
		*ret_cs		= cs_value;
		*new_eip	= addr;
		return;
	}
	
	index_num = cs_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したCSの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, (void*)index_src, sizeof(segment_entry));
	
	type = check_segment_type(&dest);
	
	if(type!=SEG_TYPE_GD){
		/* どうやらシステムセグメントのようです */
		switch(type){
			case SEG_TYPE_INTRGATE:
				break;
			case SEG_TYPE_LDT:
				break;
			case SEG_TYPE_TRAPGATE:
				break;
			case SEG_TYPE_TSS:
				break;
			default:
				break;
		}
	}
	
	tmp = (dest.typeL >> 1) & 0x7;
	
	/* csにロードできないものをロードしようとしていないか？ */
	if((index_num==0x0) || ((tmp > 4) || (tmp > 7))){
		/* これはcsレジスタにはロードできないので例外発生 */
		//_sys_printf("invalied segment data\n");
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_CS] = dest;
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_CS].guset_seg 	= cs_value;
	v_segments[CPUSEG_REG_CS].vm_sys_seg 	= (VM_GDT_ENTRY_V_CS << 3) | rpl;
	
	/* csの値を更新します */
	*ret_cs 	= v_segments[CPUSEG_REG_CS].vm_sys_seg;
	*new_eip 	= addr;
	
	return;
}



static void cpu_emu_write_es(DWORD es_value, vm_emu_datas *status){
	
	/* TI=1のときは考慮してないよ(LDTの場合) */
	
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	segment_entry		dest;
	
	if((es_value >> 2) & 0x1){
		status->regs->es = es_value;
		return;
	}
	
	index_num = es_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したESの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, index_src, sizeof(segment_entry));
	
	/* esにロードできないものをロードしようとしていないか？ */
	if(((dest.typeL >> 1) & 0x7) > 3){
		/* これはロードできないので例外発生 */
		
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_ES]	= dest;
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_ES].guset_seg		= es_value;
	v_segments[CPUSEG_REG_ES].vm_sys_seg	= (VM_GDT_ENTRY_V_ES << 3) | rpl;
	
	/* esの値を更新します */
	//*es = v_segments[CPUSEG_REG_ES].vm_sys_seg;
	status->regs->es = v_segments[CPUSEG_REG_ES].vm_sys_seg;
	
	return;
	
}


static void cpu_emu_write_gs(DWORD gs_value, vm_emu_datas *status){
	
	/* TI=1のときは考慮してないよ(LDTの場合) */
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	segment_entry		dest;
	
	if((gs_value >> 2) & 0x1){
		LOAD_VALUE_GS(gs_value);
		return;
	}
	
	index_num = gs_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したESの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, index_src, sizeof(segment_entry));
	
	/* gsにロードできないものをロードしようとしていないか？ */
	if(((dest.typeL >> 1) & 0x7) > 3){
		/* これはロードできないので例外発生 */
		
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_GS]	= dest;
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_GS].guset_seg		= gs_value;
	v_segments[CPUSEG_REG_GS].vm_sys_seg	= (VM_GDT_ENTRY_V_GS << 3) | rpl;
	
	/* esの値を更新します */
	//*es = v_segments[CPUSEG_REG_ES].vm_sys_seg;
	//status->regs-> = v_segments[CPUSEG_REG_DS].vm_sys_seg;
	asm volatile ( 
		"movw	%%ax, %%gs"
	: /* no output */ : "a" (v_segments[CPUSEG_REG_GS].vm_sys_seg));
	
	return;
	
}


static void cpu_emu_write_fs(DWORD fs_value, vm_emu_datas *status){
	
	/* TI=1のときは考慮してないよ(LDTの場合) */
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	segment_entry		dest;
	
	if((fs_value >> 2) & 0x1){
		LOAD_VALUE_FS(fs_value);
		return;
	}
	
	index_num = fs_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したESの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, index_src, sizeof(segment_entry));
	
	/* fsにロードできないものをロードしようとしていないか？ */
	if(((dest.typeL >> 1) & 0x7) > 3){
		/* これはロードできないので例外発生 */
		
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_FS]	= dest;
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_FS].guset_seg		= fs_value;
	v_segments[CPUSEG_REG_FS].vm_sys_seg	= (VM_GDT_ENTRY_V_FS << 3) | rpl;
	
	/* esの値を更新します */
	//*es = v_segments[CPUSEG_REG_ES].vm_sys_seg;
	//status->regs-> = v_segments[CPUSEG_REG_DS].vm_sys_seg;
	asm volatile ( 
		"movw	%%ax, %%fs"
	: /* no output */ : "a" (v_segments[CPUSEG_REG_FS].vm_sys_seg));
	
	return;
	
}


static void cpu_emu_write_ds(DWORD ds_value, vm_emu_datas *status){
	
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	segment_entry		dest;
	
	if((ds_value >> 2) & 0x1){
		/* TI=1のときは考慮してないよ(LDTの場合) */
		status->regs->ds = ds_value;
		return;
	}
	
	index_num = ds_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したESの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, (void*)index_src, sizeof(segment_entry));
	
	/* dsにロードできないものをロードしようとしていないか？ */
	if(((dest.typeL >> 1) & 0x7) > 3){
		/* これはロードできないので例外発生 */
		
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	//_sys_printf("ds: new_rpl: %x\n", rpl);
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_DS]	= dest;
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_DS].guset_seg		= ds_value;
	v_segments[CPUSEG_REG_DS].vm_sys_seg	= (VM_GDT_ENTRY_V_DS << 3) | rpl;
	
	/*
	_sys_printf("ds\n");
	_sys_printf("new_ds_value: %x\n", v_segments[CPUSEG_REG_DS].vm_sys_seg);
	_sys_printf("dw: %x\n", dest.limitL);
	_sys_printf("dw: %x\n", dest.baseL);
	_sys_printf("db: %x\n", dest.baseM);
	_sys_printf("db: %x\n", dest.typeL);
	_sys_printf("db: %x\n", dest.limitH_typeH);
	_sys_printf("db: %x\n", dest.baseH);
	*/
	//__STOP();
	
	/* dsの値を更新します */
	status->regs->ds = v_segments[CPUSEG_REG_DS].vm_sys_seg;
	
	//__STOP();
	
	return;
	
}

WORD cpu_emu_write_ss_value(DWORD ss_value){
	
	DWORD				index_num;
	BYTE				dpl, rpl;
	void				*index_src;
	segment_entry		dest;
	
	/* TI=1のときは考慮してないよ(LDTの場合) */
	if((ss_value >> 2) & 0x1){
		/* TI=1のときは考慮してないよ(LDTの場合) */
		return ss_value;
	}
	
	index_num = ss_value >> 3;
	index_src = (void*)(v_gdtr.base + index_num * sizeof(segment_entry));
	
	/* 物理アドレス空間から、GuestOSが選択したESの値に対応するインデックスをコピーする */
	_sys_physic_mem_read((void*)&dest, index_src, sizeof(segment_entry));
	
	/* ssにロードできないものをロードしようとしていないか？ */
	if((index_num==0x0) || (dest.typeL!=1 && dest.typeL!=3)){
		/* これはロードできないので例外発生 */
		
	}
	
	/* 特権レベルどうするか... */
	dpl = (dest.typeL >> 5) & 0x3;
	rpl = dpl = level_table[dpl];
	
	/* dplを新たに設定しなおす */
	dest.typeL = (dest.typeL & 0x9f) | (dpl << 5);
	
	/* vm側のセグメントに設定する */
	vm_sys_gdts[VM_GDT_ENTRY_V_SS] 	= dest;

/*
	_sys_printf("ss\n");
	_sys_printf("dw: %x\n", dest.limitL);
	_sys_printf("dw: %x\n", dest.baseL);
	_sys_printf("db: %x\n", dest.baseM);
	_sys_printf("db: %x\n", dest.typeL);
	_sys_printf("db: %x\n", dest.limitH_typeH);
	_sys_printf("db: %x\n", dest.baseH);
*/
	
	/* vmのセグメントの値を設定 */
	v_segments[CPUSEG_REG_SS].guset_seg		= ss_value;
	v_segments[CPUSEG_REG_SS].vm_sys_seg	= (VM_GDT_ENTRY_V_SS << 3) | rpl;
	
	//_sys_printf("new_ss_value: %x\n", v_segments[CPUSEG_REG_SS].vm_sys_seg);
	
	/* ssの値を更新します */
	return (v_segments[CPUSEG_REG_SS].vm_sys_seg);
	
}


WORD cpu_emu_make_ss(DWORD base_addr, DWORD limit, BYTE dpl){
	
	segment_entry	dest;
	
	make_gdt_value(base_addr, limit, SEG_FLAG_S | SEG_FLAG_P | GDT_SEG_TYPE_3, 0x0, dpl, &dest);
	
	//vm_sys_gdts[VM_GDT_ENTRY_V_SS] = dest;
	write_raw_vm_segment(VM_GDT_ENTRY_V_SS, &dest);
	
	return (VM_GDT_ENTRY_V_SS << 3) | dpl;
}


static void cpu_emu_write_ss(DWORD ss_value, vm_emu_datas *status){
	
	/* ssの値を更新します */
	status->stack.sys_stack->ss = cpu_emu_write_ss_value(ss_value);
	
	return;
	
}


/* far jmpをエミュレートする */
void emu_far_jmp(vm_emu_datas *status){
	
	WORD		segment, ss_segment, new_cs;
	DWORD		addr, new_eip;
	
	if(get_vm_mode_status()){
		
		/* protect_mode */
		if(status->addr_prefix){
			addr 	= get_userseg_word(status->code_addr);	/* 2byte */
			segment	= get_userseg_word(status->code_addr);	/* 2byte */
		}else{
			addr 	= get_userseg_dword(status->code_addr);	/* 4byte */
			segment	= get_userseg_word(status->code_addr);	/* 2byte */
			
		}
		
		/*
		_sys_cls();
		_sys_printf("far jmp:\n %x:%x\n", segment, addr);
		*/
		
		/* すでにprotect modeに移行している */
		cpu_emu_write_cs(segment, addr, &new_cs, &new_eip);
		
		status->stack.sys_stack->cs		= new_cs;
		status->stack.sys_stack->eip 	= new_eip;
		
	}else{
		
		/* real_mode */
		if(status->addr_prefix){
			addr	= get_userseg_dword(status->code_addr);
			segment = get_userseg_word(status->code_addr);
			
			//_sys_printf("far jump!\n");
			//_sys_printf("jmp %x:%x\n", segment, addr);
		
		}else{
			addr	= get_userseg_word(status->code_addr);
			segment	= get_userseg_word(status->code_addr);
		}
		
		/* 32bitモードに移行する */
		change_vm_mode_status();
		
		/* エミュ用のcsを用意する */
		cpu_emu_write_cs(segment, addr, &new_cs, &new_eip);
		
		/* 適当なSSぶっこんでおかないと動かないもんね */
		/* gusetOSに期待するのではなく、vm側で一個予備を用意しておいたほうがいい */
		ss_segment = cpu_emu_make_ss(status->stack.sys_stack->ss, 0xffff, new_cs & 0x3);
		
		/*
		_sys_printf(
			"new_eip=%x, new_cs=%x, eflags=%x\n"
			"esp=%x, ss=%x\n"
		, new_eip, new_cs, status->stack.sys_stack->eflags, status->stack.sys_stack->esp, status->stack.sys_stack->ss);
		*/
		
		status->stack.sys_stack->cs		= new_cs;
		status->stack.sys_stack->eip	= new_eip;
		
		//status->stack.sys_stack->eflags	&= 0xfffd7fd7;
		status->stack.sys_stack->ss		= ss_segment;
		
	}
	
	asm volatile (
		"subl		%%eax, %%ecx	\n"
		"movl		%%ecx, %%esp	\n"
		"jmp		%%ebx			\n"
	::
	"c" (_TSS_BASE_ESP),
	"a" (_STACK_SIZE),
	"b" (&_general_iret));
	
}


void cpu_emu_lidt(BYTE modr_m, vm_emu_datas *status){
	
	DWORD			addr;
	
	if(get_vm_mode_status()){
		/* protect_mode */
		
		if(status->opcode_prefix){
			
			addr = func_table[bit16[(modr_m & 0xf) - 0xC]](status) & 0xffff;
			
			if(modr_m==0x56){
				get_userseg_byte(status->code_addr);
			}
			
		}else{
			
			if(modr_m==0x15){
				addr = get_userseg_dword(status->code_addr);
			}else{
				//_sys_printf("hello\n");
				addr = func_table[bit32[(modr_m & 0xf) - 8]](status);
				//__STOP();
			}
			
		}
		
		//アドレスが抜けている
		
	}else{
		/* real_mode */
		if(status->opcode_prefix){
			
			addr = func_table[bit32[(modr_m & 0xf) - 8]](status);
			
			if(modr_m==0x55){
				get_userseg_byte(status->code_addr);
			}
			
		}else{
			if(modr_m==0x16){
				addr = get_userseg_word(status->code_addr);
			}else{
				addr = func_table[bit16[(modr_m & 0xf) - 0xC]](status) & 0xffff;
				
				if(modr_m==0x56){
					get_userseg_byte(status->code_addr);
				}
				
			}
		}
		
		/* 物理アドレス算出 */
		addr = GET_PHYSIC_ADDR(status->stack.real_mode->ds, addr);
		
	}
	
	regist_v_idtr(addr);
	
}


void cpu_emu_lgdt(BYTE modr_m, vm_emu_datas *status){
	
	DWORD			addr;
	
	if(get_vm_mode_status()){
		/* protect_mode */
		
		if(status->opcode_prefix){
			
			addr = func_table[bit16[(modr_m & 0xf) - 4]](status) & 0xffff;
			
			if(modr_m==0x56){
				get_userseg_byte(status->code_addr);
			}
			
		}else{
			
			if(modr_m==0x15){
				addr = get_userseg_dword(status->code_addr);
			}else{
				addr = func_table[bit32[modr_m & 0xf]](status);
			}
			
		}
		
	}else{
		/* real_mode */
		
		if(status->opcode_prefix){
			
			addr = func_table[bit32[modr_m & 0xf]](status);
			
			if(modr_m==0x55){
				get_userseg_byte(status->code_addr);
			}
			
		}else{
			if(modr_m==0x16){
				addr = get_userseg_word(status->code_addr);
			}else{
				addr = func_table[bit16[(modr_m & 0xf) - 4]](status) & 0xffff;
				
				if(modr_m==0x56){
					get_userseg_byte(status->code_addr);
				}
				
			}
		}
		
		/* 物理アドレス算出 */
		addr = GET_PHYSIC_ADDR(status->stack.real_mode->ds, addr);
		
	}
	
	//_sys_printf("\nlgdt [%x]\n", addr);
	
	/* 物理アドレスから引っ張ってくる */
	_sys_physic_mem_read((void*)&v_gdtr, (void*)addr, sizeof(gdtr_table));

	//_sys_printf("gdtr limit: %x\n", v_gdtr.limit);
	//_sys_printf("gdtr base: %x\n", v_gdtr.base);
	
}


void cpu_emu_lgdt_or_lidt(vm_emu_datas *status){
	
	BYTE	modr_m 		= get_userseg_byte(status->code_addr);
	BYTE	modr_m_high = (modr_m >> 4) & 0xf;
	BYTE	modr_m_low 	= modr_m & 0xf;
	
	switch(modr_m_high){
		case 0xf:
		case 0x3:
		case 0x7:
			emu_lmsw(modr_m, status);
			break;
			
		case 0x1:
		case 0x5:
			if(modr_m_low >= 8){
				cpu_emu_lidt(modr_m, status);
			}else{
				cpu_emu_lgdt(modr_m, status);
			}
			break;
	}
	
}

void cpu_emu_ltr_or_lldt(vm_emu_datas *status){
	
	BYTE	modr_m = get_userseg_byte(status->code_addr);
	
	if((modr_m & 0xf) >= 8){
		cpu_emu_ltr(modr_m, status);
	}else{
		cpu_emu_lldt(modr_m, status);
	}
	
}


void cpu_emu_mov_sw_ew(vm_emu_datas *status){
	
	BYTE	modr_m 	= get_userseg_byte(status->code_addr);
	BYTE	tmp 	= modr_m & 0xf;
	WORD	value;
	
	tmp &= 0xf;
	
	switch(modr_m >> 4){
		case 0xC:
			if(tmp >= 8){
				tmp -= 8;
				value = func_table[bit32[tmp]](status);
				
				cpu_emu_write_cs(
					value, 
					status->stack.sys_stack->eip,
					(WORD*)&status->stack.sys_stack->cs,
					&status->stack.sys_stack->eip
				);
				
			}else{
				value = func_table[bit32[tmp]](status);
				cpu_emu_write_es(value, status);
			}
			
			break;
		case 0xD:
		
			if(tmp >= 8){
				
				tmp -= 8;
				
				value = func_table[bit32[tmp]](status);
				cpu_emu_write_ds(value, status);
				
			}else{
				value = func_table[bit32[tmp]](status);
				
				cpu_emu_write_ss(value, status);
				
			}
			
			break;
		case 0xE:
			/* gs, fsレジスタに書き込み */
			
			if(tmp >= 8){
				
				tmp -= 8;
				
				value = func_table[bit32[tmp]](status);
				cpu_emu_write_gs(value, status);
				
			}else{
				
				value = func_table[bit32[tmp]](status);
				cpu_emu_write_fs(value, status);
				
			}
			
			
			break;
	}
	
}


void write_raw_vm_segment(DWORD segment_entry_num, segment_entry *info){
	
	//_sys_printf("addr: %x %x\n", (DWORD)vm_sys_gdts, segment_entry_num);
	
	vm_sys_gdts[segment_entry_num] = (*info);
	
}


void cpu_emu_pop_cs(vm_emu_datas *status){
	
	
}


void cpu_emu_pop_ds(vm_emu_datas *status){
	
	DWORD		old_esp = status->stack.sys_stack->esp;
	DWORD		pushed_value;
	
	//old_esp += 4;
	
	pushed_value = get_user_dword_value(status->stack.sys_stack->ss, old_esp);
	
	//_sys_printf("push ds(%x)\n", pushed_value);
	
	cpu_emu_write_ds(pushed_value, status);
	
	return;
	
}


void cpu_emu_pop_ss(vm_emu_datas *status){
	
	DWORD		old_esp = status->stack.sys_stack->esp;
	DWORD		pushed_value;
	
	//old_esp += 4;
	
	pushed_value = get_user_dword_value(status->stack.sys_stack->ss, old_esp);
	
	cpu_emu_write_ss(pushed_value, status);
	
	return;
	
}


void cpu_emu_pop_es(vm_emu_datas *status){
	
	DWORD		old_esp = status->stack.sys_stack->esp;
	DWORD		pushed_value;
	
	//old_esp += 4;
	
	pushed_value = get_user_dword_value(status->stack.sys_stack->ss, old_esp);
	
	cpu_emu_write_es(pushed_value, status);
	
	return;
	
}


void cpu_emu_pop_fs(vm_emu_datas *status){
	
	DWORD		old_esp = status->stack.sys_stack->esp;
	DWORD		pushed_value;
	
	//old_esp += 4;
	
	pushed_value = get_user_dword_value(status->stack.sys_stack->ss, old_esp);
	
	cpu_emu_write_fs(pushed_value, status);
	
	return;
	
}


void cpu_emu_pop_gs(vm_emu_datas *status){
	
	DWORD		old_esp = status->stack.sys_stack->esp;
	DWORD		pushed_value;
	
	//old_esp += 4;
	
	pushed_value = get_user_dword_value(status->stack.sys_stack->ss, old_esp);
	
	cpu_emu_write_gs(pushed_value, status);
	
	return;
	
}


void cpu_emu_lss(vm_emu_datas *status){
	
	BYTE	next_code = get_userseg_byte(status->code_addr);
	WORD	segment;
	
	//_sys_printf("lss\n");
	
	segment = emu_lxs_func[next_code >> 4](status, next_code & 0xf);
	
	cpu_emu_write_ss(segment, status);
	
}


void cpu_emu_lds(vm_emu_datas *status){
	
	BYTE	next_code = get_userseg_byte(status->code_addr);
	WORD	segment;
	
	segment = emu_lxs_func[next_code >> 4](status, next_code & 0xf);
	
	cpu_emu_write_ds(segment, status);
	
}


void cpu_emu_les(vm_emu_datas *status){
	
	BYTE	next_code = get_userseg_byte(status->code_addr);
	WORD	segment;
	
	segment = emu_lxs_func[next_code >> 4](status, next_code & 0xf);
	
	cpu_emu_write_es(segment, status);
	
}


void cpu_emu_lfs(vm_emu_datas *status){
	
	BYTE	next_code = get_userseg_byte(status->code_addr);
	WORD	segment;
	
	segment = emu_lxs_func[next_code >> 4](status, next_code & 0xf);
	
	cpu_emu_write_fs(segment, status);
	
}


void cpu_emu_lgs(vm_emu_datas *status){
	
	BYTE	next_code = get_userseg_byte(status->code_addr);
	WORD	segment;
	
	segment = emu_lxs_func[next_code >> 4](status, next_code & 0xf);
	
	cpu_emu_write_gs(segment, status);
	
}


static WORD cpu_emu_lxs_high_0x0(vm_emu_datas *status, BYTE low_code){
	
	DWORD			value;
	WORD			first_byte, second_byte;
	
	if(low_code >= 0x8){
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code - 0x8]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
			status->regs->ecx &= 0xffff0000;
			status->regs->ecx |= first_byte;
		}else{
			status->regs->ecx = first_byte;
		}
		
	}else{
		
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
			status->regs->eax &= 0xffff0000;
			status->regs->eax |= first_byte;
		}else{
			status->regs->eax = first_byte;
		}
		
	}
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x1(vm_emu_datas *status, BYTE low_code){
	
	DWORD			value;
	WORD			first_byte, second_byte;
	
	if(low_code >= 0x8){
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code - 0x8]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
		status->regs->ebx &= 0xffff0000;
			status->regs->ebx |= first_byte;
		}else{
			status->regs->ebx = first_byte;
		}
		
	}else{
		
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
			status->regs->edx &= 0xffff0000;
			status->regs->edx |= first_byte;
		}else{
			status->regs->edx = first_byte;
		}
		
	}
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x2(vm_emu_datas *status, BYTE low_code){
	
	DWORD			addr;
	WORD			second_byte;
		
	if(low_code >= 0x8){
		addr = func_table[bit32[low_code - 0x8]](status);
		cpu_emu_lxs_common(status, &status->regs->ebp, addr);
	}else{
		if(status->opcode_prefix){
			
			switch(low_code){
				case 0x4:
					addr = status->regs->esi;
					break;
				case 0x5:
					addr = status->regs->edi;
					break;
				case 0x7:
					addr = status->regs->ebx;
					break;
			}
			
			addr &= 0xffff;
			
		}else{
			addr = func_table[bit32[low_code]](status);
		}
		
		//_sys_printf("addr: %x\n", addr);
		
		//_sys_cls();
		
		second_byte = cpu_emu_lxs_common(status, &(status->stack.sys_stack->esp), addr);
		
	}
	
	//_sys_printf("lss %x, %x\n", status->stack.sys_stack->esp, second_byte);
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x3(vm_emu_datas *status, BYTE low_code){
	
	DWORD			value;
	WORD			first_byte, second_byte;
	
	if(low_code >= 0x8){
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code - 0x8]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
			status->regs->edi &= 0xffff0000;
			status->regs->edi |= first_byte;
		}else{
			status->regs->edi = first_byte;
		}
		
	}else{
		
		value = get_user_dword_value(status->regs->ds, func_table[bit32[low_code]](status));
		
		first_byte = (WORD)(value & 0xffff);
		second_byte = (WORD)(value >> 16);
		
		if(status->addr_prefix){
			status->regs->esi &= 0xffff0000;
			status->regs->esi |= first_byte;
		}else{
			status->regs->esi = first_byte;
		}
		
	}
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x4(vm_emu_datas *status, BYTE low_code){
	
	DWORD			tmp 		= get_user_dword_value(status->regs->ds, status->regs->ebp);
	WORD			first_byte 	= tmp;
	WORD			second_byte = tmp >> 16;
	
	switch(low_code){
		
		case 0xD:
			
			if(status->addr_prefix){
				status->regs->ecx &= 0xffff0000;
				status->regs->ecx |= first_byte;
			}else{
				status->regs->ecx = first_byte;
			}
			
			break;
			
		case 0x5:
			
			if(status->addr_prefix){
				status->regs->eax &= 0xffff0000;
				status->regs->eax |= first_byte;
			}else{
				status->regs->eax = first_byte;
			}
			break;
			
	}
	
	/* 一回空読み */
	get_userseg_byte(status->code_addr);
	
	return second_byte;
	
}

static WORD cpu_emu_lxs_high_0x5(vm_emu_datas *status, BYTE low_code){
	
	DWORD			tmp 		= get_user_dword_value(status->regs->ds, status->regs->ebp);
	WORD			first_byte 	= tmp;
	WORD			second_byte = tmp >> 16;
	
	switch(low_code){
		
		case 0xD:
			
			if(status->addr_prefix){
				status->regs->ebx &= 0xffff0000;
				status->regs->ebx |= first_byte;
			}else{
				status->regs->ebx = first_byte;
			}
			
			break;
			
		case 0x5:
			
			if(status->addr_prefix){
				status->regs->edx &= 0xffff0000;
				status->regs->edx |= first_byte;
			}else{
				status->regs->edx = first_byte;
			}
			
			break;
			
	}
	
	/* 一回空読み */
	get_userseg_byte(status->code_addr);
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x6(vm_emu_datas *status, BYTE low_code){
	
	DWORD			tmp 		= get_user_dword_value(status->regs->ds, status->regs->ebp);
	WORD			first_byte 	= tmp;
	WORD			second_byte = tmp >> 16;
	
	if(low_code!=0xD){
		return 0;
	}
	
	if(status->addr_prefix){
		status->regs->ebp &= 0xffff0000;
		status->regs->ebp |= first_byte;
	}else{
		status->regs->ebp = first_byte;
	}
	
	/* 一回空読み */
	get_userseg_byte(status->code_addr);
	
	return second_byte;
	
}


static WORD cpu_emu_lxs_high_0x7(vm_emu_datas *status, BYTE low_code){
	
	DWORD			tmp 		= get_user_dword_value(status->regs->ds, status->regs->ebp);
	WORD			first_byte 	= tmp;
	WORD			second_byte = tmp >> 16;
	
	switch(low_code){
		
		case 0xD:
			
			if(status->addr_prefix){
				status->regs->edi &= 0xffff0000;
				status->regs->edi |= first_byte;
			}else{
				status->regs->edi = first_byte;
			}
			
			break;
			
		case 0x5:
			
			if(status->addr_prefix){
				status->regs->esi &= 0xffff0000;
				status->regs->esi |= first_byte;
			}else{
				status->regs->esi = first_byte;
			}
			
			break;
			
	}
	
	/* 一回空読み */
	get_userseg_byte(status->code_addr);
	
	return second_byte;
	
}

static DWORD cpu_emu_lxs_high_0x9(vm_emu_datas *status, BYTE low_code){
	
	return 0;
	
}


static WORD cpu_emu_lxs_common(vm_emu_datas *status, DWORD *first_oprand, DWORD addr){
	
	DWORD		first_word, addr_mask;
	WORD		segment_value;
	
	if(get_vm_mode_status()){
		
		/* protect mode */
		if(status->addr_prefix){
			//first_op_mask = 0xffff;
			first_word = get_user_word_value(status->regs->ds, addr);
			addr += 2;
			
			(*first_oprand) &= 0xffff0000;
			(*first_oprand) |= (WORD)first_word;
			
		}else{
			
			first_word = get_user_dword_value(status->regs->ds, addr);
			addr += 4;
			
			(*first_oprand) = first_word;
			
		}
		
	}else{
		
		/* real mode */
		if(status->addr_prefix){
			
			first_word = get_user_dword_value(status->regs->ds, addr);
			addr += 4;
			
			(*first_oprand) = first_word;
		}else{
			
			first_word = get_user_word_value(status->regs->ds, addr);
			addr += 2;
			
			(*first_oprand) &= 0xffff0000;
			(*first_oprand) |= (WORD)first_word;
			
		}
		
	}
	
	/*
	_sys_printf("second byte: %x\n", addr);
	_sys_printf("ds: %x\n", status->regs->ds);
	*/
	
	return get_user_word_value(status->regs->ds, addr);
	
}


static void gdtr_null_load(){
	
	//_sys_printf("cs load expection!!\n");
	__STOP();
	
}
