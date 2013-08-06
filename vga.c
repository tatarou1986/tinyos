 
#include "vga.h"
#include "idt.h"

static WORD cursolX;
static WORD cursolY;

static int lockflag;
static BYTE color_value;

void _sys_initVGA(void)
{   
    lockflag    = 0;
    
    /* colorを黒に */
    color_value = 0x07;
    _sys_cls();
}

void _sys_cls(void)
{   
    register int i;
    
    cursolX = cursolY = 0;
    
    //カーソルポジションを初期化
    _sys_updata_cursolpos(cursolX, cursolY);
    
    for (i = 0 ; i < ((VGA_WIDTH*VGA_HEIGHT)<<1) ; i+=2 ) {
        VRAM[i]   = '\0';
        VRAM[i+1] = 0x07;
    }
    
}

void _sys_lockpos(void)
{   
    lockflag    = TRUE;
}

void _sys_unlockpos(void)
{   
    lockflag    = FALSE;
}


void _sys_set_color(BYTE color)
{
    color_value = color;
}

void _sys_updata_vram_pos(WORD x, WORD y)
{
    cursolX = x;
    cursolY = y;
    
}

void _sys_putchar_coord(WORD *x, WORD *y, char s)
{    
    WORD *cursol_pos = (WORD*)VRAM_ADDR + (*x + (*y * VGA_WIDTH));
    switch (s) {
        case '\n':
            (*y)++;     //一行進める
            *x = 0;
            
            if(*y >= VGA_HEIGHT){
                _sys_scrollup();
                *y = VGA_HEIGHT - 1;
                *x = 0;
            }
            break;
            
        case '\b':
            if (*x==0) {
                (*y)--;
                *x = (VGA_WIDTH - 1);
            } else {
                (*x)--;
            }
            *cursol_pos = (WORD)(color_value << 8) + 0x00;
            break;
            
        default:
            //黒で表示0x07
            *cursol_pos = (WORD)(color_value << 8) + s;
            (*x)++;
            
            if (*x >= VGA_WIDTH) {
                *x = 0;
                (*y)++;
                if(*y >= VGA_HEIGHT){
                    _sys_scrollup();
                    *y = VGA_HEIGHT - 1;
                    *x = 0;
                }
            }
            break;
    }
    _sys_updata_cursolpos(*x, *y);
}

void _sys_putchar(char s)
{    
    WORD *cursol_pos = (WORD*)VRAM_ADDR + (cursolX + (cursolY * VGA_WIDTH));

    switch (s) {
        case '\n':
            cursolY++;      //一行進める
            cursolX = 0;
            
            if(cursolY >= VGA_HEIGHT){
                _sys_scrollup();
                cursolY = VGA_HEIGHT - 1;
                cursolX = 0;
            }
            break;
            
        case '\b':
            if (cursolX == 0) {
                cursolY--;
                cursolX = (VGA_WIDTH - 1);
            } else {
                cursolX--;
            }
            *cursol_pos = (WORD)(color_value << 8) + 0x00;
            break;
            
        case '\r':
            cursolX = (VGA_WIDTH - 1);
            break;
            
        default:
            //黒で表示0x07
            *cursol_pos = (WORD)(color_value << 8) + s;
            cursolX++;
            
            if (cursolX >= VGA_WIDTH) {
                cursolX = 0;
                cursolY++;
                if (cursolY >= VGA_HEIGHT) {
                    _sys_scrollup();
                    cursolY = VGA_HEIGHT - 1;
                    cursolX = 0;
                }
            }
            
            break;
    }
     _sys_updata_cursolpos(cursolX, cursolY);
 }

//カーソルの位置を変える
void _sys_updata_cursolpos(WORD x, WORD y)
{    
    WORD pos_sum = y * VGA_WIDTH + x;    
    outb(0x03D4, 0x0f);
    outb(0x03D5, (BYTE)pos_sum);
    outb(0x03D4, 0x0e);
    outb(0x03D5, (BYTE)(pos_sum >> 8));
}

void _sys_scrollup(void)
{    
    WORD         *vram_ptr = (WORD*)VRAM_ADDR;
    register int i;
   
    /* 全部の行を一行上に上げる */
    for(i = 0 ; i < VGA_WIDTH * (VGA_HEIGHT-1) ; i++) {
        vram_ptr[i] = vram_ptr[i+VGA_WIDTH];
    }
    
    /* 最後の行だけをクリアする */
    for(; i < VGA_WIDTH * VGA_HEIGHT ; i++) {
        /* 0x07は黒 */
        vram_ptr[i] = (WORD)((0x07 << 8) + 0x00);
    }    
}

void _sys_putpixel(int xpos, int ypos, BYTE value)
{    
    BYTE *pixel = (BYTE*)VRAM_COLOR_ADDR + (xpos + (ypos * VGA_WIDTH));
    *pixel = value;
    return;
}


void _sys_pset(int x, int y)
{
    BYTE *pixel = (BYTE*)0xA0000 + (x + (y * VGA_WIDTH));
    *pixel  = 0x0f;
}

/*
 * row: 列
 * col: 行
*/
void _sys_get_cursor_pos(WORD *page_num,
                         BYTE *row,
                         BYTE *col,
                         BYTE *start_scan_line,
                         BYTE *end_scan_line)
{    
    outb(0x03D4, 0x0A);
    *start_scan_line = (BYTE)(inb(0x0A) & 0x1f);
    
    outb(0x03D4, 0x0B);
    *end_scan_line = (BYTE)(inb(0x0B) & 0x1f);
    
    *page_num = (0xB8000 + cursolX + (cursolY * VGA_WIDTH)) / 0xB8000;
    
    *row = cursolX;
    *col = cursolY;
    
    return;
}

/* テスト用 */
void show_blue_screen(void)
{
}
