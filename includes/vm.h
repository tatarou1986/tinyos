
#ifndef	vmH
#define	vmH

#include		"kernel.h"
#include		"ethernet.h"

#define		VM_PACKET_MAX_LEN		(MAX_PACKET_DATA_SIZE - sizeof(vm_packet_hdr))
#define		VM_PACKET_MIN_LEN		(MIN_PACKET_DATA_SIZE - sizeof(vm_packet_hdr))

#define		VM_PACKET_SIGN			0xaaab

/* VM packet commands */
#define		VM_CMD_HELLO			0x01
#define		VM_CMD_MYDEV			0xfe
#define		VM_CMD_GET_IN_B			0x02
#define		VM_CMD_GET_IN_W			0x03
#define		VM_CMD_GET_IN_DW		0x04
#define		VM_CMD_RES_IN_B			(~0x02)
#define		VM_CMD_RES_IN_W			(~0x03)
#define		VM_CMD_RES_IN_DW		(~0x04)

#define		VM_CMD_OUT_B			0x05
#define		VM_CMD_OUT_W			0x06
#define		VM_CMD_OUT_DW			0x07

#define		VM_CMD_GET_INTx_DATA	0x08
#define		VM_CMD_RES_INTx_DATA	(~0x08)


/* ms */
#define		VM_TIMEOUT			10


#define		TYPE_IO_BYTE		0xaa
#define		TYPE_IO_WORD		0xbb
#define		TYPE_IO_DWORD		0xcc


typedef struct _vm_packet_hdr {
	BYTE		op_code;
	WORD		sign;			/* always 0xaaab */
	WORD		payload_len;
} __attribute__ ((packed)) vm_packet_hdr;


typedef struct _vm_client {
	ethernet_addr	src_addr;
	DWORD			total_receive_len;
	DWORD			payload_len;
} vm_client;


typedef struct _eth_io_hdr {
	BYTE		TYPE;
	WORD		io_addr;
	DWORD		value;
} eth_io_hdr;

void _sys_init_vm();

int send_vm_data(void *data, ethernet_addr *to, int len, BYTE op_code_flag);

#endif

