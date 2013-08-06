
%include	"headers.S"

[section .text]

extern __sys_ignore_int


%macro ig_int 1
	global	_sys_ignore_trampoline_%1
	push	dword %1
	jmp		__sys_ignore_int
%endmacro

%assign i 0
%rep 255
	ig_int i
%assign i i+1
%endrep


