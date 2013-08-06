

/* debug.h */


#define		__DEBUG(a, b, c)								\
		asm volatile (										\
			"pushl	%%eax			\n"						\
			"pushl	%%ebx			\n"						\
			"pushl	%%ecx			\n"						\
			"xorl	%%eax, %%eax	\n"						\
			"xorl	%%ebx, %%ebx	\n"						\
			"xorl	%%ecx, %%ecx	\n"						\
			"movl	%0, %%eax		\n"						\
			"movl	%1, %%ebx		\n"						\
			"movl	%2, %%ecx		\n"						\
			"hlt					\n"						\
			"nop					\n"						\
			"popl	%%ecx			\n"						\
			"popl	%%ebx			\n"						\
			"popl	%%eax			\n"						\
		: /* no output */ : "m"(a), "m"(b), "m"(c) );		\
		
						
#define		__DEBUG_B(a)					\
		asm volatile (						\
			"pushl	%%eax			\n"		\
			"xorl	%%eax, %%eax	\n"		\
			"movb	%0, %%al		\n"		\
			"hlt					\n"		\
			"nop					\n"		\
			"popl	%%eax			\n"		\
		: /* no output */ : "m"(a) );		\
		

#define		__HLT()		asm volatile ( "hlt" : /* no output */ : /* no input */);

#define		__NOP()		asm volatile ( "nop" : /* no output */ : /* no input */);

