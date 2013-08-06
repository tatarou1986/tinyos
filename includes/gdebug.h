#ifndef gdebugH
#define gdebugH

#include		"kernel.h"
#include		"stack.h"

#define		__DEBUG(a, b, c)	\
		asm volatile (	"pushl	%%eax		\n"		\
						"pushl	%%ebx		\n"		\
						"pushl	%%ecx		\n"		\
						"movl	%0, %%eax	\n"		\
						"movl	%1, %%ebx	\n"		\
						"movl	%2, %%ecx	\n"		\
						"hlt				\n"		\
						"nop				\n"		\
						"nop				\n"		\
						"popl	%%ecx		\n"		\
						"popl	%%ebx		\n"		\
						"popl	%%eax		\n"		\
						: /* no output */ : "m"(a), "m"(b), "m"(c) );
						
#define		__DEBUG_B(a)		\
		asm volatile (	"pushl	%%eax			\n"		\
						"xorl	%%eax, %%eax	\n"		\
						"movl	%0, %%eax		\n"		\
						"hlt					\n"		\
						"nop					\n"		\
						"popl	%%eax			\n"		\
						: /* no output */ : "m"(a) );
						

#define		__HLT()		asm volatile ( "hlt" : /* no output */ : /* no input */);
#define		__NOP()		asm volatile ( "nop" : /* no output */ : /* no input */);


#define		__STOP()	for(;;){}


/* ì¡å†ÉåÉxÉãà⁄çsÇ»Çµ! */
typedef struct _dbg_regs{
	DWORD			ebx;
	DWORD			ecx;
	DWORD			edx;
	DWORD			esi;
	DWORD			edi;
	DWORD			ebp;
	DWORD			eax;
	WORD			ds;
	WORD			es;
	WORD			ss;
	DWORD			esp;
	DWORD			eflags;
	WORD			cs;
	DWORD			eip;
} __attribute__ ((packed)) dbg_regs;


void _sys_dump_cpu(pushed_regs regs);

void _sys_pushed_cpu(pushed_regs *regs);

int test_trap();

#endif
