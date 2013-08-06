
#include        "ldt_emu.h"
#include        "gdebug.h"
#include        "system.h"
#include        "vga.h"
#include        "gdt.h"
#include        "cpuseg_emu.h"
#include        "memory.h"

#define     GDT_VM_LDT_INDEX        12

static WORD                 v_ldtr = 0x0;
static segment_entry        *v_ldts = NULL;
static DWORD        (*reg_func[])(vm_emu_datas*) = {
    &ret_eax,
    &ret_ecx,
    &ret_edx,
    &ret_ebx,
    &ret_esi,
    &ret_ebp,
    &ret_esi,
    &ret_edi
};

static WORD lldt_emu_high_0xD(BYTE low_code, vm_emu_datas *status);
static WORD lldt_emu_high_0x1(BYTE low_code, vm_emu_datas *status);
static WORD lldt_common_with_prefix(BYTE low_code, vm_emu_datas *status);
static WORD lldt_common_with_no_prefix(BYTE low_code, vm_emu_datas *status);
static WORD lldt_emu_high_0x5(BYTE low_code, vm_emu_datas *status);
static void load_lldt(DWORD selector);

static WORD lldt_emu_high_0xD(BYTE low_code, vm_emu_datas *status)
{    
    return reg_func[low_code](status) & 0xffff;    
}

static WORD lldt_emu_high_0x1(BYTE low_code, vm_emu_datas *status)
{    
    WORD    ret;    
    if (get_vm_mode_status()) {
        if (status->opcode_prefix) {
            lldt_common_with_no_prefix(low_code, status);
        } else {
            lldt_common_with_prefix(low_code, status);
        }
    } else {
        if (status->opcode_prefix) {
            lldt_common_with_prefix(low_code, status);
        } else{
            lldt_common_with_no_prefix(low_code, status);
        }
    }
    
    return ret;
    
}

static WORD lldt_common_with_prefix(BYTE low_code, vm_emu_datas *status)
{    
    DWORD   tmp;
    WORD    ret;
    
    switch (low_code) {
        case 0x4:
            get_userseg_byte(status->code_addr);
            ret = get_user_word_value(status->regs->ds, status->stack.sys_stack->esp);
            break;
        case 0x5:
            tmp = get_userseg_word(status->code_addr);
            ret = get_user_word_value(status->regs->ds, tmp);
            break;
        default:
            ret = get_user_word_value(status->regs->ds, reg_func[low_code](status));
            break;
    }    
    return ret;    
}

static WORD lldt_common_with_no_prefix(BYTE low_code, vm_emu_datas *status)
{    
    WORD ret;    

    switch (low_code) {
        case 0x4:
            ret = get_user_word_value(status->regs->ds, status->regs->esi);
            break;
        case 0x5:
            ret = get_user_word_value(status->regs->ds, status->regs->edi);
            break;
        case 0x7:
            ret = get_user_word_value(status->regs->ds, status->regs->ebx);
            break;
    }
    
    return ret;
    
}

static WORD lldt_emu_high_0x5(BYTE low_code, vm_emu_datas *status)
{    
    WORD    ret;
    
    if (get_vm_mode_status()) {
        if (status->opcode_prefix) {
            ret = get_user_word_value(status->regs->ds, status->regs->ebp & 0xffff);
        } else {
            ret = get_user_word_value(status->regs->ds, status->regs->ebp);
        }
    } else {
        if (status->opcode_prefix) {
            ret = get_user_word_value(status->regs->ds, status->regs->ebp);
        } else {
            ret = get_user_word_value(status->regs->ds, status->regs->ebp & 0xffff);
        }
    }
    
    get_userseg_byte(status->code_addr);
    
    return ret;
}

void cpu_emu_lldt(BYTE modr_m, vm_emu_datas *status)
{    
    WORD ret;
    
    switch ((modr_m >> 4) & 0xf) {
        case 0x1:
            ret = lldt_emu_high_0x1(modr_m & 0xf, status);
            break;
            
        case 0x5:
            ret = lldt_emu_high_0x5(modr_m & 0xf, status);
            break;
            
        case 0xD:
            ret = lldt_emu_high_0xD(modr_m & 0xf, status);
            break;
    }
    
    v_ldtr = ret;
    refresh_ldtr();
    return;
}

void refresh_ldtr(void)
{    
    segment_entry ldt_desc;
    segment_entry *base_addr;
    int limit;
    int i;
    DWORD dpl;
    void *tmp;
    DWORD addr;
    
    if (v_ldtr == 0x0) {
        return;
    }
    
    ldt_desc = cpu_emu_get_guest_seg_value(v_ldtr);
    
    /* Guestのセグメントの値を求める */
    tmp       = (void*)((ldt_desc.baseH << 24) | (ldt_desc.baseM << 16) | ldt_desc.baseL);
    tmp       += cpu_emu_get_seg_base(CPUSEG_REG_CS);
    base_addr = (segment_entry*)tmp;
    limit     = ((ldt_desc.limitH_typeH & 0xf) << 16) | ldt_desc.limitL;
    
    ++limit;
    
    /* levelを落とします */
    dpl = cpu_emu_get_segment_dpl(&ldt_desc);
    dpl = gen_emu_level(dpl);
    
    ldt_desc.typeL &= 0x9f;
    ldt_desc.typeL |= (dpl << 5);
    
    if (v_ldts != NULL) {
        _sys_kfree(v_ldts);
    }
    
    if ((v_ldts = _sys_kmalloc(limit)) == NULL) {
        _sys_freeze("ldt malloc");
    }
    
    for (i = 0 ; i < limit / (int)sizeof(segment_entry) ; i++) {
        _sys_physic_mem_read(&v_ldts[i], base_addr + i, sizeof(segment_entry));
        
        /* dplを下げます */
        dpl = cpu_emu_get_segment_dpl(&v_ldts[i]);
        dpl = gen_emu_level(dpl);
        
        v_ldts[i].typeL &= 0x9f;
        v_ldts[i].typeL |= (dpl << 5);
    }
    
    addr = (DWORD)v_ldts;
    
    /* VMのLDT領域を書き直す */
    ldt_desc.baseH = (BYTE)(addr >> 24);
    ldt_desc.baseM = (BYTE)(addr >> 16);
    ldt_desc.baseL = (WORD)(addr & 0xffff);
    
    /* VMのLDT領域にコピー */
    cpu_emu_write_vm_segment(GDT_VM_LDT_INDEX, &ldt_desc);
    
    /* 更新します */
    //load_lldt(GDT_VM_LDT_INDEX);
}

static void load_lldt(DWORD selector)
{    
    asm volatile (
        "   str         %%ax            \n"
        "   xorl        %%ecx, %%ecx    \n"
        "   ltr         %%cx            \n"
        "   lldt        %0              \n"
        "   ltr         %%ax            \n"
    :: "g"(selector) : "%memory");    
}
