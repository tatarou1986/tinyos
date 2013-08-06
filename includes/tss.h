#ifndef tssH
#define tssH

#include "kernel.h"
#include "resource.h"

/*
#define         _DEFAULT_TSS_INDEX      0x28
#define         _USER_TSS_INDEX         0x2B
*/

#define         TSS_ENTRY_NUM           11
#define         LDT_ENTRY_NUM           12

//#define           _DEFAULT_TSS_INDEX      0x30
#define         _DEFAULT_TSS_INDEX      (TSS_ENTRY_NUM << 3)
#define         _TSS_BASE_ESP           (0x80000)
#define         _TSS_SS                 (_KERNEL_DS)

typedef struct _TSS{
    WORD        backlink;       WORD        f1;
    DWORD       esp0;
    WORD        ss0;            WORD        f2;
    DWORD       esp1;
    WORD        ss1;            WORD        f3;
    DWORD       esp2;
    WORD        ss2;            WORD        f4;
    DWORD       cr3;
    DWORD       eip;
    DWORD       eflags;
    DWORD       eax;
    DWORD       ecx;
    DWORD       edx;
    DWORD       ebx;
    DWORD       esp;
    DWORD       ebp;
    DWORD       esi;
    DWORD       edi;
    WORD        es;             WORD        f5;
    WORD        cs;             WORD        f6;
    WORD        ss;             WORD        f7;
    WORD        ds;             WORD        f8;
    WORD        fs;             WORD        f9;
    WORD        gs;             WORD        f10;
    WORD        ldt;            WORD        f11;
    WORD        t;              WORD        iobase;
} __attribute__ ((packed)) TSS;

typedef struct _TSS_IO {
    TSS     tss;
    BYTE    iomap[8193];
} TSS_IO;

/* TRƒŒƒWƒXƒ^‚ÉTSSÝ’è */
#define _sys_set_TR(index_num)                    \
    do {                                          \
        asm volatile (                            \
            "ltr        %%ax"                     \
            : /* no output */ : "a" (index_num)); \
    } while (0)
    
/* LDTÝ’è */
#define     _sys_set_LDTR(index_num)                \
    do {                                            \
        asm volatile (                              \
            "lldt       %%ax"                       \
            : /* no output */ : "a" (index_num));   \
    } while (0)
    

void _sys_init_TSS();

//void _sys_init_TSS(WORD now_ss, DWORD now_esp);

void _sys_make_ldt();

int start_listen_io(emulation_handler *handler);
int stop_listen_io(emulation_handler *handler);

/*
void _sys_set_tss_ss_esp_0(WORD ss, DWORD esp);

void _sys_set_tss_ss_esp_1(WORD ss, DWORD esp);

void _sys_set_tss_ss_esp_2(WORD ss, DWORD esp);
*/

#endif
