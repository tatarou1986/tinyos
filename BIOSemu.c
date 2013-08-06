
#include "BIOSemu.h"
#include "system.h"
#include "memory.h"
#include "device.h"
#include "vga.h"
#include "stack.h"
#include "gdebug.h"
#include "idt.h"
#include "asm-i386.h"

#define         WORD_HIGH(x)        (((x) >> 8) & 0xff)
#define         WORD_LOW(x)         ((x) & 0xff)

#define         REG_WRITE_xLOW(reg, value)              \
                ((reg) &= 0xffffff00);                  \
                ((reg) |= (BYTE)(value))                \
                
#define         REG_WRITE_xHIGH(reg, value)             \
                ((reg) &= 0xffff00ff);                  \
                ((reg) |= (((BYTE)(value)) << 8))       \
                
#define         REG_WRITE_WORD(reg, value)              \
                ((reg) &= 0xffff0000);                  \
                ((reg) |= ((WORD)(value))               \
                
typedef struct __hst_param {
    DWORD       head;
    DWORD       sector;
    DWORD       track;
} hst_param;

/* static void wait_keypush(pushed_regs *regs, v8086_intr_stacks *stack); */

/* asm部から呼ばれる */
void _do_BIOS_emu(DWORD vector_num, pushed_regs regs, DWORD error_code, v8086_intr_stack stack);

static void emulation_int11(pushed_regs *regs, v8086_intr_stack *stack);
static void emulation_int15(pushed_regs *regs, v8086_intr_stack *stack);
static int copy_extended_mem(pushed_regs *regs, v8086_intr_stack *stack);

static void emulation_int13(pushed_regs *regs, v8086_intr_stack *stack);
static void return_cursor_pos(pushed_regs *regs);
/* static void return_disk_type(pushed_regs *regs); */
static void write_string_vga(pushed_regs *regs);
static void get_disk_type(pushed_regs *regs, v8086_intr_stack *stack);

static void reset_fdc(pushed_regs *regs, v8086_intr_stack *stack);

static void emulation_int16(pushed_regs *regs, v8086_intr_stack *stack);

static void check_devices(pushed_regs *regs);

/* int 0x10 function 0x0e */
static void put_char(pushed_regs *regs);

static void set_carry_flag(v8086_intr_stack *stack);
static void clear_carry_flag(v8086_intr_stack *stack);
static void enable_A20(v8086_intr_stack *stack);

static void clear_zero_flag(v8086_intr_stack *stack);
static void set_zero_flag(v8086_intr_stack *stack);

static const char   *fdd[] = {
    "/dev/floppy0",     /* floppy drive A */
    "/dev/floppy1",     /* floppy drive B */
};

static const char   *hdd[] = {
    "/dev/ata00",       /* ata[0:0] device */
    "/dev/ata01",       /* ata[0:1] device */
    "/dev/ata10",       /* ata[1:0] device */
    "/dev/ata11"        /* ata[1:1] device */
};

void _do_BIOS_emu(DWORD vector_num, pushed_regs regs, 
                  DWORD error_code, v8086_intr_stack stack)
{  
    BYTE ah_value = (BYTE)(regs.eax >> 8);
    
    ENABLE_INT();
    
    switch (vector_num) {
         case 0x10:
            /* VGA関連 */
            switch (ah_value) {
                case 0x02:
                    
                    break;
                case 0x03:
                    return_cursor_pos(&regs);
                    break;
                case 0x13:
                    write_string_vga(&regs);
                    break;
                case 0x0e:
                    put_char(&regs);
                default:
                    break;
            }
            break;
            
        case 0x13:
            /* I/O系 */
            emulation_int13(&regs, &stack);
            break;
            
        case 0x11:
            emulation_int11(&regs, &stack);
            break;
        case 0x15:
            emulation_int15(&regs, &stack);
            break;
        case 0x16:
            /* keyboard関連 */
            emulation_int16(&regs, &stack);
            break;
        default:
        
            break;
    }
    
    DISABLE_INT();
    
    //_sys_printf("%x software intrrupt!!\n", vector_num);
    
    return;
    
}

static void emulation_int11(pushed_regs *regs, v8086_intr_stack *stack)
{    
    /* 0でいいのかな */
    regs->eax &= 0xffff0000;    
}

static void emulation_int13(pushed_regs *regs, v8086_intr_stack *stack)
{    
    BYTE   type = (BYTE)((regs->eax >> 8) & 0xff);
    BYTE   read_sector_cnt;
    BYTE   cylinder_num, sector_num, head_num;
    BYTE   drive_num;
    void   *buf_ptr;
    char   buffer[512];
    int    flag;
    DWORD  i = 0;
    FILE   fd;
    
    switch (type) {        
        case 0x00:
            reset_fdc(regs, stack);
            break;
        case 0x01:
            /* 再確認 */
            if ((regs->eax & 0xffff) == 0x2401) {
                enable_A20(stack);
            } else {
                set_carry_flag(stack);
                return;
            }
            break;
        
        case 0x02:
            read_sector_cnt = (BYTE)(regs->eax & 0xff);
            cylinder_num    = (BYTE)((regs->ecx >> 8) & 0xff);
            sector_num      = (BYTE)(regs->ecx & 0xff);
            head_num        = (BYTE)((regs->edx >> 8) & 0xff);
            drive_num       = (BYTE)(regs->edx & 0xff);
            buf_ptr         = (void*)((stack->es << 4) + (regs->ebx & 0xffff));
            
            if (drive_num & 0x80) {
                fd = open(hdd[drive_num & ~0x80]);
            } else {
                fd = open(fdd[drive_num]);                
                if (read_sector_cnt > 18) {
                    set_carry_flag(stack);
                    REG_WRITE_xHIGH(regs->eax, 0x01);
                    close(fd);
                    return;
                }                
            }
            
            if (fd == -1) {
                set_carry_flag(stack);
                REG_WRITE_xHIGH(regs->eax, 0x01);
                return;
            }
            
            do {                
                if(!(ioctl(fd, 0x04, (DWORD)cylinder_num, (DWORD)head_num, (DWORD)(sector_num - 1)))){
                    set_carry_flag(stack);
                    REG_WRITE_xHIGH(regs->eax, 0x04);
                    
                    close(fd);
                    return;
                }
                
                if (!(read(fd, 512, (char*)buffer))) {
                    set_carry_flag(stack);
                    REG_WRITE_xHIGH(regs->eax, 0x01);                    
                    close(fd);
                    return;
                }
                
                //dump_hex(buffer, 512, 13);
                _sys_physic_mem_write(buf_ptr, buffer, 512);
                
                //__STOP();
                
                ++i;
                ++sector_num;
                
                buf_ptr += 512;
                
            } while(i<read_sector_cnt);
            
            REG_WRITE_xHIGH(regs->eax, 0x00);
            REG_WRITE_xLOW(regs->eax, read_sector_cnt);
            
            close(fd);
            
            break;
            
        case 0x15:
            get_disk_type(regs, stack);
            break;
        
        default:
            set_carry_flag(stack);
            return;
    }
    
    /* 成功した */
    
    clear_carry_flag(stack);
    
    //_sys_printf("leave\n");
    //__HLT();
    
    return;
    
}


static void enable_A20(v8086_intr_stack *stack){
    
    clear_carry_flag(stack);
    
}

static void return_cursor_pos(pushed_regs *regs){
    
    WORD    page_num;
    BYTE    start_scan_line, end_scan_line;
    BYTE    row, col;
    
    _sys_get_cursor_pos(&page_num, &row, &col, &start_scan_line, &end_scan_line);
    
    regs->ebx = (regs->ebx & 0xffff0000) | page_num;
    regs->ecx = (regs->ecx & 0xffff0000) | ((start_scan_line << 8) | end_scan_line);
    regs->edx = (regs->edx & 0xffff0000) | ((row << 8) | col);
    
    return;
}


static void write_string_vga(pushed_regs *regs){
    
    BYTE    type        = (BYTE)regs->eax;
    BYTE    page_num    = (BYTE)(regs->ebx >> 8);
    BYTE    show_type   = (BYTE)regs->ebx;
    WORD    str_length  = (WORD)regs->ecx;
    WORD    x_pos       = (BYTE)(regs->edx >> 8);
    WORD    y_pos       = (BYTE)(regs->edx);
    char    *str_buf    = (char*)((regs->es << 16) + (WORD)(regs->ebp & 0xffff));
    
    /* y_pos += page_num * VGA_WIDTH; */
    
    _sys_set_color(show_type);
    
    while(--str_length > 0){
        _sys_putchar_coord(&x_pos, &y_pos, *str_buf);
        str_buf++;
    }
    
}


static void reset_fdc(pushed_regs *regs, v8086_intr_stack *stack){
    
    FILE    fd;
    DWORD   drive_num = regs->edx & 0xff;
    
    if(drive_num & 0x80){
        fd = open(hdd[drive_num & ~0x80]);
    }else{
        fd = open(fdd[drive_num]);
    }
    
    if(fd==-1){
        set_carry_flag(stack);
    }else{
        close(fd);
        clear_carry_flag(stack);
    }
    
    /* 成功したというフラグだけ立てろ */
    //REG_WRITE_xHIGH(regs->eax, 0x00);
    
    //clear_carry_flag(stack);
    
}


static void put_char(pushed_regs *regs){
    
    BYTE    s       = (BYTE)regs->eax;
    //BYTE  color   = (BYTE)regs->ebx;
    
    //_sys_set_color(color);
    
    switch(s){
        
        case 0x07:
            /* ベル鳴らす */
            break;
        
        case 0x08:
            /* バックスペース */
            _sys_putchar('\b');
            break;
            
        case 0x0A:
            _sys_putchar('\n');
            break;
            
        case 0x0D:
            _sys_putchar('\r');
            break;
            
        default:
            _sys_putchar((char)s);
            break;
        
    }
    
}


static void emulation_int15(pushed_regs *regs, v8086_intr_stack *stack){
    
    DWORD           eax_value   = regs->eax;
    _e820map        *mem_map    = P_E820_ENTRY;
    DWORD           mem_size;
    void            *dest;
    
    switch((eax_value >> 8) & 0xff){
        
        case 0x88:
            regs->eax &= 0xffff0000;
            regs->eax |= (WORD)(kernel_virtual_memsize >> 10);
            break;
        case 0x87:
            copy_extended_mem(regs, stack);
            break;
            
        default:
            switch(eax_value & 0xffff){
                
                case 0xe820:
                    if(regs->edx==E820_SMAP && regs->ecx==20){
                        
                        dest = (void*)((stack->es << 4) + (regs->edi & 0xffff));
                        
                        _sys_physic_mem_write(dest, (mem_map->entry + regs->ebx), sizeof(_e820entry));
                        
                        ++(regs->ebx);
                        
                        if(regs->ebx >= E820_MAX){
                            regs->ebx = 0;
                        }
                        
                        regs->ecx = sizeof(_e820entry);
                        regs->eax = E820_SMAP;
                        
                    }else{
                        regs->eax = 0x0;
                        set_carry_flag(stack);
                        return;
                    }
                case 0xe801:
                    
                    /* 16MB以上のメモリ！？ */
                    mem_size = kernel_virtual_memsize;
                    
                    /* kbyte単位に変換 */
                    mem_size >>= 10;
                    
                    if(kernel_virtual_memsize >= 0xffffff){
                        /* 16MB以上？ */
                        
                        regs->eax &= 0xffff0000;
                        regs->eax |= 0x4000;
                        
                        regs->ebx &= 0xffff0000;
                        regs->ebx |= (WORD)((mem_size - 0x4000) >> 6);
                        
                        regs->ecx &= 0xffff0000;
                        regs->ecx |= 0x4000;
                        
                        regs->edx &= 0xffff0000;
                        regs->edx |= (WORD)((mem_size & 0x4000) >> 6);
                        
                    }else{
                    /* 16MB以下のメモリである */
                        regs->ebx &= 0xffff0000;
                        regs->edx &= 0xffff0000;
                        
                        regs->eax &= 0xffff0000;
                        regs->eax |= mem_size;
                        
                        regs->ecx &= 0xffff0000;
                        regs->ecx |= mem_size;
                    }
                    
                    break;
                default:
                    set_carry_flag(stack);
                    return;
            }
    }
    
    clear_carry_flag(stack);
    
}

static int copy_extended_mem(pushed_regs *regs, v8086_intr_stack *stack){
    
    int_15h_87h_desc    desc;
    void                *dest, *src;
    DWORD               cnt;
    
    //ここまちがってるよ
    src = (void*)((regs->es << 4) + (regs->esi & 0xffff));
    _sys_physic_mem_read((void*)&desc, src, sizeof(int_15h_87h_desc));
    
    cnt = (regs->ecx & 0xffff) << 1;
    
    if(((desc.src_physics_addr & 0xff000000)
        & (desc.dest_physics_addr & 0xff000000))==0x93000000){
        
        src     = (void*)(desc.src_physics_addr & 0x00ffffff);
        dest    = (void*)(desc.dest_physics_addr & 0x00ffffff);
        
        _sys_physic_mem_write(dest, src, cnt);
        
        return 1;
    }
    
    return 0;
}

static void check_devices(pushed_regs *regs){
    
    
    
}


static void set_carry_flag(v8086_intr_stack *stack){
    
    stack->eflags |= 0x1;
    
}


static void clear_carry_flag(v8086_intr_stack *stack){
    
    stack->eflags &= ~0x1;
}

static void set_zero_flag(v8086_intr_stack *stack){
    
    stack->eflags |= 0x42;
    
}

static void clear_zero_flag(v8086_intr_stack *stack){
    
    stack->eflags &= ~0x42;
    
}


static void get_disk_type(pushed_regs *regs, v8086_intr_stack *stack){
    
    BYTE        drive_num = (regs->edx & 0xff);
    FILE        fd;
    
    regs->eax &= 0xffff00ff;
    
    clear_carry_flag(stack);
    
    if(drive_num & 0x80){
        /* hddですね */
        if((fd = open(hdd[drive_num & ~0x80])) > -1){
            regs->eax |= 0x0300;
            return;
        }
        
    }else{
        
        if((fd = open(fdd[drive_num])) > -1){
            regs->eax |= 0x0200;
            return;
        }
        
    }
    
/*
    regs->eax &= 0xffff00ff;
    
    if(WORD_LOW(regs->edx) & 0x80){
        
        if((open(hdd[WORD_LOW(regs->edx) & 0x7f]))!=-1){
            regs->eax |= 0x00000300;
        }
        
    }else{
        if((open(fdd[WORD_LOW(regs->edx) & 0x7f]))!=-1){
            regs->eax |= 0x00000200;
        }
    }
    
    clear_carry_flag(stack);
*/

}


static void emulation_int16(pushed_regs *regs, v8086_intr_stack *stack){
    
    int             buf = 0x0;
    FILE            fd;
    static BYTE     key_code = 0x0;
    
    switch((regs->eax >> 8) & 0xff){
        
        case 0x1:
            
            buf = 0x0;
            
            fd = open("/dev/kbd0");
            
            read(fd, 1, &buf);
            
            if(buf!=0x0){
                //set_zero_flag(stack);
                clear_zero_flag(stack);
            }else{
                set_zero_flag(stack);
            }
            
            key_code = buf;
            
            break;
            
        case 0x0:
        
            buf = 0x0;
            
            if(key_code!=0x0){
                REG_WRITE_xLOW(regs->eax, key_code);
                REG_WRITE_xHIGH(regs->eax, key_code);
                
                clear_zero_flag(stack);
                
            }else{
                fd = open("/dev/kbd00");
                
                read(fd, 0, &buf);
                
                if(buf!=0x0){
                    REG_WRITE_xLOW(regs->eax, buf);
                    REG_WRITE_xHIGH(regs->eax, buf);
                    
                    clear_zero_flag(stack);
                }else{
                    set_zero_flag(stack);
                }
                
                close(fd);
                
            }
            
            key_code = 0x0;
            
        default:
            break;
    }
    
}

/*
static void wait_keypush(pushed_regs *regs, v8086_intr_stack *stack){
    
    int     buffer = 0x0;
    FILE    fd;
    
    if((fd=open("/dev/kbd0"))==-1){
        
    }
    
    switch((regs->eax >> 8) & 0xff){
        
        case 0x00:
            for(;;){
                read(fd, 1, &buf);
                
                if(buf!=0x0){
                    break;
                }
            }
            
            REG_WRITE_xLOW(
            
            break;
            
        case 0x01:
            break;
        
    }
    
}
*/

