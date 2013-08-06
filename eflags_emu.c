
#include		"eflags_emu.h"
#include		"gdebug.h"
#include		"system.h"

#define			DEFAULT_EFLAGS		0x0;

static DWORD		mask_value = 0x0;

void trap_eflags(){
	
	_sys_printf("trap\n");
	__STOP();
	
}


