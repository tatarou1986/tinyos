
#ifndef tss_emuH
#define tss_emuH


#include		"kernel.h"
#include		"CPUemu.h"


void cpu_emu_ltr(BYTE modr_m, vm_emu_datas *status);

void cpu_emu_clts(vm_emu_datas *status);

void cpu_emu_do_clts();

void cpu_emu_get_esp_ss_from_tss(int level, WORD *ss, DWORD *esp);

#endif

