
#ifndef __kernel_H__
#define __kernel_H__

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    DWORD;

#define     TRUE        1
#define     FALSE       0

#define     KERNEL_CODE_ADDR    0x010000
#define     C_CODE_OFFSET       (KERNEL_CODE_ADDR + 0x300) //アセンブラ部のコードサイズを足す

#define     NULL                ((void*)(0x0))

//secondboot.Sに本体がある
extern  DWORD   kernel_virtual_memsize;
extern  DWORD   kernel_real_memsize;

/*
 * extern  GDTR_TABLE* gdtr;
 * extern  void*       gfw_startaddr;
*/

#define     VIRTUAL_MEMSIZE_ADDR    0x84
#define     REAL_MEMSIZE_ADDR       0x88
#define     GDTR_ADDR               0x31c
#define     GDT_SEGMENT_ADDR        0x33a

#define     _KERNEL_CS              0x30
#define     _KERNEL_DS              0x38

#define     _USER_CS                0x40
#define     _USER_DS                0x48

#define     _SYSENTER_FAKE_KERNEL_CS 0x98
#define     _SYSENTER_FAKE_KERNEL_DS 0xa0

#define     _SYSENTER_FAKE_USER_CS  0xa8
#define     _SYSENTER_FAKE_USER_DS  0xb0

#define     _USER_START_OFFSET      0x7c00

#define     P_VIRTUAL_MEMSIZE       ((DWORD*)VIRTUAL_MEMSIZE_ADDR)
#define     P_REAL_MEMSIZE          ((DWORD*)REAL_MEMSIZE_ADDR)

#define     P_GDTR                  ((GDTR_TABLE*)GDTR_ADDR)

#define     asmlinkage

void _sys_printk(char *str);

//リニアアドレスを物理アドレスに変換する
DWORD liner2Physic(DWORD liner_addr);

#endif
