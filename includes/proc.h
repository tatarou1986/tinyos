
#ifndef		__PROC_H
#define		__PROC_H

#include	"kernel.h"

typedef struct _general_context {
	DWORD		ebx;
	DWORD		ecx;
	DWORD		edx;
	DWORD		esi;
	DWORD		edi;
	DWORD		ebp;
	DWORD		eax;
} general_context;

typedef struct _segment_context {
	DWORD		es;
	DWORD		fs;
	DWORD		gs;
} segment_context;

typedef struct _task_struct {
	general_context		regs;
	segment_context		segs;
	DWORD				eip;
	DWORD				esp;
	DWORD				eflags;
} task_struct;

#endif

