;使用しているシステムコール
;ah  = 0xE
;int = 0x10
;
[BITS 16]
[ORG 0]

%define			CODE_TEMP_SEG			0x07C0

jmp		CODE_TEMP_SEG:_start

mem_test	dd		0x0

tmp_gdtr:
	tmp_limit			dw		tmpseg_end - tmpseg_0 - 1
	tmp_base			dd		CODE_TEMP_SEG * 16 + tmpseg_0
	
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
	dw		0x7C00
	db		0x0
	db		0x9A
	db		0xCF
	db		0x0
	
;data segment
tmpseg_10:
	dw		0xffff
	dw		0x7C00
	db		0x0
	db		0x92
	db		0xCF
	db		0x0
tmpseg_end:

__stop:
	jmp		__stop

_start:
	
	cli
	
	mov		eax, CODE_TEMP_SEG
	mov		ds, eax
	mov		ss, eax
	mov		es, eax
	
	mov		al, 0xA
	xor		bh, bh
	mov		ah, 0xE
	mov		bl, 0x0
	int		0x10
	
	mov		eax, tmp_gdtr
	lgdt	[eax]
	
	mov		eax, cr0
	or		eax, 0x1
	mov		cr0, eax
	
	jmp		fresh
	
fresh:
	
[BITS 32]
	db		0x66
	jmp		0x8:_start32
	
_start32:

	mov		al, 0x1
	out		0x61, al
	
	mov		al, 0xb6
	out		0x43, al
	
	mov		al, 0xf2
	out		0x42, al
	
	mov		al, 0x10
	out		0x42, al
	
	mov		al, 0x3
	out		0x61, al
	
	
	
	mov		al, 0x1
	out		0x61, al
	
	mov		al, 0x3
	out		0x61, al
	
	
	mov		al, 0x1
	out		0x61, al
	
	mov		al, 0x3
	out		0x61, al
	
	
	
_loop:
	
	jmp		_loop
	
times 510-($-$$) db 0x0
dw 0xAA55
