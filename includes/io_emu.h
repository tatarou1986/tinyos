
#ifndef io_emuH
#define io_emuH

#include	"kernel.h"
#include	"stack.h"

void io_emu_in_AL_lb(vm_emu_datas *status);
void io_emu_in_eAX_lb(vm_emu_datas *status);

void io_emu_in_AL_DX(vm_emu_datas *status);
void io_emu_in_eAX_DX(vm_emu_datas *status);

void io_emu_out_lb_AL(vm_emu_datas *status);
void io_emu_out_lb_eAX(vm_emu_datas *status);

void io_emu_out_DX_AL(vm_emu_datas *status);
void io_emu_out_DX_eAX(vm_emu_datas *status);

void io_emu_outsb(vm_emu_datas *status);
void io_emu_outsw_outsd(vm_emu_datas *status);

#endif

