
#include	"i8259A.h"
#include	"io.h"

//IRQラインを有効にする
static void enable_8259A_IRQ(DWORD irq_num);

//IRQラインを無効にする（割り込みを受け付けない）
static void disable_8259A_IRQ(DWORD irq_num);

static void mask_and_ack_8259A(DWORD irq_num);

//EOIを指定IRQ番号に送る
static void end_8259A_IRQ(DWORD irq_num);


//ここ書き換えたら他のPICでもOKIIZO
pic_handler_type	pic_handlers = {
	"i8259A",
	enable_8259A_IRQ,
	disable_8259A_IRQ,
	mask_and_ack_8259A,
	end_8259A_IRQ
};


static void enable_8259A_IRQ(DWORD irq_num){
	
	BYTE	mask;
	/*
	
	メモ：
	ビットをクリアしたら無効
	
	*/
	if(irq_num <= 7){
		
		mask = ~(1 << irq_num);
		mask &= inb(PIC_MASTER_PORT_B);
		
		outb(PIC_MASTER_PORT_B, mask);
		
	}else{
		
		mask = ~(1 << (irq_num - 8));
		mask &= inb(PIC_SLAVE_PORT_B);
		
		outb(PIC_SLAVE_PORT_B, mask);
		
	}
	
}


static void disable_8259A_IRQ(DWORD irq_num){
	/*
	
	メモ：
	ビットをセットしたら有効
	
	*/
	BYTE	mask;
	
	if(irq_num <= 7){
		
		mask = (1 << irq_num);
		mask |= inb(PIC_MASTER_PORT_B);
		
		outb(PIC_MASTER_PORT_B, mask);
	}else{
		
		mask = (1 << (irq_num - 8));
		mask |= inb(PIC_SLAVE_PORT_B);
		
		outb(PIC_SLAVE_PORT_B, mask);
	}
	
}


static void mask_and_ack_8259A(DWORD irq_num){
	
	/* 
		とりあえず同一IRQからの割り込みは受け付けない
		そんなのおこるわけねーって！？、いゃーおこるかもしれんぞよ...
	*/
	disable_8259A_IRQ(irq_num);
	
}

/* EOI送信 */
static void end_8259A_IRQ(DWORD irq_num){
	
	BYTE	value;
	BYTE	slave_isr;
	
	if(irq_num <= 7){
		
		/* マスターだけに送ればいい */
		value = 0x60 + (irq_num & 7);
		outb(PIC_MASTER_PORT_A, value);
		
	}else{
		
		value = (irq_num - 8) + 0x60;
		outb(PIC_SLAVE_PORT_A, value);
		
		/* slave isrチェック */
		outb(PIC_SLAVE_PORT_A, 0x0b);
		slave_isr = inb(PIC_SLAVE_PORT_A);
		
		/* slave picは他の割り込みを受け付けていない */
		if(slave_isr==0x00){
			outb(PIC_MASTER_PORT_A, 0x62); /* IRQ2番はSlavePICがつながっている */
		}
		
	}
	
}

void disable_IRQ_line(DWORD irq_num){
	
	disable_8259A_IRQ(irq_num);
	
}


void enable_IRQ_line(DWORD irq_num){
	
	enable_8259A_IRQ(irq_num);
	
}


/*
	そのIRQラインの呼び出しが終わっている: 1
	もしくはまだ終わってない場合: 0
*/
int check_irq_state(DWORD irq_num){
	
	BYTE	isr_value;
	
	if(irq_num <= 7){
		
		/* master isrチェック */
		outb(PIC_MASTER_PORT_A, 0x0b);
		isr_value = inb(PIC_MASTER_PORT_A);
		
		if(isr_value & (0x1 << irq_num)){
			return 0;
		}else{
			return 1;
		}
		
	}else{
		
		/* slave isrチェック */
		outb(PIC_SLAVE_PORT_A, 0x0b);
		isr_value = inb(PIC_SLAVE_PORT_A);
		
		if(isr_value & (0x1 << (irq_num - 8))){
			return 0;
		}else{
			return 1;
		}
		
	}
	
}


void _sys_init_8259A(void){
	
	_sys_printk("PIC Setting...");
	
	/* Master PIC Init */
	outb(PIC_MASTER_PORT_A, 0x11);		/* setting ICW1 */
	outb(PIC_MASTER_PORT_B, 0x20 + 0);  /* setting ICW2 */
	/* IR0-7 mapped to 0x20-0x27 (master) */
	outb(PIC_MASTER_PORT_B, 0x04);		/* setting ICW3 */
	outb(PIC_MASTER_PORT_B, 0x01);		/* setting ICW4 */
 	
	/* Slave PIC Init */
	outb(PIC_SLAVE_PORT_A, 0x11);		/* setting ICW1 */
	outb(PIC_SLAVE_PORT_B, 0x20 + 8);	/* setting ICW2 */
	/* IR0-7 mapped to 0x28-0x2f (slave) */
	outb(PIC_SLAVE_PORT_B, 0x02);		/* setting ICW3 */
	outb(PIC_SLAVE_PORT_B, 0x01);		/* setting ICW4 */
	
	
	/* 全マスク */
	outb(PIC_MASTER_PORT_B, 0xFF);
	outb(PIC_SLAVE_PORT_B, 0xFF);
	
	/* とりあえずいくつかの割り込み許可 */
	enable_8259A_IRQ(1);		/* key board */
	enable_8259A_IRQ(2);		/* slave PIC */
	enable_8259A_IRQ(6);		/* Floppy drive */
	enable_8259A_IRQ(14);		/* primary IDE */
	enable_8259A_IRQ(15);		/* secondary IDE */
	enable_8259A_IRQ(8);		/* */
	
	_sys_printk("Done\n");
	
}


void _sys_irq_all_enable(void){
	
	outb(PIC_MASTER_PORT_B, 0x0);
	outb(PIC_SLAVE_PORT_B, 0x0);
	
}

