
#include		"kernel.h"

#define		PAGE_PS_FLAG	0x80


void cpu_init_page_emu();

void cpu_emu_flush_tlb(DWORD page_dir_base_addr);

void write_to_vm_page_entry(int page_num, DWORD value);

void change_PSE_mode();

