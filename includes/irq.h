
#include		"kernel.h"
#include		"i8259A.h"

typedef struct _irq_info {
	/* 
		0:Edge Trigger
		1:Level Trigger
	 */
	DWORD		interrupt_type;
} irq_info;

/* Edgeトリガーにしちゃうぞっと */
int _sys_irq_set_edge_trigger(DWORD irq_num);

/* levelトリガーにしちゃうぞっと */
int _sys_irq_set_level_trigger(DWORD irq_num);

/* これを使ってirq_numで示されるIRQのトリガーモードをチェックする */
irq_info _sys_irq_info(DWORD irq_num);

