//グラフィックスライブラリ

#ifndef vgaH
#define vgaH

#include		"kernel.h"
#include		"io.h"

/*
#define		VIDEO_REG4		((byte*)0x0004)
#define		VIDEO_REG5		((byte*)0x0005);

#define		VIDEO_X			((byte*)0x0001);
#define		VIDEO_Y			((byte*)0x0002);

#define		VIDEO_COLOR		((byte*)0x0003);
*/

#define		VRAM_ADDR			0xB8000
#define		VRAM_COLOR_ADDR		0xA0000

#define 	VRAM 				((BYTE*)VRAM_ADDR)
#define		BUFF				((WORD*)VRAM_ADDR)

#define		VGA_WIDTH			80
#define		VGA_HEIGHT			25

#define		VGA_CL_RED			0x4
#define		VGA_CL_WHITE		0x7
#define		VGA_CL_GREEN		0x2
#define		VGA_CL_BLUE			0x1
#define		VGA_CL_ORANGE		0x6
#define		VGA_CL_WATER		0x3

#define		VGA_BASE_COLOR		0x0


void _sys_initVGA();

void _sys_cls();

void _sys_lockpos();
void _sys_unlockpos();
void _sys_cls();

void _sys_set_color(BYTE color);
void _sys_putchar(char s);
void _sys_putchar_coord(WORD *x, WORD *y, char s);
void _sys_updata_cursolpos(WORD x, WORD y);
void _sys_updata_vram_pos(WORD x, WORD y);

void _sys_banner();
void _sys_scrollup();

void _sys_putpixel(int xpos, int ypos, BYTE value);
void _sys_pset(int x, int y);
void _sys_get_cursor_pos(WORD *page_num, BYTE *row, BYTE *col, 
                         BYTE *start_scan_line, BYTE *end_scan_line);

#endif
