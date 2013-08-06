
#include "memory.h"

void _sys_memset16(void *dest, const char src, WORD count)
{
	asm volatile (
		"pushf				\n"
		"cld				\n"
		"movb	%0, %%al	\n"
		"movw	%1, %%cx	\n"
		"movl	%2, %%edi	\n"
		"rep				\n"
		"stosb				\n"
		"popf				\n"
	: /* no output */ : "m"(src), "m"(count), "m"(dest) : "%al", "%cx", "%edi");
}

void _sys_memset32(void *dest, const char src, unsigned int count)
{
	asm volatile (
		"pushf				\n"
		"cld				\n"
		"movb	%0, %%al	\n"
		"movl	%1, %%ecx	\n"
		"movl	%2, %%edi	\n"
		"rep				\n"
		"stosb				\n"
		"popf				\n"
	: /* no output */ : "m"(src), "m"(count), "m"(dest) : "%al", "%ecx", "%edi");
}

/*
 * ビッグエンディアンならリトルエンディアン
 * リトルエンディアンならビッグエンディアン
 *にそれぞれ変換する
 */
WORD *change_endian(WORD *data, int count)
{
	WORD			tmp;
	int				i = 0;
			
	for ( ; i < count ; i++) {
		tmp = *data;
		*data = ((tmp >> 8) & 0xff) | ((tmp << 8) & 0xff00);
		data++;
	}
	
	return data;
}


void *_sys_memcpy(void *dest, const void *src, int n)
{
	asm volatile (
		"pushf		\n"
		"cld		\n"
		"rep		\n"
		"movsb		\n"
		"popf		\n"
	: /* no output */ : "c" ((unsigned int)n), "S" (src), "D" (dest));
				  
	return dest;
}

/*
 * セグメントにかかわらず、物理アドレスにデータをコピーする。
 */

/*
void *_sys_physic_memcpy(void *dest, const void *src, int n){
	
	WORD	data_segment = _USER_DS;
	
	asm volatile (
		"pushf							\n"
		"pushw		%%es				\n"
		"movw		%0, %%es			\n"
		"cld							\n"
		"rep							\n"
		"movsw							\n"
		"popw		%%es				\n"
		"popf							\n"
	::
	"m" (data_segment),
	"c" ((unsigned int)(n / 2)),
	"D" (dest),
	"S" (src)
	);
	
	return dest;
	
}

*/

void *_sys_physic_mem_write(void *dest, const void *src, int n)
{
	WORD	data_segment = _USER_DS;
	
	asm volatile (
		"pushf							\n"
		"pushw		%%es				\n"
		"movw		%0, %%es			\n"
		"cld							\n"
		"rep							\n"
		"movsb							\n"
		"popw		%%es				\n"
		"popf							\n"
	::
	"m" (data_segment),
	"c" ((unsigned int)n),
	"D" (dest),
	"S" (src)
	);
	
	return dest;
	
}

void *_sys_physic_mem_read(void *dest, const void *src, int n)
{
	WORD	user_segment 	= _USER_DS;
	WORD	kernel_segment 	= _KERNEL_DS;
	
	asm volatile (
		"pushf					\n"
		"pushw		%%ds		\n"
		"pushw		%%es		\n"
		"movw		%0, %%ds	\n"
		"movw		%1, %%es	\n"
		"cld					\n"
		"rep					\n"
		"movsb					\n"
		"popw		%%es		\n"
		"popw		%%ds		\n"
		"popf					\n"
	::
	"m" (user_segment),
	"m" (kernel_segment),
	"c" ((unsigned int)n),
	"D" (dest),
	"S" (src)
	);
	
	return dest;
	
}


