
#ifndef i8259A_emuH
#define i8259A_emuH

#include		"kernel.h"
#include		"resource.h"


void init_i8259A_emu(void);

/* 0x20 */
void i8259A_master_out_port_A(BYTE value);
BYTE i8259A_master_in_port_A(void);

/* 0x21 */
void i8259A_master_out_port_B(BYTE value);
BYTE i8259A_master_in_port_B(void);

/* 0xA0 */
void i8259A_slave_out_port_A(BYTE value);
BYTE i8259A_slave_in_port_A(void);

/* 0xA1 */
void i8259A_slave_out_port_B(BYTE value);
BYTE i8259A_slave_in_port_B(void);

#endif

