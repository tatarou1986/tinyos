
#include <kernel.h>
#include <misc.h>
#include <tss.h>
#include <idt.h>
#include <msr.h>
#include <gdt.h>
#include <memory.h>

asmlinkage extern void __exit_fromsysenter(void);
asmlinkage extern void _sys_sysenter_trampoline(void);

void cpuid(DWORD eax, cpuid_regs *regs)
{
    __asm__ volatile (
        "pushl %%ebx\n\t"
        "cpuid \n\t"
        "movl %%ebx, %%esi\n\t"
        "popl %%ebx"
        : "=a"(regs->a), "=S"(regs->b), 
          "=c"(regs->c), "=d"(regs->d) /* output */
        : "a"(eax) /* input */
        );
}

DWORD read_msr(DWORD msr)
{
    DWORD val;    
    __asm__ volatile("rdmsr" : "=a"(val) : "c"(msr));
    return val;
}


void write_msr(DWORD msr, 
               DWORD low, 
               DWORD high)
{
    __asm__ volatile ("wrmsr" 
                      : /* no output */ 
                      : "c" (msr), "a"(low), "d" (high) /* input  */
                      : "memory");
}

/* void start_usercodechunk_for_sysenter(void) */
/* { */
/*     __asm__ volatile ( */
/*         "cli \n\t" */
/*         "ljmp %0, $enter_fakeuser \n\t" */
/*         "enter_fakeuser:\n\t" */
/*         "movl %1, %%eax \n\t" */
/*         "movw %%ax, %%ds \n\t" */
/*         "movw %%ax, %%es \n\t" */
/*         "movw %%ax, %%ss \n\t" */
/*         "movw %%ax, %%gs \n\t" */
/*         "movw %%ax, %%fs \n\t" */
/* //        "sti \n\t" */
/*         "pushl %%ecx\n\t" */
/*         "pushl %%edx\n\t" */
/*         "pushl %%ebp\n\t" */
/*         "movl %%esp, %%ebp \n\t" */
/*         "sysenter \n\t" */
/* //        "ljmp $0x98, $_sys_sysenter_trampoline \n\t" */
/*         "__exit_fromsysenter: \n\t"         */
/*         "movl $0xb0, %%ecx \n\t" */
/*         "movw %%cx, %%ss \n\t" */
/*         "movw %%cx, %%ds \n\t" */
/*         "movw %%cx, %%es \n\t" */
/*         "movw %%cx, %%gs \n\t" */
/*         "movw %%cx, %%fs \n\t" */
/*         "movl %%ebp, %%esp \n\t" */
/*         "popl %%ebp\n\t" */
/*         "popl %%edx\n\t" */
/*         "popl %%ecx\n\t" */
/* //        "cli \n\t" */
/*         "movl %3, %%eax \n\t" */
/*         "movw %%ax, %%ds \n\t" */
/*         "movw %%ax, %%es \n\t" */
/*         "movw %%ax, %%ss \n\t" */
/*         "movw %%ax, %%gs \n\t" */
/*         "movw %%ax, %%fs \n\t" */
/*         "sti \n\t" */
/*         ".extern _sys_sysenter_trampoline\n\t" */
/*         ".global __exit_fromsysenter\n\t" */
/*         : /\* no output *\/ */
/*         : "i"(_SYSENTER_FAKE_USER_CS), "i"(_SYSENTER_FAKE_USER_DS), */
/*           "i"(_KERNEL_CS), "r"(_KERNEL_DS)); */
/* } */

#define USERSPACE_FOR_SYSENTER 0x7c000
#define ESP_FOR_USERSPACE (USERSPACE_FOR_SYSENTER + 0x10000)

void start_ring3_codechunk_for_sysenter(void)
{
    DWORD base_addr = *P_VIRTUAL_MEMSIZE;
        
    /* フラットセグメントに変更した後, ゲスト空間にコードを流しこんで実行 */
    __asm__ volatile (
        "pusha \n\r"
        "cli \n\r"
        "movl %%esp, %%ebp \n\t"
        "pushl $(4 * 1024 * 10) ## push esp \n\t"
        "pushf \n\t"
        "pushl %0 ## push cs \n\t"
        "movl $change_flatsegment, %%eax \n\t"
        "addl %2, %%eax \n\t"
        "pushl %%eax ## push eip \n\t"
        "iret \n\t"
        "cli \n\t"
        "change_flatsegment:\n\t"
        "movl %1, %%eax \n\t"
        "movw %%ax, %%ss \n\t"
        "movw %%ax, %%es \n\t"
        "movw %%ax, %%ds \n\t"
        "copyusercode: \n\t"
        "pushf \n\t"
        "cld \n\t"
        "movl $start_user_code_chunk, %%esi \n\t"
        "movl %2, %%eax \n\t"
        "addl %%eax, %%esi \n\t"
        "movl %5, %%edi \n\t"
        "movl $(end_user_code_chunk - start_user_code_chunk), %%ecx \n\t"
        "rep \n\t"
        "movsb \n\t"
        "popf \n\t"
        "__exit_aaaaaaaaaaaaaaaaa: \n\t"
        "movl %2, %%edi \n\t"
        "movl %5, %%eax \n\t"
        "jmp *%%eax \n\t"
        "__exit_fromsysenter: \n\t"        
        "ljmp %3, $__exit_fratsegment \n\t" 
        "__exit_fratsegment: \n\t"
        "movl %4, %%eax \n\t"
        "movw %%ax, %%ds \n\t"
        "movw %%ax, %%es \n\t"
        "movw %%ax, %%ss \n\t"
        "movw %%ax, %%gs \n\t"
        "movw %%ax, %%fs \n\t"
        "movl %%ebp, %%esp \n\t"
        "jmp end\n\t"
        ".extern _sys_sysenter_trampoline \n\t"
        ".global __exit_fromsysenter \n\t"        
        ".global __exit_fratsegment \n\t"
        ".global __exit_aaaaaaaaaaaaaaaaa \n\t"
        "start_user_code_chunk: \n\t"
        "sysenter \n\t"
        "end_user_code_chunk: \n\t"
        "end: \n\t"
        "sti \n\t"
        "popa \n\t"
        : /* no output */
        : "i"(_SYSENTER_FAKE_USER_CS), "i"(_SYSENTER_FAKE_USER_DS), "b"(base_addr),
          "i"(_KERNEL_CS), "i"(_KERNEL_DS), 
          "i"(USERSPACE_FOR_SYSENTER), 
          "i"(ESP_FOR_USERSPACE));
}

asmlinkage void _sysenter_callbackhandler(void)
{
    _sys_cls();    
    _sys_printf("hello world from sysenter\n");
}

#define COPY_SEGMENT_ENTRY(dst_idx, src_idx, segs) \
    _sys_memcpy(&segs[dst_idx], &segs[src_idx], sizeof(segment_entry))

/* 
 * CS register set to the value of SYSENTER_CS_MSR 
 * EIP register set to the value of SYSENTER_EIP_MSR
 * SS register set to the sum of 8 plus the value in SYSENTER_CS_MSR
 * ESP register set to the value of SYSENTER_ESP_MSR
*/
void sysenter_setup(void)
{
    /* segment_entry *segs = (segment_entry*)GDT_SEGMENT_ADDR; */
    /* gdtr_table *gdtr = (gdtr_table*)GDTR_ADDR; */
    DWORD base_addr = *P_VIRTUAL_MEMSIZE;

//    COPY_SEGMENT_ENTRY(_SYSENTER_FAKE_KERNEL_CS >> 3, _KERNEL_CS >> 3, segs);
//    COPY_SEGMENT_ENTRY(_SYSENTER_FAKE_KERNEL_DS >> 3, _KERNEL_DS >> 3, segs);
//    COPY_SEGMENT_ENTRY(_SYSENTER_FAKE_USER_CS >> 3, _KERNEL_CS >> 3, segs);
//    COPY_SEGMENT_ENTRY(_SYSENTER_FAKE_USER_DS >> 3, _KERNEL_DS >> 3, segs);

//    _sys_loadGDT(gdtr);
    
    /* refresh GDTR */
    /* __asm__ volatile ( */
    /*     "cli \n\t" */
    /*     "ljmp %0, $flush \n\t" */
    /*     "flush:\n\t" */
    /*     "movl %1, %%eax \n\t" */
    /*     "movw %%ax, %%ds \n\t" */
    /*     "movw %%ax, %%es \n\t" */
    /*     "movw %%ax, %%ss \n\t" */
    /*     "movw %%ax, %%gs \n\t" */
    /*     "movw %%ax, %%fs \n\t" */
    /*     "sti \n\t" */
    /*     : /\* no output *\/ */
    /*     : "i"(_SYSENTER_FAKE_USER_CS),  */
    /*       "r"(_SYSENTER_FAKE_USER_DS));    */

    /* __asm__ volatile ( */
    /*     "cli \n\t" */
    /*     "ljmp %0, $flush2 \n\t" */
    /*     "flush2:\n\t" */
    /*     "movl %1, %%eax \n\t" */
    /*     "movw %%ax, %%ds \n\t" */
    /*     "movw %%ax, %%es \n\t" */
    /*     "movw %%ax, %%ss \n\t" */
    /*     "movw %%ax, %%gs \n\t" */
    /*     "movw %%ax, %%fs \n\t" */
    /*     "sti \n\t" */
    /*     : /\* no output *\/ */
    /*     : "i"(_KERNEL_CS), "r"(_KERNEL_DS)); */

    DISABLE_INT();

    write_msr(MSR_IA32_SYSENTER_CS, _SYSENTER_FAKE_KERNEL_CS, 0);
    /* DO NOT USE stack in _sys_sysenter_trampoline */
    write_msr(MSR_IA32_SYSENTER_ESP, 0, 0);
    write_msr(MSR_IA32_SYSENTER_EIP, (unsigned long)_sys_sysenter_trampoline + base_addr, 0);

    ENABLE_INT();   
}
