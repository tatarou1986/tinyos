
#include "CPUemu.h"
#include "system.h"
#include "gdebug.h"
#include "memory.h"
#include "system.h"
#include "cpuseg_emu.h"
#include "ctrl_reg_emu.h"
#include "io_emu.h"
#include "tss_emu.h"
#include "vga.h"
#include "ldt_emu.h"
#include "interrupt_emu.h"
#include "eflags_emu.h"

#define         cpu_hlt()   asm volatile("hlt")

typedef struct _emu_func {
    void    (*func)(vm_emu_datas*);
} emu_func;

/*
 * wrapperルーチン
 */
DWORD _wrap_protect_mode(pushed_regs regs, DWORD error_code, intr_stack stack);
DWORD _wrap_real_mode(pushed_regs regs, DWORD error_code, v8086_intr_stack stack);
void _debug_break_point(pushed_regs regs, intr_stack stack);

static void probe_cpu_code(vm_emu_datas *status);
static void check_2byte_code(vm_emu_datas *status);

static void cpu_emu_lldt_or_ltr(vm_emu_datas *status);

void bitch_debug_reg(vm_emu_datas *status);
void fuck_debug_reg(vm_emu_datas *status);

/*
  戻り値：進めるバイト数
*/
static void Do_check_cpu_opcode(vm_emu_datas *status);

static void code_high_0x0(vm_emu_datas *status, BYTE low_byte);
static void code_high_0x1(vm_emu_datas *status, BYTE low_byte);
static void code_high_0x6(vm_emu_datas *status, BYTE low_byte);
static void code_high_0x8(vm_emu_datas *status, BYTE low_byte);
static void code_high_0xA(vm_emu_datas *status, BYTE low_byte);
static void code_high_0xC(vm_emu_datas *status, BYTE low_byte);
static void code_high_0xE(vm_emu_datas *status, BYTE low_byte);
static void code_high_0xF(vm_emu_datas *status, BYTE low_byte);

/*
  0: real_mode (実はv8086 mode）
  1: protect_mode
*/
static DWORD vm_mode_flag = 0;


/* idts.sから飛んでくる */

static void Do_check_cpu_opcode(vm_emu_datas *status){
    
    BYTE    op_code_high, op_code_low;
    
    /* 命令解析 */
    op_code_high = get_userseg_byte(status->code_addr);
    
    op_code_low = op_code_high & 0xf;
    op_code_high = (op_code_high >> 4) & 0xf;
    
    switch(op_code_high){
    case 0x0:
        code_high_0x0(status, op_code_low);
        break;
    case 0x1:
        code_high_0x1(status, op_code_low);
        break;
    case 0x8:
        code_high_0x8(status, op_code_low);
        break;
    case 0x6:
        code_high_0x6(status, op_code_low);
        break;
    case 0xA:
        code_high_0xA(status, op_code_low);
        break;
    case 0xC:
        code_high_0xC(status, op_code_low);
        break;
    case 0xE:
        code_high_0xE(status, op_code_low);
        break;
    case 0xF:
        code_high_0xF(status, op_code_low);
        break;
            
    default:
        /* これはマジで普通の一般保護例外ハンドラ */
        _sys_redirect_general_protect(status);
            
    }
    
    return;
    
}

DWORD _wrap_real_mode(pushed_regs regs, DWORD error_code,
                      v8086_intr_stack stack)
{  
    void         *org_opcode, *code_addr;
    vm_emu_datas status;
    
    org_opcode = code_addr  = (void*)((stack.cs << 4) + stack.eip);
    status.code_addr        = &code_addr;
    status.stack.real_mode  = &stack;
    status.regs             = &regs;
    
    probe_cpu_code(&status);
    Do_check_cpu_opcode(&status);
    
    return (DWORD)(code_addr - org_opcode);
}

DWORD _wrap_protect_mode(pushed_regs regs, DWORD error_code, 
                         intr_stack stack)
{
    void         *org_opcode, *code_addr;
    DWORD        seg_base;
    vm_emu_datas status;
    
    if (stack.cs == _KERNEL_CS || stack.cs == _SYSENTER_FAKE_USER_CS) {
        _sys_cls();
        _sys_cls();
        _sys_printf("vm interrupt!\ncs: %x, eip: %x\n"
                    "error code: %x\n", 
                    stack.cs, stack.eip, error_code);
        __HLT();
    }
    
    /* 物理アドレス算出 */
    seg_base = cpu_emu_get_seg_base(CPUSEG_REG_CS);
    //_sys_printf("in protect_mode, addr=%x\n", seg_base);
    org_opcode = code_addr = (void*)(seg_base + stack.eip);
    
    //_sys_cls();
    //_sys_printf("code_addr: %x", code_addr);
    
    status.code_addr            = &code_addr;
    status.stack.protect_mode   = &stack;
    status.regs                 = &regs;
    
    probe_cpu_code(&status);
    Do_check_cpu_opcode(&status);
    
    return (DWORD)(code_addr - org_opcode);
}

void _debug_break_point(pushed_regs regs, intr_stack stack)
{
  
    _sys_cls();
    _sys_printf("fuck you die bitch\n");
    
    trap_eflags();
    
}


static void probe_cpu_code(vm_emu_datas *status){
    
    BYTE    code;
    
    status->opcode_prefix = status->addr_prefix = 0;
    
    do {
        code = get_userseg_byte(status->code_addr);
        
        if (code == 0x66) {
            status->addr_prefix   = 1;
        } else if (code == 0x67) {
            status->opcode_prefix = 1;
        } else {
            /* prefixは存在しない */
            --(*(status->code_addr));
            break;
        }
        
    } while (code == 0x66 || code == 0x67);
    
    return;
    
}

static void code_high_0x0(vm_emu_datas *status, BYTE low_byte)
{    
    switch(low_byte){        
    case 0x6:
        break;
    case 0x7:
        /* 0x07 */
        cpu_emu_pop_es(status);
        break;
    case 0xE:
        break;
    case 0xF:
        /* 2-byteコードの恐れあり */
        check_2byte_code(status);
        break;
    }
    
    return;
}

static void code_high_0x1(vm_emu_datas *status, BYTE low_byte)
{
    switch (low_byte) {
    case 0x6:
        break;
    case 0x7:
        /* 0x17 pop ss */
        cpu_emu_pop_ss(status);
        break;
    case 0xF:
        /* 0x1F pop ds */
        cpu_emu_pop_ds(status);
        break;
    }
}

static void code_high_0x6(vm_emu_datas *status, BYTE low_byte)
{
    switch (low_byte) {
    case 0xE:
        io_emu_outsb(status);
        break;
    case 0xF:
        io_emu_outsw_outsd(status);
        break;
    }
}

static void code_high_0x8(vm_emu_datas *status, BYTE low_byte)
{    
    switch (low_byte) {
    case 0xC:
        /* 0xC8 */
        break;
    case 0xE:
        /* 0x8E */
        cpu_emu_mov_sw_ew(status);
        break;
    }    
}

static void code_high_0xA(vm_emu_datas *status, BYTE low_byte)
{    
    switch (low_byte) {
    case 0x0:
        break;
    case 0x1:
        break;
    case 0x8:
        break;
    case 0x9:
        break;
    }
    
}

static void code_high_0xC(vm_emu_datas *status, BYTE low_byte)
{    
    switch (low_byte) {
    case 0x5:
        /* 0xC5 */
        break;
    case 0xF:
        /* iret */
        cpu_emu_iret(status);
        break;
    }
}

static void code_high_0xE(vm_emu_datas *status, BYTE low_byte)
{    
    static void     (*func_table[])(vm_emu_datas*) = {
        &io_emu_in_AL_lb,       /* 0xE4 */
        &io_emu_in_eAX_lb,      /* 0xE5 */
        &io_emu_out_lb_AL,      /* 0xE6 */
        &io_emu_out_lb_eAX,     /* 0xE7 */
        NULL,                   /* 0xE8 */
        NULL,                   /* 0xE9 */
        &emu_far_jmp,           /* 0xEA */
        NULL,                   /* 0xEB */
        &io_emu_in_AL_DX,       /* 0xEC */
        &io_emu_in_eAX_DX,      /* 0xED */
        &io_emu_out_DX_AL,      /* 0xEE */
        &io_emu_out_DX_eAX      /* 0xEF */
    };
    
    func_table[low_byte - 0x4](status);
    
    return;    
}

static void code_high_0xF(vm_emu_datas *status, BYTE low_byte)
{    
    switch(low_byte){
    case 0x4:
        /* cpu hlt */
        //cpu_hlt();
        break;
    case 0xA:
        /* cli */
        cpu_emu_cli(status);
        break;
    case 0xB:
        /* sti */
        cpu_emu_sti(status);
        break;
    }
    
}

void fuck_debug_reg(vm_emu_datas *status)
{    
    get_userseg_byte(status->code_addr);   
}

/* こんなんまともにエミュレーションするつもりありません */
void bitch_debug_reg(vm_emu_datas *status)
{    
    /* mov edx, db6 */
    get_userseg_byte(status->code_addr);
    
}

void check_2byte_code(vm_emu_datas *status)
{    
    BYTE next_code = get_userseg_byte(status->code_addr);
    
    static void     (*func_table[])(vm_emu_datas*) = {
        &emu_mov_reg_crn,   /* 0x20 */
        &bitch_debug_reg,   /* 0x21 */
        &emu_mov_crn_reg,   /* 0x22 */
        &fuck_debug_reg,    /* 0x23 */
        NULL,               /* 0x24 */
        NULL,               /* 0x25 */
        NULL                /* 0x26 */
    };
    
    switch(next_code){
        
    case 0x00:
        cpu_emu_lldt_or_ltr(status);
        //cpu_emu_ltr(status);
        break;
        
    case 0x01:
        //cpu_emu_lgdt(status);
        //_sys_printf("lgdt ok\n");
        cpu_emu_lgdt_or_lidt(status);
        break;
            
    case 0x96:
        cpu_emu_clts(status);
            
    case 0xA1:
        cpu_emu_pop_fs(status);
        break;
            
    case 0xA9:
        cpu_emu_pop_gs(status);
        break;
            
    case 0xB2:
        cpu_emu_lss(status);
        break;
            
    default:
            
        if(next_code >= 20 && next_code <= 0x26){
            /* 関数テーブルに登録されているやつを呼び出す */
            func_table[next_code - 0x20](status);
        }else{
            cpu_emu_pop_cs(status);
        }
        break;
    }
    
    return;
}

static void cpu_emu_lldt_or_ltr(vm_emu_datas *status)
{    
    BYTE modr_m = get_userseg_byte(status->code_addr);
    
    if ((modr_m & 0xf) >= 8) {
        cpu_emu_ltr(modr_m, status);
    } else {
        cpu_emu_lldt(modr_m, status);
    }
    
}


BYTE get_userseg_byte(void **addr)
{    
    WORD    data_segment = _USER_DS;
    BYTE    ret;
    
    asm volatile (
        "pushw          %%gs                    \n"
        "movw           %%cx, %%gs              \n"
        "movb           %%gs:(%%ebx), %%al      \n"
        "popw           %%gs                    \n"
        : "=a" (ret) :
          "c" (data_segment),
          "b" (*addr)
        );
    
    ++(*addr);
    
    return ret;
}

WORD get_userseg_word(void **addr)
{    
    WORD    data_segment = _USER_DS;
    WORD    ret;
    
    asm volatile (
        "pushw          %%gs                    \n"
        "movw           %%cx, %%gs              \n"
        "movw           %%gs:(%%ebx), %%ax      \n"
        "popw           %%gs                    \n"
        : "=a" (ret) :
          "c" (data_segment),
          "b" (*addr)
        );
    
    (*addr) += 2;
    
    return ret;
}

DWORD get_userseg_dword(void **addr)
{    
    WORD    data_segment = _USER_DS;
    DWORD   ret;
    
    asm volatile (
        "pushw      %%gs                    \n"
        "movw       %%cx, %%gs              \n"
        "movl       %%gs:(%%ebx), %%eax     \n"
        "popw       %%gs                    \n"
        : "=a" (ret) :
          "c" (data_segment),
          "b" (*addr)
        );
    
    (*addr) += 4;
    
    return ret;
}

DWORD get_user_dword_value(WORD segment, DWORD ptr)
{    
    DWORD   ret;
    
    asm volatile (
        "pushl      %%fs                \n"
        "movw       %%ax, %%fs          \n"
        "movl       %%fs:(%%ecx), %%ebx \n"
        "popl       %%fs                \n"
        : "=b" (ret) :
          "a" (segment),
          "c" (ptr)
        );
    
    return ret;
}

WORD get_user_word_value(WORD segment, DWORD ptr)
{    
    WORD    ret;
    
    asm volatile (
        "pushl      %%fs                \n"
        "movw       %%ax, %%fs          \n"
        "movw       %%fs:(%%ecx), %%bx  \n"
        "popl       %%fs                \n"
        : "=b" (ret):
          "a" (segment),
          "c" (ptr)
        );
    return ret;
}


DWORD get_vm_mode_status(void)
{
    return vm_mode_flag;
}


void change_vm_mode_status(void)
{
    vm_mode_flag = (~vm_mode_flag) & 0x1;
}


DWORD ret_ebx(vm_emu_datas *status)
{
    return status->regs->ebx;
}

DWORD ret_ecx(vm_emu_datas *status)
{
    return status->regs->ecx;
}

DWORD ret_edx(vm_emu_datas *status)
{
    return status->regs->edx;
}

DWORD ret_esi(vm_emu_datas *status)
{
    return status->regs->esi;
}

DWORD ret_edi(vm_emu_datas *status)
{
    return status->regs->edi;
}

DWORD ret_ebp(vm_emu_datas *status)
{
    return status->regs->ebp;
}

DWORD ret_eax(vm_emu_datas *status)
{    
    return status->regs->eax;
}

DWORD ret_value(vm_emu_datas *status)
{    
    if (get_vm_mode_status()) {
        /* protect mode */
        if (status->addr_prefix) {
            return get_userseg_word(status->code_addr);
        } else {
            return get_userseg_dword(status->code_addr);
        }
    } else {
        
        if (status->addr_prefix) {
            return get_userseg_dword(status->code_addr);
        } else {
            return get_userseg_word(status->code_addr);
        }
    }    
}

