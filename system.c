
#include "vga.h"
#include "system.h"
#include "memory.h"
#include "gdebug.h"
#include "idt.h"

//#define MALLOC_OFFSET_ADDR 0x15000

/*
 * memo:
 * スタックは0x80000から下降へ
 * 0x10000ぐらいは欲しい
*/
#define         MALLOC_OFFSET_ADDR      0x22000
#define         MALLOC_MEM_SIZE         0x4E000

typedef struct _mem_header {
    struct _mem_header      *next_block;
    DWORD                   block_size;
} mem_header;

/* 現在どこまで探索したかを指し示す部分 */
static mem_header *free_mem_p                 = NULL;
static mem_header *mem_base                   = ((mem_header*)MALLOC_OFFSET_ADDR);
static void       (*failed_malloc_handler)()  = NULL;

static const char digitx[] = {"0123456789abcdefghijklmnopqrstuvwxyz"};

void _sys_printf(const char *format, ...)
{    
    register int i;
    void **list = (void **)&format;
    
    list++;
    
    for (i = 0 ; format[i] != '\0' ; i++) {
        if (format[i] == '%') {
            i++;            
            switch (format[i]) {
                case 'd':
                    _sys_put_signed_Int((int)*(int*)list, 10, 0);
                    list++;
                    break;
                case 'x':
                    _sys_printk("0x");
                    _sys_put_unsigned_Int((unsigned)*(int*)list, 16, 0);
                    list++;
                    break;
                case 'X':
                    _sys_printk("0x");
                    _sys_put_unsigned_Int((unsigned)*(int*)list, 16, 1);
                    list++;
                    break;
                case 'o':       /* 8進数 */
                    _sys_put_signed_Int(*(int*)list, 8, 0);
                    list++;
                    break;
                case 'c':
                    _sys_putchar((char)*(int*)(list));
                    list++;
                    break;
                case 's':
                    _sys_printk((char*)*(int*)(list));
                    list++;
                    break;
                case 'u':
                    _sys_put_unsigned_Int((unsigned int)*(int*)list, 10, 0);
                    list++;
                    break;
                case '%':
                    _sys_putchar('%');
                    break;
            }
        } else {
            _sys_putchar(format[i]);
        }
    }
}


void _sys_reverse(char *s)
{    
    int c, i, j;
    for (i = 0,j = _sys_strlen(s) - 1 ; i < j ; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }    
}

/*
 int _sys_memcmp(const void *buf1, const void *buf1, int n){
    
    asm volatile (
        "movb   (%0), %%al  \n"
        "movb   (%1), %%ah  \n"
        "cmpb   %%al, %%ah  \n"
        "jne    
        "decl   %%ecx       \n"
        "    
}
*/

int _sys_atoi(char *str)
{    
    register int i;
    register int base;
    int ret = 0;
    int count = _sys_strlen(str);
    
    for (i = count, base = 1 ; i >= 0 ; i--, base*=10) {
        ret += base * (str[i] - 0x30);
    }
    
    return ret;
}

char *strstr(const char *searchee, const char *lookfor)
{    
    int i;
    
    if (*searchee == '\0') {
        if (*lookfor) {
            return NULL;
        }
        return (char*)searchee;
    }
    
    while (*searchee) {        
        i = 0;        
        while (1) {
            if (lookfor[i] == '\0') {
                return (char*)searchee;
            }
            
            if (lookfor[i] != searchee[i]) {
                break;
            }
            i++;
        }
        searchee++;
    }
    return NULL;
}

void _sys_freeze(const char *msg)
{    
    _sys_cls();
    _sys_printf("fatal error: %s\n", msg);    
    for(;;){        
    }
}


void _sys_put_signed_Int(int n, unsigned int base, int large)
{    
    unsigned int keta;       //桁数
    unsigned int num, value;
    char buf[256];
    int  sign = 0;
    
    if (n < 0) {
        num = value = -n;
        sign = -1;
        
    } else {
        num = value = n;
        sign = 0;
    }
    
    if (value == 0) {
        keta = 1;
    } else {
        for (keta = 0 ; num > 0 ; num/=base ){
            keta++;
        }
    }
    
    if (sign < 0) {
        keta++;
        buf[0] = '-';
    }
    
    buf[keta] = '\0';
    
    do {
        if (large) {
            buf[keta-1] = _sys_toupper(digitx[(unsigned)(value % base)]);
        } else {
            buf[keta-1] = digitx[(unsigned)(value % base)];
        }
    } while(--keta > 0 && (value /= base) != 0);
    
    _sys_printk(buf);
}


void _sys_put_unsigned_Int(unsigned int n, unsigned int base, int large){
    
    unsigned int keta;
    unsigned int num, value;
    char buf[256];
    
    num = value = n;
    
    if (value == 0) {
        keta = 1;
    } else {
        for (keta = 0 ; num > 0 ; num /= base) {
            keta++;
        }
    }
    
    buf[keta] = '\0';
        
    do {
        if (large) {
          buf[keta-1] = _sys_toupper(digitx[(unsigned)(value % base)]);
        } else {
            buf[keta-1] = digitx[(unsigned)(value % base)];
        }
    } while(--keta > 0 && (value /= base) != 0);
    
    _sys_printk(buf);
}


int _sys_strlen(char s[])
{    
    int i = 0;    
    while (s[i] != '\0') {
        ++i;
    }    
    return i;    
}

int _sys_tolower(int c)
{
    if (c >= 'A' && c <='Z') {
        return ( c + 'a' - 'A' );
    } else {
        return ( c );
    }
}

int _sys_toupper(int c)
{    
    if ( c >= 'a' && c <='z' ) {
        return ( c - 'a' + 'A' );
    } else {
        return ( c );
    }
}

int _sys_bin2hex(BYTE *buffer, int buffersize, char *output_str)
{    
    //bin2hexが使うキャラクタマップ
    const char   hexlist[] = "0123456789abcdef";
    register int i;
    
    if (buffersize <= 0) {
        return -1;
    }
    
    for (i = (buffersize - 1) ; i >= 0 ; i--) {        
        *output_str = hexlist[(buffer[i] >> 4)];    //上位ビット
        output_str++;        
        *output_str = hexlist[(buffer[i] & 0x0f)];  //下位ビット
        output_str++;
    }    
    *output_str = '\0';    
    return buffersize;
}


void _sys_print_bin2hex(BYTE *buffer, int buffersize)
{
    //bin2hexが使うキャラクタマップ
    const char   hexlist[] = "0123456789abcdef";
    register int i;
    
    if (buffersize <= 0) {
        return;
    }
    
    _sys_printk("0x");
    
    for (i = (buffersize - 1) ; i >= 0 ; i--) {
        _sys_putchar(hexlist[(buffer[i] >> 4)]);
        _sys_putchar(hexlist[(buffer[i] & 0x0f)]);        
    }   
}

/*
void *_sys_kmalloc(DWORD memsize){
    
    mem_header      *prev_p;
    mem_header      *ptr;
    DWORD           mem_cnt;
    
    if(free_mem_p==NULL){
        //baseのnext_blockは自分自身を指している
        mem_base->next_block = free_mem_p = prev_p = mem_base;
        
        //とりあえず割り当て可能なメモリは0x60000ぐらい
        mem_base->block_size = (0x60000 / sizeof(mem_header));
    }else{
        prev_p = free_mem_p;
    }
    
    //mem_header分カウントだけ多くする
    mem_cnt = (memsize + sizeof(mem_header) - 1) / sizeof(mem_header) + 1;
    
    for(ptr=prev_p->next_block;;(prev_p=ptr),(ptr=ptr->next_block)){
        if(ptr->block_size >= mem_cnt){
            if(ptr->block_size==mem_cnt){
                //prevのポインタをはずす必要がある
                prev_p->next_block = ptr->next_block;
            }else{
                //新たなブロックサイズを設定
                ptr->block_size -= mem_cnt;
                ptr += ptr->block_size;
                
                //新ヘッダに要求サイズ分だけのメモリ設定
                ptr->block_size = mem_cnt;
            }
            
            //ここまで探索した
            free_mem_p = prev_p;
            return (void*)(ptr + 1);
            
        }
        
        //リストを一周してきてどうやら空きメモリが無い
        if(ptr==free_mem_p){
            if(failed_malloc_handler!=NULL){
                failed_malloc_handler();
            }
            return NULL;
        }
        
    }
    
    
}
*/

/* メモリは低位アドレスから割り当てられてゆく */
void *_sys_kmalloc(DWORD memsize)
{    
    mem_header *prev_p;
    mem_header *ptr;
    mem_header *new_block;
    DWORD      mem_cnt;
    
    if (free_mem_p == NULL) {
        /* baseのnext_blockは自分自身を指している */
        mem_base->next_block = free_mem_p = prev_p = mem_base;
        
        /* とりあえず割り当て可能なメモリは0x60000ぐらい */
        mem_base->block_size = (MALLOC_MEM_SIZE / sizeof(mem_header));
    } else {
        prev_p = free_mem_p;
    }
    
    /* mem_headerのサイズの倍数分になるように調整する */
    mem_cnt = (memsize + sizeof(mem_header) - 1) / sizeof(mem_header) + 1;
    
    for (ptr = prev_p->next_block ;; (prev_p = ptr), (ptr = ptr->next_block)) {
        if (ptr->block_size >= mem_cnt) {
            if (ptr->block_size == mem_cnt) {
                //prevのポインタをはずす必要がある
                prev_p->next_block = ptr->next_block;
            } else {
                new_block = ptr + mem_cnt;
                if((DWORD)new_block >= get_esp()){
                    if (failed_malloc_handler != NULL) {
                        failed_malloc_handler();
                    }
                    return NULL;
                }
                
                new_block->block_size = ptr->block_size - mem_cnt;
                
                /* 割り当てられるblock_sizeを設定 */
                ptr->block_size = mem_cnt;
                
                /* 自分自身を指していた場合 */
                if (ptr->next_block == ptr) {
                    new_block->next_block = new_block;
                } else {
                    new_block->next_block = ptr->next_block;
                }                
                prev_p->next_block = new_block;                
            }
            free_mem_p = new_block;
            return (void*)(ptr + 1);
        }
        
        /* リストを一周してきてどうやら空きメモリが無い */
        if (ptr == free_mem_p) {
            if (failed_malloc_handler != NULL) {
                failed_malloc_handler();
            }            
            return NULL;
        }        
    }    
}

void *_sys_calloc(DWORD size)
{    
    void *buffer = _sys_kmalloc(size);    
    if (buffer != NULL) {
        _sys_memset32(buffer, 0, size);
        return buffer;
    }
    
    return NULL;
}

void _sys_kfree(void *ptr)
{    
    mem_header *bp;
    mem_header *p;
    
    /* ヘッダーを取り出す */
    bp = (mem_header*)ptr - 1;
    
    //_sys_printf("free: %x\n", ptr);
    //__HLT();
    
    // (p p->next_block)の開集合の元pを求める．
    // p < bp < (p->next_block)
    for (p = free_mem_p ; !(bp > p && bp < p->next_block) ; p = p->next_block) {
        if (p >= p->next_block && (bp > p || bp < p->next_block)) {
            break;
        }
    }
    
    /* 下のblockに結合させる */
    if ((p + p->block_size) == bp){
        bp->next_block = p->next_block;
        p->block_size += bp->block_size;
        p->next_block = bp->next_block;
        free_mem_p = p;
        //_sys_printf("low fit\n");
        
        return;
    }
    
    /* 上のblockに結合させる */
    if ((bp + bp->block_size) == p->next_block) {
        bp->block_size += p->next_block->block_size;
        bp->next_block = p->next_block->next_block;
        p->next_block = bp;
        
        free_mem_p = p;
        
        //_sys_printf("upper fit\n");
        return;
    }
    
    /*
        いずれにも属さない場合
        すなわち、間に使用中のblockがある場合など
    */
    bp->next_block = p->next_block;
    p->next_block = bp;
    free_mem_p = p;
    
    //_sys_printf("free free_mem_p: %x\n", free_mem_p);
    return;
}

void _sys_set_new_hanlder(void (*new_hander)())
{    
    failed_malloc_handler = new_hander;    
    return;    
}

DWORD get_esp(void)
{    
    DWORD ret;
    asm volatile (
        "movl   %%esp, %%eax"
    : "=a" (ret) : /* no input */);
    return ret;
}

BYTE hex2bin(char digit)
{    
    BYTE i = digit - '0';    
    if (i > 9) {
        i -= 7;
    }
    return i;
}

void dump_hex(BYTE *buffer, int len, int row)
{    
    int     i, j;    
    for(i = 0, j = 1 ; i < len ; i++, j++) {        
        _sys_printf("%x ", buffer[i]);        
        if (j == row) {
            _sys_putchar('\n');
            j = 1;
        }
    }
}


