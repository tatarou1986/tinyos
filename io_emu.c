
#include		"system.h"
#include		"CPUemu.h"
#include		"io_emu.h"
#include		"device.h"
#include		"resource.h"
#include		"io.h"
#include		"memory.h"
#include		"gdebug.h"


static char *reg_name[] = {
		"al",
		"ax",
		"eax"
};

static void io_emu_redirect_in_AL_lb(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_in_eax_addr(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_in_ax_addr(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_in_AL_DX(vm_emu_datas *status);
static void io_emu_redirect_in_eax_dx(vm_emu_datas *status);
static void io_emu_redirect_in_ax_dx(vm_emu_datas *status);

static void io_emu_redirect_out_lb_AL(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_out_addr_eax(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_out_addr_ax(vm_emu_datas *status, WORD io_addr);
static void io_emu_redirect_out_DX_AL(vm_emu_datas *status);
static void io_emu_redirect_out_dx_ax(vm_emu_datas *status);
static void io_emu_redirect_out_dx_eax(vm_emu_datas *status);

static void io_emu_redirect_outsb(vm_emu_datas *status, DWORD dest_physic_addr);
static void io_emu_redirect_outsw(vm_emu_datas *status, DWORD dest_physic_addr);
static void io_emu_redirect_outsd(vm_emu_datas *status, DWORD dest_physic_addr);


void vm_init_io_emu(){
	
	
	
}


void io_emu_in_AL_lb(vm_emu_datas *status){
	
	WORD					addr;
	emulation_handler		*handler;
	
	addr = get_userseg_byte(status->code_addr);
	
	//_sys_printf("in al, %x\n", addr);
	
	handler = search_resource(addr);
	
	if(handler!=NULL){
		if(handler->io_read_byte!=NULL){
			status->regs->eax &= 0xffffff00;
			status->regs->eax |= handler->io_read_byte();
			return;
		}
	}
	
	/* リダイレクトする */
	io_emu_redirect_in_AL_lb(status, addr);
	
}


void io_emu_in_eAX_lb(vm_emu_datas *status){
	
	BYTE				addr 		= get_userseg_byte(status->code_addr);
	DWORD				flag 		= (DWORD)get_vm_mode_status();
	emulation_handler	*handler	= search_resource(addr);
	
	switch(status->addr_prefix | flag){
		case 0x66:
		case 0x1:
			/* eax */
			/* _sys_printf("in %s %x\n", reg_name[2], oprand); */
			if(handler!=NULL){
				if(handler->io_read_dword!=NULL){
					status->regs->eax = handler->io_read_dword();
					break;
				}
			}
			
			io_emu_redirect_in_eax_addr(status, addr);
			
			break;
		case 0x0:
		case 0x67:
			/* ax */
			/* _sys_printf("in %s %x\n", reg_name[1], oprand); */
			if(handler!=NULL){
				if(handler->io_read_word!=NULL){
					status->regs->eax &= 0xffff0000;
					status->regs->eax |= handler->io_read_word();
					break;
				}
			}
			
			io_emu_redirect_in_ax_addr(status, addr);
			
			break;
	}
	
}

void io_emu_out_lb_AL(vm_emu_datas *status){
	
	WORD				addr 		= get_userseg_byte(status->code_addr);
	emulation_handler	*handler	= search_resource(addr);
	
	/* _sys_printf("out %x, al\n", oprand); */
	
	if(handler!=NULL){
		if(handler->io_write_byte!=NULL){
			handler->io_write_byte((BYTE)(status->regs->eax & 0xff));
			return;
		}
	}
	
	io_emu_redirect_out_lb_AL(status, addr);
	
}


void io_emu_out_lb_eAX(vm_emu_datas *status){
	
	BYTE				addr 		= get_userseg_byte(status->code_addr);
	DWORD				flag 		= (DWORD)get_vm_mode_status();
	emulation_handler	*handler	= search_resource(addr);
	
	switch(status->addr_prefix | flag){
		case 0x66:
		case 0x1:
			/* eax */
			/* _sys_printf("out %x %s\n", oprand, reg_name[2]); */
			if(handler!=NULL){
				if(handler->io_write_dword!=NULL){
					handler->io_write_dword(status->regs->eax);
					break;
				}
			}
			
			io_emu_redirect_out_addr_eax(status, addr);
			
			break;
		case 0x0:
		case 0x67:
			/* ax */
			/* _sys_printf("out %x %s\n", oprand, reg_name[1]); */
			if(handler!=NULL){
				if(handler->io_write_word!=NULL){
					handler->io_write_word((WORD)(status->regs->eax & 0xffff));
				}
			}
			
			io_emu_redirect_out_addr_ax(status, addr);
			
			break;
	}
	
}


void io_emu_in_AL_DX(vm_emu_datas *status){
	
	emulation_handler	*handler	= search_resource((WORD)(status->regs->edx & 0xffff));
	
	if(handler!=NULL){
		if(handler->io_read_byte!=NULL){
			status->regs->eax &= 0xffffff00;
			status->regs->eax |= handler->io_read_byte();
			return;
		}
	}
	
	io_emu_redirect_in_AL_DX(status);
	
}


void io_emu_in_eAX_DX(vm_emu_datas *status){
	
	DWORD				flag 		= (DWORD)get_vm_mode_status();
	emulation_handler	*handler	= search_resource((WORD)(status->regs->edx & 0xffff));
	
	
	switch(status->addr_prefix | flag){
		case 0x66:
		case 0x1:
			/* eax */
			/* _sys_printf("in %s dx\n", reg_name[2]); */
			if(handler!=NULL){
				if(handler->io_read_dword!=NULL){
					status->regs->eax = handler->io_read_dword();
					break;
				}
			}
			
			io_emu_redirect_in_eax_dx(status);
			break;
			
		case 0x0:
		case 0x67:
			/* ax */
			/* _sys_printf("in %s dx\n", reg_name[1]); */
			if(handler!=NULL){
				if(handler->io_read_word!=NULL){
					status->regs->eax &= 0xffff0000;
					status->regs->eax |= handler->io_read_word();
					break;
				}
			}
			
			io_emu_redirect_in_ax_dx(status);
			
			break;
	}
	
}


void io_emu_out_DX_AL(vm_emu_datas *status){
	
	emulation_handler		*handler;
	
	if((handler = search_resource(status->regs->edx & 0xffff))!=NULL){
		if(handler->io_write_byte!=NULL){
			handler->io_write_byte((BYTE)(status->regs->eax & 0xff));
			return;
		}
	}
	
	io_emu_redirect_out_DX_AL(status);
	
}


void io_emu_out_DX_eAX(vm_emu_datas *status){
	
	int						flag		= get_vm_mode_status();
	emulation_handler		*handler	= search_resource(status->regs->edx & 0xffff);
	
	switch(status->addr_prefix | flag){
		case 0x66:
		case 0x1:
			/* eax */
			/* _sys_printf("out dx, eax\n"); */
			if(handler!=NULL){
				if(handler->io_read_dword!=NULL){
					status->regs->eax = handler->io_read_dword();
					break;
				}
			}
			io_emu_redirect_out_dx_eax(status);
			break;
		case 0x0:
		case 0x67:
			/* ax */
			/* _sys_printf("out dx, ax\n"); */
			if(handler!=NULL){
				if(handler->io_read_word!=NULL){
					status->regs->eax &= 0xffff0000;
					status->regs->eax |= handler->io_read_word();
					break;
				}
			}
			io_emu_redirect_out_dx_ax(status);
			
			break;
	}
	
}


void io_emu_outsb(vm_emu_datas *status){
	
	DWORD	flag	= get_vm_mode_status();
	void	*addr;
	DWORD	esi_value;
	WORD	dx_value;
	
	if(flag){
		/* protect_mode */
		esi_value = status->regs->esi;
	}else{
		/* real_mode */
		esi_value = status->regs->esi & 0xffff;
		
	}
	
	addr 		= (void*)((status->regs->ds << 4) + esi_value);
	dx_value 	= status->regs->edx & 0xffff;
	
	_sys_printf("outsb (dx:%x, addr:%x)\n", dx_value, addr);
	
}


void io_emu_outsw_outsd(vm_emu_datas *status){
	
	DWORD	flag	= get_vm_mode_status();
	void	*addr;
	DWORD	esi_value;
	WORD	dx_value = status->regs->edx & 0xffff;
	
	if(flag){
		/* protect_mode */
		
		esi_value = status->regs->esi;
		
		if(status->addr_prefix){
			/* outsw */
			_sys_printf("outsb");
		}else{
			/* outsd */
			_sys_printf("outsd");
		}
		
	}else{
		/* real_mode */
		esi_value = status->regs->esi & 0xffff;
		
		if(status->addr_prefix){
			/* outsd */
			_sys_printf("outsd");
		}else{
			/* outsw */
			_sys_printf("outsw");
		}
		
	}
	
}


static void io_emu_redirect_in_AL_lb(vm_emu_datas *status, WORD io_addr){
	
	BYTE	ret = inb(io_addr);
	
	status->regs->eax &= 0xffffff00;
	status->regs->eax |= ret;
	
	return;
}


static void io_emu_redirect_in_eax_addr(vm_emu_datas *status, WORD io_addr){
	
	status->regs->eax = inl(io_addr);
	
	return;
}

static void io_emu_redirect_in_ax_addr(vm_emu_datas *status, WORD io_addr){
	
	WORD	ret = inw(io_addr);
	
	status->regs->eax &= 0xffff0000;
	status->regs->eax |= ret;
	
	return;
	
}

static void io_emu_redirect_in_AL_DX(vm_emu_datas *status){
	
	WORD	io_addr = status->regs->edx & 0xffff;
	BYTE	ret		= inb(io_addr);
	
	status->regs->eax &= 0xffffff00;
	status->regs->eax |= ret;
	
	return;
	
}


static void io_emu_redirect_in_eax_dx(vm_emu_datas *status){
	
	status->regs->eax = inl(status->regs->edx & 0xffff);
	
	return;
}


static void io_emu_redirect_in_ax_dx(vm_emu_datas *status){
	
	WORD	io_addr = status->regs->edx & 0xffff;
	
	status->regs->eax &= 0xffff0000;
	status->regs->eax |= inw(io_addr);
	
	return;
	
}


static void io_emu_redirect_out_lb_AL(vm_emu_datas *status, WORD io_addr){
	
	BYTE	value	= status->regs->eax & 0xff;
	
	outb(io_addr, value);
	
}


static void io_emu_redirect_out_addr_eax(vm_emu_datas *status, WORD io_addr){
	
	outl(io_addr, status->regs->eax);
	
}


static void io_emu_redirect_out_addr_ax(vm_emu_datas *status, WORD io_addr){
	
	outw(io_addr, status->regs->eax & 0xffff);
	
}



static void io_emu_redirect_out_DX_AL(vm_emu_datas *status){
	
	WORD	io_addr = (WORD)(status->regs->edx & 0xffff);
	BYTE	value	= (BYTE)(status->regs->eax & 0xff);
	
	outb(io_addr, value);
	
}


static void io_emu_redirect_out_dx_ax(vm_emu_datas *status){
	
	WORD	io_addr = status->regs->edx & 0xffff;
	WORD	value	= status->regs->eax & 0xffff;
	
	outw(io_addr, value);
	
}


static void io_emu_redirect_out_dx_eax(vm_emu_datas *status){
	
	WORD	io_addr = status->regs->edx & 0xffff;
	
	outl(io_addr, status->regs->eax);
	
}

static void io_emu_redirect_outsb(vm_emu_datas *status, DWORD dest_physic_addr){
	
	BYTE	value;
	WORD	io_addr = status->regs->edx & 0xffff;
	
	_sys_physic_mem_read(&value, (void*)dest_physic_addr, sizeof(BYTE));
	
	outb(io_addr, value);
	
	++(status->regs->esi);
	
}


static void io_emu_redirect_outsw(vm_emu_datas *status, DWORD dest_physic_addr){
	
	WORD	value;
	WORD	io_addr = status->regs->edx & 0xffff;
	
	_sys_physic_mem_read(&value, (void*)dest_physic_addr, sizeof(WORD));
	
	(status->regs->esi) += 2;
	
}


static void io_emu_redirect_outsd(vm_emu_datas *status, DWORD dest_physic_addr){
	
	DWORD	value;
	WORD	io_addr = status->regs->edx & 0xffff;
	
	_sys_physic_mem_read(&value, (void*)dest_physic_addr, sizeof(DWORD));
	
	(status->regs->esi) += 4;
	
}


