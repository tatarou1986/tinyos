
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "resource.h"

//キーボードドライバ用のリングバッファサイズ
#define		RING_BUFFER_SIZE	15
#define		KEYBOARD_IN_PORT	0x60

//#define		USE_LINUX_KEYCODE

/* エミュレーション用のハンドラ */
extern emulation_handler	keyboard_emu;
extern device_operator		keyboard_operator;
extern device_node			keyboard_device;

void _sys_keyboard_event(void);
void _sys_init_keyboard_handler(void (*handler)(int));

#endif
