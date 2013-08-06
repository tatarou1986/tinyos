
#include		"clock.h"
#include		"gdebug.h"
#include		"idt.h"
#include		"vga.h"
#include		"i8259A.h"
#include		"task.h"


void clock_handler(void);
void init_clock(void);

device_operator		clock_operator = {
	0,
	"system clock",
	0,
	&init_clock,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


void init_clock(void){
	
	/*
	WORD			value = 11932;
	
	cli();
	
	_sys_set_irq_handle(0, &clock_handler);
	
	enable_IRQ_line(0);
	
	outb(0x43, 0x34);
	outb(0x40, value & 0xff);
	outb(0x40, value >> 8);
	
	sti();
	
	*/
	
}


void clock_handler(void){
	
	//_sys_printf("clock!\n");
	
	if(exist_task()){
		/* vmが実行すべきタスクが存在する */		
		exec_task();
	}
	
}



