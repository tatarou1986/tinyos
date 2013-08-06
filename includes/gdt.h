
#ifndef gdtH
#define gdtH

#include        "kernel.h"

#define         GDT_TYPE_G_FLAG         0x80
#define         GDT_TYPE_AVL_FLAG       0x10
#define         GDT_TYPE_D_FLAG         0x40
#define         GDT_TYPE_P_FLAG         0x80
#define         GDT_TYPE_S_FLAG         0x10
#define         GDT_TYPE_A_FLAG         0x1

#define         GDT_SEG_TYPE_0          0
#define         GDT_SEG_TYPE_1          (1 * 2)
#define         GDT_SEG_TYPE_2          (2 * 2)
#define         GDT_SEG_TYPE_3          (3 * 2)
#define         GDT_SEG_TYPE_4          (4 * 2)
#define         GDT_SEG_TYPE_5          (5 * 2)
#define         GDT_SEG_TYPE_6          (6 * 2)
#define         GDT_SEG_TYPE_7          (7 * 2)

#define         SEG_FLAG_G              0x80
#define         SEG_FLAG_D              0x40
#define         SEG_FLAG_AVL            0x10
#define         SEG_FLAG_P              0x80
#define         SEG_FLAG_S              0x10
#define         SEG_FLAG_A              0x01

/* gdt.h */
typedef struct _segment_entry{
    WORD        limitL;
    WORD        baseL;
    BYTE        baseM;
    BYTE        typeL;
    BYTE        limitH_typeH;
    BYTE        baseH;
} __attribute__ ((packed)) segment_entry;

typedef struct _seg_table{
    WORD            limit;
    DWORD           base;
} __attribute__ ((packed)) seg_table;

typedef seg_table   gdtr_table;
typedef seg_table   tss_table;
typedef seg_table   idtr_table;

void make_gdt_value(DWORD base_addr, DWORD limit, BYTE typeL, 
                    BYTE typeH, BYTE dpl, segment_entry *entry);

void _sys_loadGDT(seg_table *p);

#endif

