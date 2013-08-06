
#ifndef i8259AH
#define i8259AH

#include		"kernel.h"

#define		PIC_MASTER_PORT_A		0x20
#define		PIC_MASTER_PORT_B		0x21

#define		PIC_SLAVE_PORT_A		0xA0
#define		PIC_SLAVE_PORT_B		0xA1

typedef struct _pic_handler_type {
	const char		*pic_typename;
	void 			(*enable)(DWORD irq_num);
	void 			(*disable)(DWORD irq_num);
	void 			(*ack)(DWORD irq_num);
	void 			(*end)(DWORD irq_num);
} pic_handler_type;

//PICドライバ用ハンドラ
extern	pic_handler_type		pic_handlers;

void _sys_init_8259A(void);

void disable_IRQ_line(DWORD irq_num);
void enable_IRQ_line(DWORD irq_num);

void _sys_irq_all_enable(void);


/*
	そのIRQラインの呼び出しが終わっている: 1
	もしくはまだ終わってない場合: 0
*/
int check_irq_state(DWORD irq_num);

#endif

