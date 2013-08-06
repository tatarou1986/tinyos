
#include		"irq.h"
#include		"io.h"

#define			ELCR1		0x4d0
#define			ELCR2		0x4d1

int _sys_irq_set_edge_trigger(DWORD irq_num)
{
	BYTE ret;
    
	if (irq_num <= 7) {
		/* 一部のレガシーデバイスIRQは付加 */
		if (irq_num == 1 || irq_num == 2) {
			return 0;
		}
		ret = inb(ELCR1);
		
		ret &= ~(0x1 << irq_num);
		outb(ELCR1, ret);
	} else {
		if (irq_num == 13 || irq_num == 8) {
			return 0;
		}
		ret = inb(ELCR2);
		
		ret &= ~(0x1 << irq_num);
		outb(ELCR2, ret);
	}
	
	delay(2);
	
	return 1;
	
}


int _sys_irq_set_level_trigger(DWORD irq_num)
{
	BYTE	ret;
	
	if(irq_num <= 7){
		/* 一部のレガシーデバイスIRQは不可 */
		if(irq_num==1 || irq_num==2){
			return 0;
		}
		ret = inb(ELCR1);
		
		ret |= (0x1 << irq_num);
		outb(ELCR1, ret);
		
	} else {
		if (irq_num == 13 || irq_num == 8) {
			return 0;
		}
		ret = inb(ELCR2);
		
		ret |= (0x1 << irq_num);
		outb(ELCR2, ret);
		
	}
	
	delay(2);
	
	return 1;
	
}

irq_info _sys_irq_info(DWORD irq_num)
{
	
	BYTE		ret;
	irq_info	info;
	
	if (irq_num <= 7) {
		/* 一部のレガシーデバイスIRQは不可 */
		if (irq_num==1 || irq_num==2) {
			info.interrupt_type = 0;
			return info;
		}
		
		ret = inb(ELCR1);
		info.interrupt_type = ((ret >> irq_num) & 0x1);
		return info;
		
	} else {
      
		if (irq_num == 13 || irq_num == 8) {
			info.interrupt_type = 0;
			return info;
		}
		
		ret = inb(ELCR2);
		info.interrupt_type = ((ret >> irq_num) & 0x1);
		return info;
	}
}


