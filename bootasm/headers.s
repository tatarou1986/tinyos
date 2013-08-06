;headers.S（ブート時に使う定数を収めたものです）


%define			DRV_SECTOR				2
%define			C_COUNTER				256


%define			BOOTCODE_SEG			9000h

%define			DEF_SEG					07C0h


%define			CODE_TEMP_SEG			1000h


%define			SIG1					0AA55h
%define			SIG2					5A5Ah



%define			GFW_SIZE				0x100000

%define			GFW_FILE_SIZE			(102400 - 512)


;kernel.hと同期を取ること(^_^;
;%define		_KERNEL_CS				0x8
;%define		_KERNEL_DS				0x10

;%define		_USER_CS				0x18
;%define		_USER_DS				0x20

;kernel.hと同期を取ること(^_^;
%define		_KERNEL_CS				0x30
%define		_KERNEL_DS				0x38

%define		_USER_CS				0x40
%define		_USER_DS				0x48

%define		_TSS_BASE_ESP			0x80000


