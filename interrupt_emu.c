
#include		"interrupt_emu.h"
#include		"gdt.h"
#include		"memory.h"
#include		"cpuseg_emu.h"
#include		"system.h"
#include		"gdebug.h"
#include		"stack.h"
#include		"tss_emu.h"
#include		"idt.h"


static int check_cs_dpl(WORD cs_value);
static void vm_fatal_error(int intr_num);

idtr_table		v_idtr;
volatile  	int				vm_if_flag = 0;

/* 割り込み許可 */
void cpu_emu_sti(vm_emu_datas *status){
	
	vm_if_flag = 1;
	/* sti(); */
	
}

/* 割り込み禁止 */
void cpu_emu_cli(vm_emu_datas *status){
	
	vm_if_flag = 0;
	/* cli(); */
	
}


void regist_v_idtr(DWORD addr){
	
	_sys_physic_mem_read((void*)&v_idtr, (void*)addr, sizeof(idtr_table));
	
}


void get_guest_idt_handler(DWORD vect_num, WORD *cs_value, DWORD *eip_value){
	
	segment_entry		entry;
	segment_entry		*addr = (segment_entry*)v_idtr.base;
	
	addr += vect_num;
	
	/*
	if(vect_num==0x80){
		asm volatile (
			"movl	%0, %%eax	\n"
		:: "g"(v_idtr.base));
		
		__STOP();
	}
	*/
	
	_sys_physic_mem_read(&entry, addr, sizeof(segment_entry));
	
	*cs_value	= entry.baseL;
	*eip_value	= (entry.baseH << 24) | (entry.limitH_typeH << 16) | entry.limitL;
	
}


void jmp_v_idtr(pushed_regs *regs, intr_stack *stack, DWORD idt_num, int use_err_code, DWORD error_code){
	
	segment_entry 		tmp, idt_seg;
	DWORD				offset_addr;
	WORD				segment;
	int					dpl, cpl;
	WORD				raw_stack_cs, raw_stack_ss;
	WORD				new_ss, emued_ss;
	DWORD				new_esp;
	DWORD				stack_pointer;
	DWORD				push_eflags_value;
	
	/* _sys_printf("oh shit!!\n"); */
	
	if(vm_if_flag==0){
		if(stack->eip < 0x108ea0){
			return;
		}
	}
	
	get_guest_idt_handler(idt_num, &segment, &offset_addr);
	
	/*
		この時点でstack.csはVMがエミュレーションした仮想セグメントを指しています
		一方IDTのセグメントは、GusetOSが設定したものなので、VMは介入していません
	*/
	idt_seg = cpu_emu_get_guest_seg_value(segment);
	
	/* 現在のCSとSSの値を退避 */
	raw_stack_cs = vm_get_guset_selector(CPUSEG_REG_CS);
	raw_stack_ss = vm_get_guset_selector(CPUSEG_REG_SS);
	
	tmp = cpu_emu_get_guest_seg_value(raw_stack_cs);
	
	cpl = cpu_emu_get_segment_dpl(&tmp);
	dpl = cpu_emu_get_segment_dpl(&idt_seg);
	
/*
	このスタックはGuestOSが期待したとおりの値をつむこと！
*/
	push_eflags_value = stack->eflags;
	
	if(vm_if_flag){
		/* 割り込み許可 */
		push_eflags_value |= 1 << 9;
	}else{
		/* 割り込み禁止 */
		push_eflags_value &= ~(1 << 9);
	}
	
	if(cpl > dpl){
		/* 特権移行あり,許可される */
		
		/* TSSからssとespを引っ張ってくる */
		cpu_emu_get_esp_ss_from_tss(dpl, &new_ss, &new_esp);
		
		/* ssエミュ */
		emued_ss = cpu_emu_write_ss_value(new_ss);
		
		/* csエミュ */
		cpu_emu_write_cs(segment, offset_addr, &segment, &offset_addr);
		
		if(error_code){
			
			asm volatile (
				"	pushw		%%fs					\n"
				"	movl		%%ebx, %%fs				\n"
				"	subl		$0x18, %%eax			\n"
				"	movl		%%ecx, %%fs:(%%eax)		\n"
				"	movl		%%edx, %%fs:4(%%eax)	\n"
				"	movl		%%esi, %%fs:8(%%eax)	\n"
				"	movl		%%eax, %%edi			\n"
			/*	"	movl		%%edi, %%fs:12(%%eax)	\n
				"	movl		%0, %%fs:16(%%eax)		\n"
				"	movl		%1, %%fs:20(%%eax)		\n"
			*/
			: "=D" (stack_pointer) :
			"a" (new_esp),
			"b" ((DWORD)emued_ss),
			"c" (error_code),
			"d" (stack->eip),
			"S" ((DWORD)raw_stack_cs)
			/* "D" (stack->eflags), 		*/
			/* "g" (stack->esp), 			*/
			/* "g" ((DWORD)raw_stack_ss) 	*/
			);
			
			asm volatile (
				"	movl		%%eax, %%fs:12(%%edi)	\n"
				"	movl		%%ecx, %%fs:16(%%edi)	\n"
				"	movl		%%ebx, %%fs:20(%%edi)	\n"
				"	popw		%%fs					\n"
			::
			"D" (stack_pointer),
		//	"a" (stack->eflags),
			"a" (push_eflags_value),
			"c" (stack->esp),
			"b" ((DWORD)raw_stack_ss)
			);
			
			/* gusetのidtルーチンへ飛ぶ */
			stack->esp	= new_esp + 0x18;
		
		}else{
			/* エラーコードなし */
			
			asm volatile (
				"	pushw		%%fs					\n"
				"	movl		%%ebx, %%fs				\n"
				"	subl		$0x14, %%eax			\n"
				"	movl		%%ecx, %%fs:(%%eax)		\n"
				"	movl		%%edx, %%fs:4(%%eax)	\n"
				"	movl		%%esi, %%fs:8(%%eax)	\n"
				"	movl		%%eax, %%edi			\n"
			/*	"	movl		%%edi, %%fs:12(%%eax)		\n"
				"	movl		%0, %%fs:16(%%eax)		\n"
			*/
			: "=D" (stack_pointer) :
			"a" (new_esp),
			"b" ((DWORD)emued_ss),
			"c" (stack->eip),
			"d" ((DWORD)raw_stack_cs),
			"S" (push_eflags_value)
			/*
			"D" (stack->esp),
			"g" ((DWORD)raw_stack_ss)
			*/
			);
			
			asm volatile (
				"	movl		%%eax, %%fs:12(%%edi)	\n"
				"	movl		%%ecx, %%fs:16(%%edi)	\n"
				"	popw		%%fs					\n"
			::
			"D" (stack_pointer),
			"a" ((DWORD)raw_stack_cs),
			/* "c" (stack->eflags) */
			"c" (push_eflags_value)
			);
			
			stack->esp	= new_esp + 0x14;
			
		}
		
		stack->ss		= emued_ss;
		stack->cs		= segment;
		stack->eip		= offset_addr;
		
		//stack->eflags	|= (vm_if_flag & 0x1) << 9;
		
	}else if(cpl==dpl){
		
		/* 特権移行無し,許可される */
		/* _sys_printf("sgement: %x, offset_addr: %x\n", segment, offset_addr); */
		
		cpu_emu_write_cs(segment, offset_addr, &segment, &offset_addr);
		
		if(use_err_code){
			
			/* エラーコードあり */
			asm volatile (
				"		pushw		%%fs									\n"
				"		movl		%%ebx, %%fs								\n"
				"	user_error_code:										\n"
				"		subl		$0x10, %%eax							\n"
				"		movl		%%ecx, %%fs:(%%eax)		##error_code	\n"
				"		movl		%%edx, %%fs:4(%%eax) 	##eip			\n"
				"		movl		%%edi, %%fs:8(%%eax) 		##cs			\n"
				"		movl		%%esi, %%fs:12(%%eax) 		##eflags		\n"
				"		popw		%%fs									\n"
			::
			"a" (stack->esp),
			"b" (stack->ss),
			"c" (error_code),
			"d" (stack->eip),
			"D" ((DWORD)raw_stack_cs),	/* これはエミュレートがかかってない生のセレクタ値 */
			/* "S" (stack->eflags) */
			"S" (push_eflags_value)
			: "memory" );
			
			stack->esp -= 0x10;
			
		}else{
			
			/* エラーコード無し */
			if(idt_num==0x80){
				/*
				_sys_cls();
				_sys_cls();
				_sys_printf("0x80!\n");
				__HLT();
				*/
				
				//stack->eflags &= ~(0x1 << 9);
				
				/* stack->eflags |= (0x1 << 9); */
				vm_if_flag = 1;
				push_eflags_value |= 1 << 9;
			}
			
			asm volatile (
				"pushw		%%fs								\n"
				"movl		%%ebx, %%fs							\n"
				"subl		$0xc, %%eax							\n"
				"movl		%%ecx, %%fs:(%%eax) 	##eip		\n"
				"movl		%%edx, %%fs:4(%%eax) 	##cs		\n"
				"movl		%%edi, %%fs:8(%%eax) 	##eflags	\n"
				"popw		%%fs								\n"
			::
			"a" (stack->esp),
			"b" (stack->ss),
			"c" (stack->eip),
			"d" (raw_stack_cs),
			/* "D" (stack->eflags) */
			"D" (push_eflags_value)
			: "memory" );
			
			stack->esp -= 0xc;
			
		}
		
		/* idtのルーチンに飛ぶ */
		stack->eip		= offset_addr;
		stack->cs		= segment;
		
	}else{
		/* これは許可されない、一般保護例外発生 */
		
		/* error_codeを適切に設定しておかないと... */
		jmp_v_idtr(regs, stack, 13, 1, error_code);
	}
	
}

void cpu_emu_iret(vm_emu_datas *status){
	
	int					cpl, dpl;
	WORD				new_cs;
	DWORD				stack_ss, stack_esp;
	basic_stack			stack;
	intr_stack			chg_level_stack;
	void				*stack_addr;
	segment_entry		entry;
	
	if(status->stack.sys_stack->cs == _KERNEL_CS){
		_sys_freeze("vm fatal error!\n");
	}
	
	stack_addr = (void*)(cpu_emu_get_seg_base(CPUSEG_REG_SS) + status->stack.sys_stack->esp);
	_sys_physic_mem_read(&stack, stack_addr, sizeof(basic_stack));
	
	/* 現行特権レベル */
	cpl = get_xchg_level(cpu_emu_get_vm_seg_dpl(status->stack.sys_stack->cs));
	entry = cpu_emu_get_guest_seg_value(stack.cs);
	dpl = cpu_emu_get_segment_dpl(&entry);
	
	//_sys_printf("eip: %x, cs: %x\n", stack.eip, stack.cs);
	
	/* エミュレーションをかける */
	cpu_emu_write_cs(stack.cs, stack.eip, &new_cs, &stack.eip);
	stack.cs = new_cs;
	
	/* 書き戻し */
	_sys_physic_mem_write(stack_addr, &stack, sizeof(basic_stack));
	
	if(cpl > dpl){
		
		_sys_physic_mem_read(&chg_level_stack, stack_addr, sizeof(intr_stack));
		chg_level_stack.ss = cpu_emu_write_ss_value(chg_level_stack.ss);
		
		//_sys_printf("fuck you bitch\n");
		
		/* もう一度書き戻し */
		_sys_physic_mem_write(stack_addr, &chg_level_stack, sizeof(intr_stack));
		
	}else if(cpl < dpl){
		/* これは許可されない */
		_sys_freeze("guset iret error\n");
	}
	
	_sys_physic_mem_read(&stack, stack_addr, sizeof(basic_stack));
	
	/* Gusetにもう一度, iretを実行させる */
	--(*status->code_addr);
	
}


static int check_cs_dpl(WORD cs_value){
	
	DWORD		dpl = cpu_emu_get_vm_seg_dpl(cs_value);
	DWORD		rpl = cs_value >> 3;
	
	return (dpl & rpl);
	
}



static void vm_fatal_error(int intr_num){
	
	_sys_printf("vm fatal error!!\n");
	_sys_printf("called interrupt handler %d\n", intr_num);
	
	for(;;){}
	
}


/* 0 */
void _sys_wrap_divide_error_fault(pushed_regs regs, intr_stack stack){
	
	if(check_cs_dpl(stack.cs)==0){
		/* VMの中でエラー発生! */
		vm_fatal_error(0);
	}
	
	jmp_v_idtr(&regs, &stack, 0, 0, 0);
	
}

/* 1 */
void _sys_wrap_debug_fault(pushed_regs regs, intr_stack stack){
	
	if(check_cs_dpl(stack.cs)==0){
		vm_fatal_error(0);
	}
	
	jmp_v_idtr(&regs, &stack, 1, 0, 0);
	
}

/* 2 */
void _sys_wrap_nmi_fault(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 2, 0, 0);
	
}

/* 3 */
void _sys_wrap_break_point(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 3, 0, 0);
	
}

/* 4 */
void _sys_wrap_overflow_fault(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 4, 0, 0);
	
}

/* 5 */
void _sys_wrap_bound_fault(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 5, 0, 0);
	
}

/* 6 */
void _sys_wrap_invalid_opcode(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 6, 0, 0);
	
}

/* 7 */
void _sys_wrap_device_not_available(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 7, 0, 0);
	
}

/* 8 */
void _sys_wrap_double_fault(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 8, 1, error_code);
	
}

/* 9 */
void _sys_wrap_cop_seg_overflow(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 9, 0, 0);
	
}

/* 10 */
void _sys_wrap_invalid_tss(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 10, 1, error_code);
	
}

/* 11 */
void _sys_wrap_seg_not_present(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 11, 1, error_code);
	
}

/* 12 */
void _sys_wrap_stack_exception(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 12, 1, error_code);
	
}

/* 13 */
/* void _sys_wrap_general_protection(pushed_regs regs, DWORD error_code, intr_stack stack){ */


void _sys_redirect_general_protect(vm_emu_datas *status){
	
	DWORD	error_code = 0x0;
	
	jmp_v_idtr(status->regs, status->stack.protect_mode, 13, 1, error_code);
	
}

/* 14 */
void _sys_wrap_page_fault(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 14, 1, error_code);
	
}

/* 16 */
void _sys_wrap_floating_point_error(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 16, 0, 0);
	
}

/* 17 */
void _sys_wrap_alignment_check_error(pushed_regs regs, DWORD error_code, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 17, 1, error_code);
	
}

/* 18 */
void _sys_wrap_machine_check_error(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 18, 0, 0);
	
}

/* 19 */
void _sys_wrap_simd_float_error(pushed_regs regs, intr_stack stack){
	
	jmp_v_idtr(&regs, &stack, 19, 0, 0);
	
}


void _sys_wrap_ignore_int(pushed_regs regs, DWORD idt_num, intr_stack stack){
	
	/* Guestへリダイレクション */
	jmp_v_idtr(&regs, &stack, idt_num, 0, 0);
	
}



