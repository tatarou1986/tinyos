; nasm用のマクロだ。
; BIOSで文字を一列分だけ表示できる。
; BIOS割り込みを使っているのでプロテクトモードでは使えないぞ。
; 
%macro printstr 1
	pusha				;内部でaxレジスタ使うから、とりあえず退避
	mov		si, %1
	
%%loop:
	lodsb
	cmp		al, 0
	je		%%end		;等しかったらジャーンプ
	mov		ah, 0Eh
	mov		bx, 7
	int		10h
	
	jmp		%%loop

%%end:
	popa				;退避したaxレジスタを復帰させよう
	
%endmacro


%macro putchar 1
	pusha
	
	mov		al, %1
	mov		ah, 0eh
	
	mov		bx, 0
	
	int		10h
	
	popa
%endmacro

