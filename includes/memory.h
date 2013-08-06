
#ifndef memoryH
#define	memoryH

#include		"kernel.h"

/*
	ビッグエンディアンならリトルエンディアン
	リトルエンディアンならビッグエンディアン
	にそれぞれ変換する
	WORD単位なので注意！
*/
WORD *change_endian(WORD *data, int count);

void _sys_memset16(void *dest, const char src, WORD count);

void _sys_memset32(void *dest, const char src, unsigned int count);

void *_sys_memcpy(void *dest, const void *src, int n);

void *_sys_physic_mem_write(void *dest, const void *src, int n);

void *_sys_physic_mem_read(void *dest, const void *src, int n);

#endif

