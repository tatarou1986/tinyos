
;%define        _KERNEL_CS              0x8
;%define        _KERNEL_DS              0x10

;%define        _USER_CS                0x18
;%define        _USER_DS                0x20

;;;kernel.hと必ず同期を取ること(^_^;       
%define     _KERNEL_CS              0x30
%define     _KERNEL_DS              0x38

%define     _USER_CS                0x40
%define     _USER_DS                0x48

%define     _TSS_BASE_ESP           0x80000
%define     _TSS_SS                 _KERNEL_DS

[section .text]

;マクロで何とかできん！？汚すぎ...
global _sys_intrruptIRQ0
global _sys_intrruptIRQ1
global _sys_intrruptIRQ2
global _sys_intrruptIRQ3
global _sys_intrruptIRQ4
global _sys_intrruptIRQ5
global _sys_intrruptIRQ6
global _sys_intrruptIRQ7
global _sys_intrruptIRQ8
global _sys_intrruptIRQ9
global _sys_intrruptIRQ10
global _sys_intrruptIRQ11
global _sys_intrruptIRQ12
global _sys_intrruptIRQ13
global _sys_intrruptIRQ14
global _sys_intrruptIRQ15

global _sys_8086_to_protect

;割り込み一覧
global _sys_ignore_int
global _sys_simd_float_error
global _sys_machine_check_error
global _sys_alignment_check_error
global _sys_floating_point_error
global _sys_page_fault
global _sys_stack_exception
global _sys_seg_not_present
global _sys_invalid_tss
global _sys_cop_seg_overflow
global _sys_double_fault
global _sys_device_not_available
global _sys_invalid_opcode
global _sys_bound_fault
global _sys_overflow_fault
global _sys_break_point
global _sys_nmi_fault
global _sys_debug_fault
global _sys_divide_error_fault
global _sys_general_protection

global _general_iret
global _sys_sysenter_trampoline       

;本体はidt.cにあるよ
extern _sys_printk
extern _sys_printf
extern _IRQ_handler
extern _do_BIOS_emu

;;; in CPUemu.c       
extern _wrap_protect_mode
extern _wrap_real_mode
extern _debug_break_point

;;;本体はdebug.c
extern _sys_dump_cpu

;;;本体はinterrupt.emu.c
extern _sys_wrap_divide_error_fault
extern _sys_wrap_debug_fault
extern _sys_wrap_nmi_fault
extern _sys_wrap_break_point
extern _sys_wrap_overflow_fault
extern _sys_wrap_bound_fault
extern _sys_wrap_invalid_opcode
extern _sys_wrap_device_not_available
extern _sys_wrap_double_fault
extern _sys_wrap_cop_seg_overflow
extern _sys_wrap_invalid_tss
extern _sys_wrap_seg_not_present
extern _sys_wrap_stack_exception
extern _sys_wrap_general_protection
extern _sys_wrap_page_fault
extern _sys_wrap_floating_point_error
extern _sys_wrap_alignment_check_error
extern _sys_wrap_machine_check_error
extern _sys_wrap_simd_float_error
extern _sys_wrap_ignore_int

;;; 本体はmisc.cの中
extern _sysenter_callbackhandler
extern __exit_fromsysenter        
extern __exit_fratsegment        
        
%macro push_stack 0
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
%endmacro

%macro pop_stack    0
    pop         ebx
    pop         ecx
    pop         edx
    pop         esi
    pop         edi
    pop         ebp
    pop         eax
    pop         ds
    pop         es
%endmacro

%macro ig_int 1
    global  _sys_ignore_trampoline_%1
    
    _sys_ignore_trampoline_%1:
        push    dword %1
        jmp     _sys_ignore_int
%endmacro

;256個のidtハンドラ
%assign i 0
%rep 256
    ig_int i
%assign i i+1
%endrep


;だせぇコード...マクロで書き換えられないかしら...
_sys_intrruptIRQ0:
    push        dword 0x0
    jmp         _common_intr_handle
    
_sys_intrruptIRQ1:
    push        dword 0x1
    jmp         _common_intr_handle

_sys_intrruptIRQ2:
    push        dword 0x2
    jmp         _common_intr_handle

_sys_intrruptIRQ3:
    push        dword 0x3
    jmp         _common_intr_handle

_sys_intrruptIRQ4:
    push        dword 0x4
    jmp         _common_intr_handle
    
_sys_intrruptIRQ5:
    push        dword 0x5
    jmp         _common_intr_handle

_sys_intrruptIRQ6:
    push        dword 0x6
    jmp         _common_intr_handle

_sys_intrruptIRQ7:
    push        dword 0x7
    jmp         _common_intr_handle

_sys_intrruptIRQ8:
    push        dword 0x8
    jmp         _common_intr_handle

_sys_intrruptIRQ9:
    push        dword 0x9
    jmp         _common_intr_handle

_sys_intrruptIRQ10:
    push        dword 0xA
    jmp         _common_intr_handle

_sys_intrruptIRQ11:
    push        dword 0xB
    jmp         _common_intr_handle

_sys_intrruptIRQ12:
    push        dword 0xC
    jmp         _common_intr_handle

_sys_intrruptIRQ13:
    push        dword 0xD
    jmp         _common_intr_handle
    
_sys_intrruptIRQ14:
    push        dword 0xE
    jmp         _common_intr_handle
    
_sys_intrruptIRQ15:
    push        dword 0xF
    jmp         _common_intr_handle

_common_intr_handle:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    ;IRQ用の呼び出しハンドラ
    call        _IRQ_handler
    
    jmp         _general_iret
    
_push_stacks:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    mov         eax, esp
    add         eax, 0x24
    call        [ss:eax]
    
    jmp         _general_iret
    
;GuestOSがプロテクトモード時の場合
protect_mode_emu:
    call        _wrap_protect_mode      ;命令を調べる
    jmp         _rebuild_stack          ;eax分だけ、EIPを進める
    
check_cmd_type:
    
    mov         ebp, esp
    
    add         ebp, 0x28
    mov         eax, [ss:ebp]           ;eaxにEIPレジスタが入った
    add         ebp, 0x4
    
    mov         ebx, [ss:ebp]           ;CSレジスタをeaxに
    shl         ebx, 4
    
    add         eax, ebx                ;これでリニアアドレス生成
    
    mov         ebx, _USER_DS
    mov         gs, ebx                 ;V8086mode時のエミュセグメント
    
    ;命令の内容退避
    xor         ecx, ecx
    mov         cx, [gs:eax]
    
    cmp         cl, 0xCD                ;int命令かどうかを確認
    
    jne         _chk_another_cpu_cmd
    
    shr         ecx, 8
    
    push        ecx
    call        _do_BIOS_emu
    add         esp, 0x4
    
    mov         eax, 0x2                ;int命令分(2byte)
    
;eax分だけ、retポインタを進める
_rebuild_stack:

    mov         ebp, esp
    add         ebp, 0x28
    mov         ebx, [ss:ebp]
    add         ebx, eax                ;EIPをeaxの数値分だけ進める
    mov         [ss:ebp], ebx
    
    jmp         _general_iret
    
_chk_another_cpu_cmd:
;realモードだけどint命令じゃない
    call        _wrap_real_mode
    cmp         eax, 0x0
    
    jne         _rebuild_stack
    jmp         no_emulation

_sys_sysenter_trampoline:
    jmp _KERNEL_CS:__exit_fratsegment        

_sysenter_code_chunk:        
    jmp 0xa8:__exit_fromsysenter        
    
_sys_divide_error_fault:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_divide_error_fault
    jmp         _general_iret_no_err_code
    
_sys_debug_fault:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    ;call       _sys_wrap_debug_fault
    call        _debug_break_point
    jmp         _general_iret_no_err_code
    
_sys_nmi_fault:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_nmi_fault
    jmp         _general_iret_no_err_code

_sys_break_point:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    ;call       _sys_wrap_break_point
    call        _debug_break_point
    jmp         _general_iret_no_err_code
    
_sys_overflow_fault:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_overflow_fault
    jmp         _general_iret_no_err_code
    
_sys_bound_fault:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_bound_fault
    jmp         _general_iret_no_err_code
    
_sys_invalid_opcode:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_invalid_opcode
    jmp         _general_iret_no_err_code
    
_sys_device_not_available:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_device_not_available
    jmp         _general_iret_no_err_code
    
_sys_double_fault:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_double_fault
    jmp         _general_iret
    
    
_sys_cop_seg_overflow:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_cop_seg_overflow
    jmp         _general_iret_no_err_code
    
_sys_invalid_tss:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_invalid_tss
    jmp         _general_iret
    
_sys_seg_not_present:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_seg_not_present
    jmp         _general_iret
    
_sys_stack_exception:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_stack_exception
    jmp         _general_iret
    
_sys_page_fault:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_page_fault
    jmp         _general_iret
    
    
_sys_floating_point_error:

    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_floating_point_error
    jmp         _general_iret_no_err_code
    
_sys_alignment_check_error:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_alignment_check_error
    jmp         _general_iret
    

_sys_machine_check_error:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_machine_check_error
    jmp         _general_iret_no_err_code
    
_sys_simd_float_error:
    
    cld 
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_simd_float_error
    jmp         _general_iret_no_err_code
    
;なにこの割り込みふざけてるの？
_sys_ignore_int:

    cld
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    call        _sys_wrap_ignore_int
    jmp         _general_iret
    
    
_sys_general_protection:
    ;V8086modeかどうかを調べる
    
    cld
    push        es
    push        ds
    push        eax
    push        ebp
    push        edi
    push        esi
    push        edx
    push        ecx
    push        ebx
    
    mov         eax, _KERNEL_DS
    mov         ds, eax
    mov         es, eax
    
    ;未来の俺へ
    ;このやり方は馬鹿みたい、スタックにプッシュされたEFLAGSレジスタのVMビットを
    ;調べりゃVMモードかどうかはすぐ分かる、書き直せ！
    mov         ecx, esp
    add         ecx, 0x30
    mov         eax, [ss:ecx]
    
    and         eax, 0x20000
    cmp         eax, 0x20000
    
    jz          check_cmd_type          ;v8086ですね
    jne         protect_mode_emu        ;v8086じゃない(realmodeじゃない)
    
;これが呼び出されたということは、チョイおかしなことが事が起こってる
no_emulation:
;マーク
    ;push       general_protection_msg
    ;call       _sys_printf
    ;add            esp, 0x4
    jmp         no_emulation
    
_general_iret:
    
    pop         ebx
    pop         ecx
    pop         edx
    pop         esi
    pop         edi
    pop         ebp
    pop         eax
    pop         ds
    pop         es
    
    add         esp, 0x4    ;エラーコード破棄
    
    iret
    
_general_iret_no_err_code:
    
    pop         ebx
    pop         ecx
    pop         edx
    pop         esi
    pop         edi
    pop         ebp
    pop         eax
    pop         ds
    pop         es
    
    iret
    
_stop:
    jmp         _stop

;extern void _sys_8086_to_protect(DWORD new_eip, DWORD new_cs, DWORD eflags, DWORD esp, DWORD ss);
_sys_8086_to_protect:       
    
    ;param退避
    pop     eax
    pop     eax     ;new_eip
    pop     ebx     ;new_cs
    pop     ecx     ;eflags
    pop     ebp     ;esp
    pop     esi     ;ss
    
    ;stack完全破壊
    mov     esp, _TSS_BASE_ESP
    
    ;stackを作り直す
    push    esi     ;ss
    push    ebp     ;esp
    
    ;vm-flag解除
    and     ecx, 0xfffd7fd7
    push    ecx     ;eflags
    
    push    ebx
    push    eax
    
    ;これでプロテクトモードへ移行する(v8086解除)
    iret

