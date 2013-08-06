
#ifndef idtH
#define idtH

#include	"kernel.h"
#include	"i8259A.h"
#include	"stack.h"

//#define		i(n)	void i##n();

#define		IDTNUM			256

//IRQ0～IRQ15まで
#define		MAX_IRQ_NUM		16

#define		DISABLE_INT()		asm volatile ("cli")
#define		ENABLE_INT()		asm volatile ("sti")

#define		cli()				asm volatile ("cli")
#define		sti()				asm volatile ("sti")


#define		ENABLE_NMI()					\
		asm volatile (						\
			"movb	$0x0, %%al		\n"		\
			"outb	%%al, $0x70		\n"		\
		/* no output */ : /* no input */ )	\


#define		INTRR_FLAG			0xF00

#define		IRQ_PROC_TYPE_1		0x00	/* normal */
#define		IRQ_PROC_TYPE_2		0x01	/* 後処理(eoi)を自動的に発行しない */
#define		IRQ_PROC_TYPE_3		0x02	/* 全処理も後処理(eoi)も一切行わない */


typedef enum{
	IRQ_INPROGRESS = 1,	/* 現在処理中 */
	IRQ_DISABLE,		/* このIRQラインは呼び出し禁止 */
	IRQ_IDEL,			/* 呼び出すことが出来る */
	IRQ_VM_RESERVE,		/* VMが使用する */
	IRQ_RELEASE,			/* この状態はGuestのIRQにリダイレクションする */
	IRQ_GUEST_INPROGRESS
} IRQstatus;

typedef struct _IntGateDisc{
	unsigned short		offset_L;
	unsigned short		segment_select;
	unsigned short		type;
	unsigned short		offset_H;
} __attribute__ ((packed)) IntGateDisc;


typedef struct _IRQdatas{
	pic_handler_type	*pic_func;
	void				(*action_handler)(void);
	IRQstatus			status;
	BYTE				process_type;
}IRQdatas;

typedef struct _DescTblPtr{
	unsigned short		limit;		//16bits
	unsigned int		base;		//32bits
} __attribute__ ((packed)) DescTblPtr;


void _sys_SetUpIDT();

void _sys_init_IRQn();

void _sys_set_irq_handle(DWORD irq_num, void *handle_func);

void _sys_set_intrgate(DWORD vector_num, void *intrr_handle);

void _sys_set_trapgate(DWORD vector_num, void *trap_handle);

void _sys_vm_reserve_irq_line(DWORD irq_num);

void _sys_release_irq_line(DWORD irq_num);

void manual_eoi(DWORD irq_num);

void _sys_enable_irq_line(DWORD irq_num);

void _sys_disable_irq_line(DWORD irq_num);

void _sys_change_irq_proc_type(DWORD irq_num, BYTE type);

/* void IRQ_handler(pushed_regs regs, DWORD IRQ_num, DWORD error_code, ...); */

void IRQ_handler(pushed_regs regs, DWORD IRQ_num, ...);

#endif

