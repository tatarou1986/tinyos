
#ifndef stackH
#define stackH

#include	"kernel.h"


/* 特権レベルが変わる際のスタック */
typedef struct _intr_stack {
	DWORD		eip;
	DWORD		cs;
	DWORD		eflags;
	DWORD		esp;
	DWORD		ss;
} __attribute__ ((packed)) intr_stack;

/* 特権レベルが変わらない際のスタック */
typedef struct _irq_stack {
	DWORD		eip;
	DWORD		cs;
	DWORD		eflags;
} __attribute__ ((packed)) irq_stack;


typedef struct __v8086_intr_stacks {
	DWORD			eip;
	DWORD			cs;
	DWORD			eflags;
	DWORD			esp;
	DWORD			ss;
	DWORD			es;
	DWORD			ds;
	DWORD			fs;
	DWORD			gs;
} __attribute__ ((packed)) v8086_intr_stack;


typedef struct __common_stacks {
	DWORD			eip;
	DWORD			cs;
	DWORD			eflags;
	DWORD			esp;
	DWORD			ss;
} __attribute__ ((packed)) common_stacks;


typedef struct _pushed_regs {
	DWORD			ebx;
	DWORD			ecx;
	DWORD			edx;
	DWORD			esi;
	DWORD			edi;
	DWORD			ebp;
	DWORD			eax;
	DWORD			ds;
	DWORD			es;
} __attribute__ ((packed)) pushed_regs;

/*
typedef union _irq_stack {
	intr_stack				*chg_level_stack;
	irq_stack				*common_stack;
	_v8086_intr_stacks		*v8086_stack;
	
} irq_stack;
*/

typedef irq_stack	basic_stack;

#endif
