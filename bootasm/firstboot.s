
[ORG 0]
[BITS 16]

%include		'printstr.s'
%include		'headers.s'

%define			RST_CNT			10
%define			DRIVE_NUM		0


jmp		DEF_SEG:start


ReadFDOk		db		'Read FD Complete!!',0x0d,0x0a,0x00
loading_msg		db		'gfw Loading',0x00
errmsg			db		'disk read error',0x0d,0x0a,0x00

strack			db		2		;ここから読み出す
scylinder		db		0
shead			db		0
fdcnt			db		0

scnt			dw		GFW_FILE_SIZE / 512

;drive_num		dw		0

hexlist			db		'0123456789abcdef'


;ALレジスタの内容を表示する
print_al:
	pusha
	
	lea			bx, [hexlist]
	mov			dl, al
	
	mov			cl, 4		;4bitシフト
	shr			al, cl
	xlat
	
	putchar		al
	
	lea			bx, [hexlist]
	mov			al, dl
	and			al, 0Fh
	xlat
	
	putchar		al		;マクロ
	
	popa
	ret

start:

;現在CSレジスタの値は07C0h
;今のところデータ領域がCSと同一セグメント内にあるので、DSもCSと
;同一の値にしておく。
	cli
	mov			ax, cs
	mov			ds, ax
	
	mov			ax, 0x7000
	mov			ss, ax			;スタックも同一セグメントにおく
	mov			sp, 0xffff		;spとりあえずこれだけ
	sti
	
;十分な領域があるところまで、自身を移動しよう
moveself:

	mov			si, 0  ;dsは初期化されているから cs = ds　siレジスタにオフセットを入れる。
	mov			ax, BOOTCODE_SEG
	mov			es, ax				;引越し先セグメント
	mov			di, 0				;引越し先オフセット
	mov			cx, C_COUNTER		; 512 / 2(WORD) = 256
	
	;移動
	rep
	movsw
	
	;転送先へ
	jmp			BOOTCODE_SEG:go
	
go:
	;転送されたBOOTCODE_SEG以下のgoへ
	
	;スタック再設定
	cli
	mov			ax, cs
	mov			ds, ax
	;mov			ax, 0x7000
	mov			ss, ax				;スタックも同一セグメント
	mov			sp, 0xffff			;とりあえずスタックこれだけ
	sti
	
diskreset:
	xor			ax, ax
	int			0x13				;FDCを初期化
	jc			diskreset			;とりあえず成功するまでやり続ける
	
_prepare_readfd:
	
	mov			ax, CODE_TEMP_SEG	;転送先セグメントアドレス
	mov			es, ax
	
	mov			bx, 0				;転送先オフセットアドレス
	mov			ah, 0x02			;割り込み用番号

	printstr	loading_msg
	
_reset_counter:
	mov			[fdcnt], byte RST_CNT	;トライする回数のカウンターリセット
	jmp			_read_fd_loop
	
_fd_faild:
	putchar		0x0d
	putchar		0x0a
	printstr	errmsg
	
	xor			ax, ax
	xor			dl, dl
	int			0x13
	
__L0:
	jmp			__L0
	
_read_fd_loop:
	mov			bx, 0
	mov			ah, 0x02
	mov			al, 1
	mov			cl, [strack]		;何トラック(セクタ)目から読み込むか
	mov			ch, [scylinder]		;何シリンダ目から読み込むか
	mov			dh, [shead]			;どのヘッドから読み込むか
	
	dec			byte [fdcnt]
	jz			_fd_faild
	
_read_fd:
	
	int			0x13					;read floppy
	jc			_read_fd_loop
	
	mov			[fdcnt], byte RST_CNT	;リードが成功したらカウンターリセット
	putchar		'.'						;リード成功のポイント
	
	;セグメント側を512byte分足してゆく
	mov			ax, es
	add			ax, 0x20
	mov			es, ax
	
	inc			cl
	cmp			cl, 19
	je			_next_head
	
	mov			[strack], cl
	
	dec			word [scnt]
	cmp			[scnt], word 0x0
	jne			_read_fd_loop
	
	jmp			_read_complete
	
_next_head:

	inc			dh
	cmp			dh, 2
	je			_next_cylinder
	
	mov			cl, 1
	mov			[strack], cl
	mov			[shead], dh
	
	dec			word [scnt]
	cmp			[scnt], word 0x0
	jne			_read_fd_loop
	
	jmp			_read_complete
	
_next_cylinder:
	
	inc			ch
	
	mov			[scylinder], ch
	mov			[strack], byte 1
	mov			[shead], byte 0
	
	dec			word [scnt]
	cmp			[scnt], word 0x0
	jne			_read_fd_loop
	
	jmp			_read_complete
	
_read_complete:
	
stop_FDmoter:
	xor			ax, ax
	xor			dl, dl
	int			0x13
	
jumppad:
	printstr	ReadFDOk

	jmp			CODE_TEMP_SEG:0000h			;secondbootへ
	
times 510-($-$$) db 0				;とりあえず510byteに足りなかったら0で
dw	SIG1							;最後の2byte分はブートシグネチャ

