
#include		"i8259A.h"
#include		"i8259A_emu.h"
#include		"tss.h"
#include		"irq_emu.h"
#include		"gdebug.h"
#include		"device.h"
#include		"idt.h"


#define		TYPE_MASTER		0
#define		TYPE_SLAVE		1

#define		_ICW1		0
#define		_ICW2		0
#define		_ICW3		1
#define		_ICW4		2
#define		_OCW1		3
#define		_OCW2		1
#define		_OCW3		2

#define		_ICW1_VALUE(type)			(type ? _slave_xCWx_A_value[_ICW1] : _master_xCWx_A_value[_ICW1])
#define		_ICW2_VALUE(type)			(type ? _slave_xCWx_B_value[_ICW2] : _master_xCWx_B_value[_ICW2])
#define		_ICW3_VALUE(type)			(type ? _slave_xCWx_B_value[_ICW3] : _master_xCWx_B_value[_ICW3])
#define		_ICW4_VALUE(type)			(type ? _slave_xCWx_B_value[_ICW4] : _master_xCWx_B_value[_ICW3])

#define		_OCW1_VALUE(type)			(type ? _slave_xCWx_B_value[_OCW1] : _master_xCWx_B_value[_OCW1])
#define		_OCW2_VALUE(type)			(type ? _slave_xCWx_A_value[_OCW2] : _master_xCWx_A_value[_OCW2])
#define		_OCW3_VALUE(type)			(type ? _slave_xCWx_A_value[_OCW3] : _master_xCWx_A_value[_OCW3])

#define		_OCW2_EOI_CMD		0x60

static emulation_handler	i8259A_emu[] = {
	
	{
		PIC_MASTER_PORT_A,
		&i8259A_master_out_port_A,
		&i8259A_master_in_port_A,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		PIC_MASTER_PORT_B,
		&i8259A_master_out_port_B,
		&i8259A_master_in_port_B,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		PIC_SLAVE_PORT_A,
		&i8259A_slave_out_port_A,
		&i8259A_slave_in_port_A,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		PIC_SLAVE_PORT_B,
		&i8259A_slave_out_port_B,
		&i8259A_slave_in_port_B,
		NULL,
		NULL,
		NULL,
		NULL
	}
	
};


/*
	0: OCW2
	1: OCW3
	2: ICW1
*/
static BYTE		_master_xCWx_A_value[3] = {0};
static BYTE		_slave_xCWx_A_value[3] 	= {0};

static BYTE		_master_xCWx_B_value[4] = {0};
static BYTE		_slave_xCWx_B_value[4] 	= {0};

static BYTE		_IRR[2] = {0};
static BYTE		_IMR[2] = {0};
static BYTE		_ISR[2] = {0};

//static BYTE		_idt_base_vector[2] = {0};

static int		_use_ICW4_flag[2] 		= {0};
static int		_next_reg_num[2] 		= {0};
static int		_IRR_ISR_next_read[2] 	= {0};


static void refresh_OCW3_status(int type);
static void refresh_OCW2_status(int type);
static void refresh_OCW1_status(int type);

static void refresh_ICW4_status(int type);
static void refresh_ICW3_status(int type);
static void refresh_ICW2_status(int type);
static void refresh_ICW1_status(int type);


void init_i8259A_emu(void){
	
	int						i;
	int						ret = 0;
	emulation_handler		*handler;
	
	_sys_printf(" virtual i8259A start...\n");
	_sys_printf(
		"  Listen io: %x %x %x %x\n",
		PIC_MASTER_PORT_A, PIC_MASTER_PORT_B,
		PIC_SLAVE_PORT_A, PIC_SLAVE_PORT_B
	);
	
	ret += start_listen_io(&i8259A_emu[0]);
	ret += start_listen_io(&i8259A_emu[1]);
	ret += start_listen_io(&i8259A_emu[2]);
	ret += start_listen_io(&i8259A_emu[3]);
	
	if(ret!=4){
		_sys_printf("  failed\n");
		_sys_freeze("io emulation malloc error\n");
	}
	
	_sys_printf(" done\n");
	
}



/* 0x20 */
void i8259A_master_out_port_A(BYTE value){
	
	/*
		10: ICW1
		
		00: OCW2
		01: OCW3
	*/
	
	if(value & 0x10){
		/* ICW1 */
		/* _ICW1_VALUE(TYPE_MASTER) = value; */
		_master_xCWx_A_value[_ICW1] = value;
		refresh_ICW1_status(TYPE_MASTER);
		
		_ISR[TYPE_MASTER] = _IRR[TYPE_MASTER] = 0x0;
		
		_IRR_ISR_next_read[TYPE_MASTER] = 1;
		
	}else{
		
		switch((value >> 3) & 0x3){
			
			case 0x0:
				/* OCW2 */
				/* _OCW2_VALUE(TYPE_MASTER) = value; */
				_master_xCWx_A_value[_OCW2] = value;
				refresh_OCW2_status(TYPE_MASTER);
				break;
				
			case 0x1:
				/* OCW3 */
				/* _OCW3_VALUE(TYPE_MASTER) = value; */
				_master_xCWx_A_value[_OCW3] = value;
				refresh_OCW3_status(TYPE_MASTER);
				break;
				
		}
		
	}

}


BYTE i8259A_master_in_port_A(void){
	
	if(_IRR_ISR_next_read[TYPE_MASTER]){
		return _IRR[TYPE_MASTER];
		/* IRR */
	}else{
		/* ISR */
		return _ISR[TYPE_MASTER];
	}
	
}


/* 0x21 */
void i8259A_master_out_port_B(BYTE value){
	
	/* 順番に書き込んでゆく */
	_master_xCWx_B_value[_next_reg_num[TYPE_MASTER]] = value;
	
	switch(_next_reg_num[TYPE_MASTER]){
		
		case 0x0:
			refresh_ICW2_status(TYPE_MASTER);
			break;
		
		case 0x1:
			refresh_ICW3_status(TYPE_MASTER);
			break;
			
		case 0x2:
			refresh_ICW4_status(TYPE_MASTER);
			break;
			
		case 0x3:
			refresh_OCW1_status(TYPE_MASTER);
			break;
			
	}
	
}


BYTE i8259A_master_in_port_B(void){
	
	return _IMR[TYPE_MASTER];
	
}



/* 0xA0 */
void i8259A_slave_out_port_A(BYTE value){
	
	/*
		10: ICW1
		
		00: OCW2
		01: OCW3
	*/
	
	if(value & 0x10){
		/* ICW1 */
		/* _ICW1_VALUE(TYPE_MASTER) = value; */
		_slave_xCWx_A_value[_ICW1] = value;
		refresh_ICW1_status(TYPE_SLAVE);
		
		_ISR[TYPE_SLAVE] = _IRR[TYPE_SLAVE] = 0x0;
		
		_IRR_ISR_next_read[TYPE_SLAVE] = 1;
		
	}else{
		
		switch((value >> 3) & 0x3){
			
			case 0x0:
				/* OCW2 */
				/* _OCW2_VALUE(TYPE_MASTER) = value; */
				_slave_xCWx_A_value[_OCW2] = value;
				refresh_OCW2_status(TYPE_SLAVE);
				break;
				
			case 0x1:
				/* OCW3 */
				/* _OCW3_VALUE(TYPE_MASTER) = value; */
				_slave_xCWx_A_value[_OCW3] = value;
				refresh_OCW3_status(TYPE_SLAVE);
				break;
				
		}
		
	}
	
}


BYTE i8259A_slave_in_port_A(void){
	
	if(_IRR_ISR_next_read[TYPE_SLAVE]){
		return _IRR[TYPE_SLAVE];
		/* IRR */
	}else{
		/* ISR */
		return _ISR[TYPE_SLAVE];
	}
	
}

/* 0xA1 */
void i8259A_slave_out_port_B(BYTE value){
	
	/* 順番に書き込んでゆく */
	_slave_xCWx_B_value[_next_reg_num[TYPE_SLAVE]] = value;
	
	switch(_next_reg_num[TYPE_SLAVE]){
		
		case 0x0:
			refresh_ICW2_status(TYPE_SLAVE);
			break;
		
		case 0x1:
			refresh_ICW3_status(TYPE_SLAVE);
			break;
			
		case 0x2:
			refresh_ICW4_status(TYPE_SLAVE);
			break;
			
		case 0x3:
			refresh_OCW1_status(TYPE_SLAVE);
			break;
	}
	
}

BYTE i8259A_slave_in_port_B(void){
	
	return _IMR[TYPE_SLAVE];
	
}


static void refresh_ICW1_status(int type){
	
	/* 初期化シークエンス開始,次はICW2として値を読む */
	_next_reg_num[type] = 2 - 2;
	
	_IRR_ISR_next_read[type] = 0;
	
	switch(type){
		
		case TYPE_MASTER:
			/* _master_ICW4_flag = MASTER_ICW1_VALUE & 0x1; */
			_use_ICW4_flag[TYPE_MASTER] = _ICW1_VALUE(TYPE_MASTER) & 0x1;
			break;
		
		case TYPE_SLAVE:
			/* _slave_ICW4_flag = SLAVE_ICW1_VALUE & 0x1; */
			_use_ICW4_flag[TYPE_SLAVE] = _ICW1_VALUE(TYPE_SLAVE) & 0x1;
			break;
	}
	
}


static void refresh_ICW2_status(int type){
	
	_next_reg_num[type] = 3 - 2;
	
	/* if(!(_ICW2_VALUE(type) & 0x7)){ */
		/* _idt_reg_num[type] = _ICW1_VALUE(type); */
		
	switch(type){
		case 0:
			regist_master_idt_base_addr(_ICW2_VALUE(type) & 0xf8);
			break;
			
		case 1:
			regist_slave_idt_base_addr(_ICW2_VALUE(type) & 0xf8);
			break;
	}
	
}


static void refresh_ICW3_status(int type){
	
	
	_next_reg_num[type] = 4 - 2;
	
	/* 初期化シークエンス */
	if(_use_ICW4_flag[type]){
		/* ICW4を使う! */
		_next_reg_num[type] = 4 - 2;
	}else{
		/* ICW4を使わない */
		_next_reg_num[type] = 3;
	}
	
}


static void refresh_ICW4_status(int type){
	
	/* 初期化シークエンス */
	_next_reg_num[type] = 3;
	
}


static void refresh_OCW1_status(int type){
	
	BYTE	irq_num;
	/* BYTE	pattern	= _IMR[type] ^ _OCW1_VALUE(type); */
	BYTE	value = _OCW1_VALUE(type);
	int		i		= 0;
	
	for(i=0;i<8;i++){
		
		irq_num = i + ((BYTE)type << 3);
		
		switch((value >> i) & 0x1){
			
			case 0:
				/* 割り込み有効 */
				enable_IRQ_line(irq_num);
				break;
				
			case 1:
				/* 割り込み無効 */
				disable_IRQ_line(irq_num);
				break;
		}
	}
	
}


static void refresh_OCW2_status(int type){
	
	BYTE	irq_num = _OCW2_VALUE(type) & 7;
	
	switch(_OCW2_VALUE(type) & 0xE0){
		
		case _OCW2_EOI_CMD:
			manual_eoi(irq_num);
			break;
			
	}
	
}


static void refresh_OCW3_status(int type){
	
	_IRR_ISR_next_read[type] = _OCW3_VALUE(type) & 0x1;
	
}


