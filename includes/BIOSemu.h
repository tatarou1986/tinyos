
#ifndef			BIOSemuH
#define			BIOSemuH

#include		"kernel.h"

typedef struct __int_15h_87h_desc {
	DWORD			bios_1;
	DWORD			bios_2;
	WORD			double_cx_1;
	DWORD			src_physics_addr;
	WORD			zero_1;
	WORD			double_cx_2;
	DWORD			dest_physics_addr;
	WORD			zero_2;
	DWORD			bios_3;
	DWORD			bios_4;
} __attribute__ ((packed)) int_15h_87h_desc;


typedef struct __e820entry {
	DWORD			base_addr_low;
	DWORD			base_addr_high;
	DWORD			length_low;
	DWORD			length_high;
	DWORD			type;
} __attribute__ ((packed)) _e820entry;


#endif

