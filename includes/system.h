
#ifndef systemH
#define	systemH

#include		"kernel.h"

typedef char*	va_list;


#define 		__va_argsiz(t) 													\
				(((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))	\

#define 		va_start(ap, pN) 												\
				((ap) = ((va_list)(&pN) + __va_argsiz(pN)))						\
	
#define			va_arg(ap,type)							\
				((type*)(ap+=sizeof(type)))[-1]			\

#define			va_end(ap)				((void)0x0)


//本体はdumpcpu.Sにある
//extern _sys_pushall(void);
void _sys_dumpcpu(void);

//%dと%xと%sしかサポートしとらん！
void _sys_printf(const char *format, ...);

int _sys_atoi(char *str);

char *strstr(const char *searchee, const char *lookfor);

void _sys_reverse(char *s);

int _sys_strlen(char *str);

int _sys_bin2hex(BYTE *buffer, int buffersize, char *output_str);

void _sys_print_bin2hex(BYTE *buffer, int buffersize);

char* _sys_ltona(long value, char *str, int n, int base);

void _sys_put_signed_Int(int n, unsigned int base, int large);

void _sys_put_unsigned_Int(unsigned int n, unsigned int base, int large);

int _sys_tolower(int c);

int _sys_toupper(int c);

void _sys_freeze(const char *msg);

BYTE hex2bin(char digit);

void dump_hex(BYTE *buffer, int len, int row);


/* メモリアロケーター */

void *_sys_kmalloc(DWORD memsize);

void *_sys_calloc(DWORD size);

void _sys_kfree(void *ptr);

void _sys_set_new_hanlder(void (*new_hander)());

DWORD get_esp();

#endif
