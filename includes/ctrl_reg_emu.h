

#include		"kernel.h"
#include		"stack.h"
#include		"CPUemu.h"

#define			PE_FLAG			0x1
#define			MP_FLAG			(0x1 << 1)
#define			EM_FLAG			(0x1 << 2)
#define			TS_FLAG			(0x1 << 3)
#define			ET_FLAG			(0x1 << 4)
#define			NE_FLAG			(0x1 << 5)

#define			WP_FLAG			(0x1 << 16)
#define			AM_FLAG			(0x1 << 18)
#define			NW_FLAG			(0x1 << 29)
#define			CD_FLAG			(0x1 << 30)
#define			PG_FLAG			(0x1 << 31)

/* 0x22 */
void emu_mov_crn_reg(vm_emu_datas *status);

/* 0x20 */
void emu_mov_reg_crn(vm_emu_datas *status);

void emu_lmsw(BYTE modr_m, vm_emu_datas *status);

void vm_set_flag_crn(int reg_num, int bit_num);

void vm_clear_flag_crn(int reg_num, int bit_num);

void cpu_init_ctrlreg_emu();

