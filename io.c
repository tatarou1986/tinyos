
#include "io.h"
#include "system.h"
#include "vga.h"

void outl(WORD dest_port, DWORD input_data)
{   
    asm volatile (
        "outl       %%eax, %%dx"
    :: "d" (dest_port), "a"((DWORD)input_data));    
}

void outw(WORD dest_port, WORD input_data)
{
    asm volatile (
        "outw       %%ax, %%dx"
    :: "d" (dest_port), "a" ((WORD)input_data));    
    return;    
}

void outb(WORD dest_port, BYTE input_data)
{    
    DWORD value = input_data;
    asm volatile (
        "outb       %%al, %%dx"
    :: "d" (dest_port), "a" (value) : "%memory" );
    
    return;    
}

BYTE inb(WORD port)
{
    BYTE ret;    
    asm volatile (
        "inb        %%dx, %%al"
    : "=a" (ret) : "d" (port));
     return ret;
}

WORD inw(WORD port)
{
    WORD    ret;
    asm volatile (
        "inw        %%dx, %%ax"
    : "=a" (ret) : "d" (port));
    return ret;
}

DWORD inl(WORD port)
{
    DWORD   ret;
    asm volatile (
        "inl        %%dx, %%eax"
    : "=a" (ret) : "d" (port));
    return ret;
}

void delay(int n)
{
    int i = 0;    
    for( ; i < n ; i++) {
        //適当なポートに何かぶち込んでおけ
        inb(0x80);
    }    
}

void u_sleep(int usec)
{
    int i = 0;    
    for ( ; i < usec ; i++) {
        inb(0x80);
        inb(0x80);
    }    
}

void data_dump(void *buffer, int len, int col)
{    
    BYTE *ptr = (BYTE*)buffer;
    int i, j;
    
    for (i = 0, j = 0 ; i < len ; i++, j++) {
        if(j >= col){
            _sys_putchar('\n');
            j = 0;
        }
        _sys_printf(" %x ", *(ptr+i));
    }
    _sys_putchar('\n');
}
