

#ifndef	ethernetH
#define	ethernetH

#include	"kernel.h"


#define		MAC_ADDR_SIZE			6
#define		CRC_SIZE				(sizeof(DWORD))
#define		MAX_PACKET_DATA_SIZE	1500
#define		MIN_PACKET_DATA_SIZE	46


typedef struct __802_3_raw {
	BYTE		_dest_mac_addr[MAC_ADDR_SIZE];
	BYTE		_src_mac_addr[MAC_ADDR_SIZE];
	WORD		_length;
} __attribute__ ((packed)) _802_3_raw;

typedef struct _ethernet_addr {
	BYTE		byte[6];
} ethernet_addr;


int _sys_init_ethernet();

int make_8023_client();

int send_8023(void *buffer, int n, ethernet_addr *to, ethernet_addr *from);

/* データが着たらたたき起こされる */
int listen_8023_server(int (*f)(void*, ethernet_addr*, int), DWORD flag);

int destroy_8023_server();

int destroy_8023_client();

void clear_recv_buffer();

ethernet_addr eth_addr(char *addr_str);

ethernet_addr get_host_mac();

void regist_eth_addr(ethernet_addr);

void start_packet_store();


/* ビッグエンディアン <-> リトルエンディアン */
DWORD htonl(DWORD hostlong);

WORD htons(WORD hostshort);

DWORD ntohl(DWORD netlong);

WORD ntohs(WORD netshort);

#endif

