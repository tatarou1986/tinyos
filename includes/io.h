
#ifndef ioH
#define ioH

#include		"kernel.h"


BYTE inb(WORD port);

WORD inw(WORD port);

DWORD inl(WORD port);

void outb(WORD dest_port, BYTE input_data);

void outw(WORD dest_port, WORD input_data);

void outl(WORD dest_port, DWORD input_data);

/* íxâÑÇî≠ê∂Ç≥ÇπÇÈ */
void delay(int n);

void u_sleep(int usec);

/*  */
void data_dump(void *buffer, int len, int col);

#endif
