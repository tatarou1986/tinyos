
#include "kernel.h"
#include "keyboard.h"
#include "io.h"
#include "vga.h"
#include "system.h"
#include "memory.h"
#include "idt.h"

#define USE_LINUX_KEYCODE

#ifdef USE_LINUX_KEYCODE

void _sys_init_keyboard();
int _sys_add_keyboard_buff(char c);
int _sys_read_keyborad_buff(int *data);
void _sys_keyboard_event();

static void _sys_keyboard_irq_event(void);
static int keyboard_read(int read_size, void *buffer);

static unsigned char keyboard_buffer[RING_BUFFER_SIZE];
static unsigned int  readpos, writepos;
static void (*custom_keyboard_handler)(int) = NULL;

static unsigned char linux_keymap[128] = {
    "\000\0331234567890-=\177\t"                                        /* 0x00 - 0x0f */
    "qwertyuiop[]\r\000as"                                              /* 0x10 - 0x1f */
    "dfghjkl;'`\000\\zxcv"                                              /* 0x20 - 0x2f */
    "bnm,./\000*\000 \000\201\202\203\204\205"                          /* 0x30 - 0x3f */
    "\206\207\210\211\212\000\000789-456+1"                             /* 0x40 - 0x4f */
    "230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000"   /* 0x50 - 0x5f */
    "\r\000/"                                                           /* 0x60 - 0x6f */
};

#else

static unsigned char keymap[] = {
    0x00,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']',0x0D,0x80,
    'a','s','d','f','g','h','j','k','l',';',047,0140,0x80,
    0134,'z','x','c','v','b','n','m',',','.','/',0x80,
    '*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,'0',0177
};

#endif

/* エミュレーション用のハンドラ */
emulation_handler   keyboard_emu = {
    KEYBOARD_IN_PORT,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


/* デバイス登録用 */
device_operator     keyboard_operator = {
    1,
    "keyboard device",
    1,  /* ブロックデバイス */
    &_sys_init_keyboard,
    NULL,
    NULL,
    NULL,
    &keyboard_read,
    NULL
};

device_node         keyboard_device = {
    &keyboard_emu,
    NULL,
    NULL
};


void _sys_init_keyboard(){
    
    //キーボードバッファ初期化
    _sys_memset32((BYTE*)keyboard_buffer, 0x0, (sizeof(BYTE) * RING_BUFFER_SIZE));
    
    writepos = readpos = 0;
    
    //IRQ1にキーボードイベント登録
    _sys_set_irq_handle(1, &_sys_keyboard_irq_event);
    
}


/* 
    戻り値に1なら書き込みOK
    0なら書き込み失敗   
*/
int _sys_add_keyboard_buff(char c)
{
    
    if(((writepos+1) % RING_BUFFER_SIZE)==readpos){
        return 0;
    }
    
    //1から書き込み始める
    keyboard_buffer[writepos] = c;
    
    writepos++;
    writepos %= RING_BUFFER_SIZE;
    
    return 1;
}


/* 
    戻り値に1なら書き込みOK
    0なら書き込み失敗   
*/
int _sys_read_keyboard_buff(int *data)
{   
    if((readpos % RING_BUFFER_SIZE)==writepos){
        return 0;
    }
    
    *data = (int)keyboard_buffer[readpos];
    
    readpos++;
    readpos %= RING_BUFFER_SIZE;
    
    return 1;
}



void _sys_keyboard_event(void)
{   
    int buff;
    //char  output[2];

    if (_sys_read_keyboard_buff(&buff)) {
        if (custom_keyboard_handler != NULL) {
            custom_keyboard_handler(linux_keymap[buff]);
        } else {
#ifdef  USE_LINUX_KEYCODE
            _sys_putchar(linux_keymap[buff]);
#else
            _sys_putchar(keymap[buff]);
#endif  
        }
    }
    
}

void _sys_init_keyboard_handler(void (*handler)(int))
{
    custom_keyboard_handler = handler;
}

static void _sys_keyboard_irq_event(void)
{
    _sys_add_keyboard_buff(inb(KEYBOARD_IN_PORT));
}


static int keyboard_read(int read_size, void *buffer)
{
    read_size = 1;
    
    if (_sys_read_keyboard_buff((int*)buffer)) {
        return 1;
    }
    
    return 0;
}
