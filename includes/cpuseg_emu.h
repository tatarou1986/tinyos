#ifndef cpuseg_emuH
#define cpuseg_emuH


#include		"kernel.h"
#include		"stack.h"
#include		"CPUemu.h"
#include		"gdt.h"

#define		CPUSEG_REG_CS			0x0
#define		CPUSEG_REG_DS			0x1
#define		CPUSEG_REG_ES			0x2
#define		CPUSEG_REG_SS			0x3
#define		CPUSEG_REG_FS			0x4
#define		CPUSEG_REG_GS			0x5

/*
#define		VM_GDT_ENTRY_V_CS		8
#define		VM_GDT_ENTRY_V_DS		9
#define		VM_GDT_ENTRY_V_ES		10
#define		VM_GDT_ENTRY_V_SS		11
#define		VM_GDT_ENTRY_V_FS		12
#define		VM_GDT_ENTRY_V_GS		13
*/

#define		VM_GDT_ENTRY_V_CS		13
#define		VM_GDT_ENTRY_V_DS		14
#define		VM_GDT_ENTRY_V_ES		15
#define		VM_GDT_ENTRY_V_SS		16
#define		VM_GDT_ENTRY_V_FS		17
#define		VM_GDT_ENTRY_V_GS		18

#define		VM_GDT_ENTRY_ADD_BASE	13


#define		LOAD_VALUE_CS(x)			asm volatile ( "movw %%ax, %%cs" :: "a" (x))
#define		LOAD_VALUE_DS(x)			asm volatile ( "movw %%ax, %%ds" :: "a" (x))
#define		LOAD_VALUE_SS(x)			asm volatile ( "movw %%ax, %%ss" :: "a" (x))
#define		LOAD_VALUE_ES(x)			asm volatile ( "movw %%ax, %%es" :: "a" (x))
#define		LOAD_VALUE_FS(x)			asm volatile ( "movw %%ax, %%fs" :: "a" (x))
#define		LOAD_VALUE_GS(x)			asm volatile ( "movw %%ax, %%gs" :: "a" (x))


typedef enum {
	SEG_TYPE_GD = 0,
	SEG_TYPE_TSS,
	SEG_TYPE_LDT,
	SEG_TYPE_INTRGATE,
	SEG_TYPE_TRAPGATE,
	SEG_TYPE_UNKNOW
} seg_type;


void emu_far_jmp(vm_emu_datas *status);

/* ç≈èâÇ…èâä˙âªÇµÇƒ */
void cpu_init_segment_emu();

int cpu_emu_get_vm_seg_dpl(DWORD);

WORD cpu_emu_get_vm_segment_value(int segment_reg);

DWORD cpu_emu_get_seg_base(int segment_reg);

void cpu_emu_cli(vm_emu_datas *status);

void cpu_emu_sti(vm_emu_datas *status);


void cpu_emu_lidt(BYTE modr_m, vm_emu_datas *status);

void cpu_emu_lgdt(BYTE modr_m, vm_emu_datas *status);

void cpu_emu_lgdt_or_lidt(vm_emu_datas *status);

void cpu_emu_mov_sw_ew(vm_emu_datas *status);


void cpu_emu_pop_cs(vm_emu_datas *status);

void cpu_emu_pop_ds(vm_emu_datas *status);

void cpu_emu_pop_ss(vm_emu_datas *status);

void cpu_emu_pop_es(vm_emu_datas *status);

void cpu_emu_pop_fs(vm_emu_datas *status);

void cpu_emu_pop_gs(vm_emu_datas *status);


void cpu_emu_lss(vm_emu_datas *status);

void cpu_emu_lds(vm_emu_datas *status);

void cpu_emu_les(vm_emu_datas *status);

void cpu_emu_lfs(vm_emu_datas *status);

void cpu_emu_lgs(vm_emu_datas *status);


void write_raw_vm_segment(DWORD segment_entry_num, segment_entry *info);

WORD cpu_emu_make_ss(DWORD base_addr, DWORD limit, BYTE dpl);

WORD cpu_emu_write_ss_value(DWORD ss_value);

void cpu_emu_write_cs(WORD cs_value, DWORD addr, WORD *ret_cs, DWORD *new_eip);

segment_entry cpu_emu_get_guest_seg_value(WORD selector);

int gen_emu_level(int trust_level);

int get_xchg_level(int emu_level);

WORD vm_get_guset_selector(int segment_type);

int cpu_emu_get_segment_dpl(segment_entry *entry);

void cpu_emu_write_vm_segment(DWORD index, segment_entry *entry);


seg_type check_segment_type(segment_entry *entry);

seg_type check_guest_segment_type(DWORD selector);

#endif

