
#ifndef __ASM_I386_H__
#define __ASM_I386_H__

#include "kernel.h"
#include "gdt.h"
#include "BIOSemu.h"

#define			E820_MAX				32
#define			E820MAP_ENTRY_ADDR		0x92
#define			INT_11h_VALUE			0x90

#define			E820_RAM				1
#define			E820_RESERVED			2
#define			E820_ACPI				3
#define			E820_NVS				4

#define			E820_SMAP				0x534d4150

typedef struct __e820map {
	DWORD	 		length;
	_e820entry		entry[E820_MAX];
} __attribute__ ((packed)) _e820map;

#define			P_E820_ENTRY			((_e820map*)E820MAP_ENTRY_ADDR)
#define			P_INT_11H_VALUE			((WORD*)VIRTUAL_MEMSIZE_ADDR)

#endif
