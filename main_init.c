//メインルーチン
#include <main.h>
#include <keyboard.h>
#include <misc.h>
#include <msr.h>

#define _sys_color_putchar(s, color)            \
    do {                                        \
        _sys_set_color(color);                  \
        _sys_putchar(s);                        \
    } while(0);

#define SHOW_USAGE()                                                    \
    do {                                                                \
        _sys_printf(" Push r key to execute rdmsr(MSR_IA32_SYSENTER_CS)\n"); \
        _sys_printf(" Push w key to execute wrmsr(0xffff -> MSR_IA32_SYSENTER_CS)\n"); \
        _sys_printf(" Push c key to execute cpuid(eax = 0x1)\n");       \
        _sys_printf(" Push s key to execute sysenter()\n");             \
    } while (0);

static void _sys_show_banner(void);
static void init_kernel(void);

void ata_test(void);
void asm_test(void);
void malloc_test(void);

void nest1(void);
void nest2(void);
void nest3(void);
void func(void);

void _sys_init_test(void);
void execute_special_op(int key);

DWORD kernel_virtual_memsize;
DWORD kernel_real_memsize;
DWORD memsize;

/* すべてはここから始まる（C言語のコードね） */
void start_kernel(void)
{
    DWORD base_addr = *P_VIRTUAL_MEMSIZE;
    
    /* とっととVGA初期化 */
    _sys_initVGA();

    init_kernel();
    
    /* Initialize PIC */
    _sys_init_8259A();
    
    /* TSS設定 */
    _sys_init_TSS();
    
    /* IDTを設定 */
    _sys_SetUpIDT();
    
    /* LDTに設定 */
    _sys_make_ldt();
    
    /* 周辺機器をすべて初期化 */
    _sys_init_all_device();
    
    /* ethernet初期化 */
    _sys_init_ethernet();

    _sys_init_keyboard_handler(&execute_special_op);

    sysenter_setup();

    SHOW_USAGE();

    while (1) {
        _sys_keyboard_event();
        __asm__ volatile ("hlt");
    }

    __STOP();
    
    /* vmを初期化 */
    _sys_init_vm();
    
    _sys_setup_mem();
    
    memsize = *P_REAL_MEMSIZE;
    _sys_printf("total memsize: %d [kbyte]\n", memsize >> 10);   
    
    /* VM起動開始 */
    //start_vm8086();
    
}

void execute_special_op(int key)
{
    cpuid_regs regs;
    int show_usage = 0;
    DWORD ret;
    DWORD r[3];
    
    switch (key) {
    case 'c':
        _sys_memset32(&regs, 0x0, sizeof(regs));
        cpuid(0x1, &regs);
        _sys_set_color(VGA_CL_GREEN);
        _sys_printf("cpuid eax:%x, ebx:%x, ecx:%x, edx:%x\n", 
                    regs.a, regs.b, regs.c, regs.d);
        _sys_set_color(VGA_CL_WHITE);
        show_usage = 1;
        break;
    case 'w':
        write_msr(MSR_IA32_SYSENTER_CS, 0xffff, 0x0);
        _sys_set_color(VGA_CL_ORANGE);
        _sys_printf("wrmsr 0x1111 -> MSR_IA32_SYSENTER_CS\n"
                    "wrmsr 0x22222222 -> MSR_IA32_SYSENTER_ESP\n"
                    "wrmsr 0x33333333 -> MSR_IA32_SYSENTER_EIP\n");
        _sys_set_color(VGA_CL_WHITE);
        show_usage = 1;
        break;
    case 'r':
        r[0] = read_msr(MSR_IA32_SYSENTER_CS);
        r[1] = read_msr(MSR_IA32_SYSENTER_ESP);
        r[2] = read_msr(MSR_IA32_SYSENTER_EIP);
        _sys_set_color(VGA_CL_RED);
        _sys_printf("MSR_IA32_SYSENTER_CS = %x\n"
                    "MSR_IA32_SYSENTER_ESP = %x\n" 
                    "MSR_IA32_SYSENTER_EIP = %x\n",
                    r[0], r[1], r[2]);
        _sys_set_color(VGA_CL_WHITE);
        show_usage = 1;
        break;
    case 's':        
//        start_usercodechunk_for_sysenter();
        sysenter_setup();
        start_ring3_codechunk_for_sysenter();
        r[0] = read_msr(MSR_IA32_SYSENTER_CS);
        r[1] = read_msr(MSR_IA32_SYSENTER_ESP);
        r[2] = read_msr(MSR_IA32_SYSENTER_EIP);
        _sys_set_color(VGA_CL_WATER);
        _sys_printf("sysenter (cs: %x, eip:%x, esp:%x)\n",
                    r[0], r[1], r[2]);
        _sys_set_color(VGA_CL_WHITE);
        show_usage = 1;
    default: 
        break;
    }

    if (show_usage)
        SHOW_USAGE();
    
}

void init_kernel(void)
{
    
    kernel_virtual_memsize  = *P_VIRTUAL_MEMSIZE;
    kernel_real_memsize     = *P_REAL_MEMSIZE;
    
    _sys_printf("Paging enabled...( virtualmem = %x, realmem = %x )\n", 
                kernel_virtual_memsize, kernel_real_memsize);
    
    /* page.h */
    _sys_printf(" PDE address...%x\n", 
                kernel_virtual_memsize + (DWORD)pg0_PDE_entry);
    _sys_printf(" PTE address...%x\n", 
                kernel_virtual_memsize + (DWORD)pg0_PTE_entry);

    _sys_printf("Grid Frame Work starting address...%x\n", 
                kernel_virtual_memsize);
    
    //これもまたご愛嬌
    _sys_show_banner();    
}

static void color_print(char *s, BYTE *cycle_pattern, int pattern_len)
{
    int i = 0;
    BYTE c;

    while ((c = *s++)) {
        _sys_color_putchar(c, cycle_pattern[i]);
        i = (i + 1) % pattern_len;
    }
}

static void _sys_show_banner(void)
{
    static BYTE color_cycle[] = {
        VGA_CL_RED, VGA_CL_WATER,
        VGA_CL_GREEN, VGA_CL_ORANGE, 
    };

    color_print("\nKAZUSHI Operating System\n   (written by kazushi)\n", 
                color_cycle, 
                sizeof(color_cycle) / sizeof(BYTE));   
    
    _sys_set_color(VGA_CL_WHITE);   
    
    _sys_printf("$DATE: ");
    _sys_printf(" %s ", __DATE__);
    _sys_printf(" %s $\n", __TIME__);
    
    _sys_putchar('\n');
    
}


void _sys_printk(char *str)
{
    while (*str!='\0') {
        _sys_putchar(*str);
        str++;
    }
    
    return;
}


DWORD liner2Physic(DWORD liner_addr)
{
    //基底メモリを足せば物理アドレスになる
    return (liner_addr + kernel_virtual_memsize);
}


void malloc_test(void)
{
    char *buffer[9];
    char *test;
    int i;
    
    buffer[0] = (char*)_sys_kmalloc(2310);
    _sys_memset32(buffer[0], 'q', 2310);
    //_sys_kfree(buffer[0]);
    
    buffer[1] = (char*)_sys_kmalloc(1000);
    _sys_kfree(buffer[1]);
    
    buffer[2] = (char*)_sys_kmalloc(1500);
    buffer[3] = (char*)_sys_kmalloc(1100);
    _sys_memset32(buffer[3], 'r', 1100);
    _sys_kfree(buffer[3]);
    
    buffer[4] = (char*)_sys_kmalloc(1600);
    buffer[5] = (char*)_sys_kmalloc(200);
    _sys_memset32(buffer[5], 'k', 200);
    
    buffer[6] = (char*)_sys_kmalloc(400);
    _sys_kfree(buffer[6]);
    
    buffer[7] = (char*)_sys_kmalloc(4000);
    _sys_memset32(buffer[7], 'c', 4000);
    _sys_kfree(buffer[7]);
    
    buffer[8] = (char*)_sys_kmalloc(5500);
    
    _sys_printf("excellent!\n");
    
}


void ata_test(void)
{
    BYTE        *buffer = (BYTE*)_sys_kmalloc(1024);
    FILE        fd;
    
    fd = open("/dev/ata00");
    
    seek(fd, 1, 0x0);
    read(fd, 1024, buffer);
    dump_hex(buffer, 1024, 14);
}


void asm_test(void)
{
    _e820map        *map = P_E820_ENTRY;
    DWORD           base_addr, length;
    DWORD           type_num;
    WORD            value = *P_INT_11H_VALUE;
    int             i;
    char            *type[] = {
        "usable",
        "reserved",
        "ACPI data",
        "ACPI NVS"
    };
    
    for (i = 0 ; i < map->length ; i++) {
        base_addr = map->entry[i].base_addr_low;
        length    = base_addr + map->entry[i].length_low;
        type_num = map->entry[i].type;
        
        _sys_printf("%x..%x (%s)\n", 
                    base_addr, 
                    length, type[type_num - 1]);
    }
    
    _sys_printf("int 11h: %x\n", value);
    
    __STOP();
    
}

void nest1(void)
{    
    char    test[512];
    
    _sys_memset32(test, 1, 512);
    
    func();
    func();
    func();
    
    nest2();
    
    _sys_printf("return!\n");
}

void nest2(void)
{    
    char    test[512];
    
    _sys_memset32(test, 2, 512);
    
    func();
    func();
    func();
    
    nest3();
}

void nest3(void)
{    
    char    test1[512];
    char    test2[512];
    
    _sys_memset32(test1, 3, 512);
    _sys_memset32(test2, 4, 128);
    
    func();
    func();
    func();
    
    __HLT();
    __NOP();
    
}

void func(void)
{    
    _sys_printf("ok\n");    
}

void _sys_init_test(void)
{    
    _sys_printf("\n");
    _sys_printf("loading... ");
    
    for (;;) {
        //_sys_printf("\b");
        /*
        _sys_putchar('\b');
        _sys_printf("|");
        _sys_putchar('\b');
        _sys_printf("/");
        _sys_putchar('\b');
        _sys_printf("-");
        _sys_printf("\b");
        _sys_printf("\\");
        */
        _sys_printf("\b|\b/\b-\b\\");
    }
    
}

