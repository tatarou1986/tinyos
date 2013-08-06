[BITS 16]
[ORG 0]

%include		'headers.s'

;セグメントディスクリプタの数
%define			TABLES					1
%define			A20_LOOP_COUNTER		255
%define			FIRST_SEGMENT_BASE		0x10000

%define			SMAP					0x534d4150
%define			E820_MAX				32
%define			E820_ENTRY_SIZE			20


_gfw_ready:
	jmp		CODE_TEMP_SEG:start

exmsg		db		'gfw: Excellent Work!! Jumped Second boot!!',0x0D,0x0A,0x0
memerr		db		'gfw: fatal error: not enough memory!',0x0D,0x0A,0x0
a20ok		db		'gfw: A20ok',0x0D,0x0A,0x0
a20fault	db		'gfw: fatal error: a20fault!',0x0D,0x0A,0x0
;oktest		db		'gfw: test code',0x0D,0x0A,0x0


_virtual_memsize	dd		0x0		;bytes
_real_memsize		dd		0x0		;bytes
_gfw_startaddr		dd		0x0
_int_11h_value		dw		0x0

_e820map_entry:
_e820_count:
	dd 0x0
_e820_entry:
	times (E820_MAX * E820_ENTRY_SIZE) db 0x0
	
tmp_gdtr:
	tmp_limit			dw		tmpseg_end - tmpseg_0 - 1
	tmp_base			dd		CODE_TEMP_SEG * 16 + tmpseg_0
	
_gdtr:
	gdtr_limit			dw		segdesc_end - segdesc_0 - 1
	gdtr_base			dd		0
	
;全メモリフラットアクセス用セグメント
;0～0xffffffff
;32bitセグメント
tmpseg_0:
	dw		0
	dw		0
	db		0
	db		0
	db		0
	db		0
	
;code segment
tmpseg_8:
	dw		0xffff
	dw		0x0
	db		0x1
	db		0x9A
	db		0xCF
	db		0x0
	
;data segment
tmpseg_10:
	dw		0xffff
	dw		0x0
	db		0x1
	db		0x92
	db		0xCF
	db		0x0
tmpseg_end:

global __segment_descs
        
;本設定用のセグメント
__segment_descs:      
;;;  0
segdesc_0:
	dw		0		;limitL
	dw		0		;baseL
	db		0		;baseM
	db		0		;typeL
	db		0		;limitH_typeH
	db		0		;baseH
;;; 1
fake_sec_0:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 2
fake_sec_1:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 3
fake_sec_2:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 4
fake_sec_3:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 5
fake_sec_4:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 6
;kernel code segment
segdesc_8:
	code81		dw		0 ;limitL
	code82		dw		0 ;baseL
	code83		db		0 ;baseM
	code84		db		0 ;typeL
	code85		db		0 ;limitH_typeH
	code86		db		0 ;baseH
        
;;; 7
;kernel data segment
segdesc_10:
	data101		dw		0
	data102		dw		0
	data103		db		0
	data104		db		0
	data105		db		0
	data106		db		0
;;; 8
;0x0～0xffffffff
;32bit segment
;DPL=0
;usercs_27:
usercs_18:
	dw		0xffff		;limitL
	dw		0x0			;baseL
	db		0x0			;baseM
	db		0x9A		;typeL
	db		0x8F		;limitH_typeH
	db		0x0			;baseH
;;; 9
;userds_35:
userds_20:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 10
fake_seg:
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0x8F
	db		0x0
;;; 11
;TSSっつても一つだけね、タスク制御には使わんしさ。
tss_2B:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 12
;LDTも一個だけ、これもメモリ管理には使わない
ldt0:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 13
;エミュレート用のセグメント2つ
vm_segment_0:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 14
vm_segment_1:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 15
vm_segment_2:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 16
vm_segment_3:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 17
vm_segment_4:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 18
vm_segment_5:
	dw		0x0
	dw		0x0
	db		0x0
	db		0x0
	db		0x0
	db		0x0
;;; 19
;;; code segment for sysenter (ring 0)        
sysenter_seg_cs1_r0:        
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x9A
	db		0xCF
	db		0x0
;;; 20
;;; data segment for sysenter (ring 0)        
sysenter_seg_ds1_r0:        
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0xCF
	db		0x0
;;; 21
;;; code segment for sysenter (ring 0)
sysenter_seg_cs2_r0:    
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x9A
	db		0xCF
	db		0x0
;;; 22       
;;; data segment for sysenter (ring 0)        
sysenter_seg_ds2_r0:       
	dw		0xffff
	dw		0x0
	db		0x0
	db		0x92
	db		0xCF
	db		0x0        
segdesc_end:

freeze:
	mov			ah, 0
	int			16h
	jmp			freeze
	
not_enough_mem:
	mov			si, memerr
	call		printstr
	
	jmp			freeze
	
;0x0:0x200と0xffff:0x210が同じ値であればA20は有効ではない
a20_test:
	push		ax
	push		cx
	
	mov			ax, 0x0
	mov			fs, ax
	
	mov			ax, 0xffff
	mov			gs, ax

	mov			ax, [fs:0x200]
	push		ax
	
	mov			cx, A20_LOOP_COUNTER

a20_test_loop:
	inc			ax
	mov			[fs:0x200], ax
	
	call		wait_plz				;少し待つ
	
	cmp			[gs:0x210], ax
	loopz		a20_test_loop		;cmp!=0 && cx!=0
	
	pop			ax
	mov			[fs:0x200], ax		;元データーを復帰させる
	
	pop			cx
	pop			ax
	ret

;bx：色とか色々
;si：文字列ポインター
printstr:
	push		ax
	
	cld
	
printstr_loop:

	lodsb
	cmp			al, 0
	jz			printstr_end
	
	mov			ah, 0x0E
	mov			bx, 7
	
	int			0x10
	
	jmp			printstr_loop
	
printstr_end:
	pop			ax
	ret
	
	
;少し待つための手続き	
wait_plz:
	out			0x80, al
	ret

;ここからコード開始
start:
	;セグメントを再設定
	
	cli
	mov			ax, cs		;csレジスタの値は CODE_TEMP_SEGの値
	mov			ds, ax		;dsレジスタの値も設定
	
	mov			ax, 0x7000
	mov			ss, ax
	
	mov			ax, 0xffff
	mov			sp, ax
	sti
	
	mov			si, exmsg
	call		printstr
	
;システムの様子を設定する。
try_enableA20:

;BIOSコールから試してみる
try_bios:
	mov			ax, 0x2401
	int			0x15
	
	call		a20_test
	jnz			a20_done
	
;システムポートから試してみる
try_sysport:
	in			al, 0x92
	
	or			al, 0x02
	and			al, 0xfe
	
	out			0x92, al
	
	call		a20_test
	jnz			a20_done
	
	
a20_falt:
	mov			si, a20fault
	call		printstr
	
	jmp			freeze
	
a20_done:
	mov			si, a20ok
	call		printstr
	
setup_keyboard:
;リピートレートを最大にしておく
	mov			ax, 0x0305
	xor			bx, bx
	int			0x16
	
init_fpu:
	xor			ax, ax
	out			0xf0, al
	call		wait_plz
	
	out			0xf1, al
	call		wait_plz

check_device:
	int			0x11
	mov			[_int_11h_value], ax
	
;e801コールを使用して、メモリサイズ取得
getmemsize_e801:
	xor			ebx, ebx
	xor			eax, eax
	mov			ax, 0xE801
	int			0x15
	
calc_mem_size:
	shl			ebx, 6									; 64KB
	add			ebx, eax
	add			ebx, 1024					;1MB以下の分を足してやる
	shl			ebx, 10									; byte単位に直す
	mov			[_real_memsize], ebx
	mov			eax, [_real_memsize]
	
	cmp			eax, GFW_SIZE
	jle			not_enough_mem
	
	mov			eax, [_real_memsize]
	sub			eax, GFW_SIZE
	mov			[_virtual_memsize], eax
	mov			[_gfw_startaddr], eax
	
getmeminfo_e820:
	push		ds
	pop			es
	mov			edi, _e820_entry
	
	xor			ebx, ebx
	
_e820_L1:
	mov			eax, 0xe820
	mov			edx, SMAP
	mov			ecx, 20

_e820_L2:
	
	int			0x15
	
	jc			_e820_L2
	add			edi, ecx
	
	mov			eax, [_e820_count]
	inc			eax
	mov			[_e820_count], eax
	cmp			eax, E820_MAX
	je			_e820_end
	
	cmp			ebx, 0x0
	jne			_e820_L1
	
_e820_end:
	
;プロテトモードに移行
RealToProtect:

	lgdt		[tmp_gdtr]								;gdtrロード	
	cli													;とりあえず割り込みマスク
	
	mov			al, 80h									;NMIピンを
	out			70h, al									;マスク
	
	mov			eax, cr0
	or			eax, 1
	mov			cr0, eax
	
	jmp			fresh1
	
fresh1:
[BITS 32]
	db			0x66
	jmp			0x08:protect_start
	
protect_start:
;ここから32ビットへ移行だ
	mov			ax, 0x10
	mov			ds, ax
	mov			es, ax
	mov			ss, ax
	mov			gs, ax
	mov			fs, ax
	
	mov			eax, 0x90000 - FIRST_SEGMENT_BASE							;とりあえずスタックこんだけ
	mov			esp, eax
	
	jmp			make_code_segdesc
	
make_code_segdesc:
;	mov			eax, [_real_memsize]
;	sub			eax, [_virtual_memsize]
	
	mov			eax, 0xfffff
	mov			[code81], ax
	
	shr			eax, 16
	mov			ecx, eax
	
	mov			eax, [_virtual_memsize]
	mov			[code82], ax
	
	shr			eax, 16
	mov			[code83], al
	mov			[code86], ah
	
	mov			bl, 0x9A								;P,DPL,S,Type,A
	mov			[code84], bl
	and			cl, 0x0f
	or			cl, 0xC0								;G,D,res,AVL,0x0
	mov			[code85], cl
	
make_data_segdesc:
;	mov			eax, GFW_SIZE
	
	mov			eax, 0xfffff		;limit value (* kbytes)
	mov			[data101], ax
	
	shr			eax, 16
	mov			ecx, eax
	
	mov			eax, [_virtual_memsize]
	mov			[data102], ax
	
	shr			eax, 16
	mov			[data103], al
	mov			[data106], ah
	
	mov			bl, 0x92			;P,DPL,S,Type,A
	mov			[data104], bl		
	and			cl, 0x0f
	or			cl, 0xC0			;G,D,res,AVL,0x0
	mov			[data105], cl

make_finally_gdtr:
	mov			eax, [_virtual_memsize]
	;sub		eax, FIRST_SEGMENT_BASE
	
	add			eax, segdesc_0
	mov			[gdtr_base], eax
	
;secondboot.SとCのコードを、gfwのあるべき場所へ移動させる。
move_2nd:
	
	cld
	
	xor			eax, eax
	
	mov			esi, 0x10000 - FIRST_SEGMENT_BASE
	mov			eax, [_virtual_memsize]
	
	sub			eax, FIRST_SEGMENT_BASE				;baseアドレス分引く
	
	mov			edi, eax
	mov			ecx, GFW_SIZE / 2			;100000 / 2
	
	rep
	movsw
	
	add			eax, set_finally_gdtr
	jmp			eax
	
set_finally_gdtr:
	
	;lgdt		[_gdtr]
	mov			eax, [_virtual_memsize]
	sub			eax, FIRST_SEGMENT_BASE
	add			eax, _gdtr
	
	lgdt		[eax]
	
	jmp			fresh2
	
fresh2:
	jmp			_KERNEL_CS:reset_segment
	
reset_segment:
	
	mov			eax, _KERNEL_DS
	mov			ds, eax
	mov			es, eax
	mov			ss, eax
	mov			gs, eax
	mov			fs, eax
	
	mov			esp, _TSS_BASE_ESP
	
;とりあえず0x0ぶちこんどけ
LDT_setup:
	xor			eax, eax
	lldt		ax
	
kernel_start:

	call		startup_32
	
_oops:
	jmp			_oops
	
times 0x800-($-$$) db 0					;0x800byteになるように固定

startup_32:
