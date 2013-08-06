
#include	"kernel.h"

typedef struct _pkt_man_hdr {
	struct _pkt_man_hdr		*next;
	WORD					p_len;
	BYTE					*buffer;
} pkt_man_hdr;


int init_packet_que();

/* このbufferは */
int in_trans_packet_que(const BYTE *buffer, int size);

/* パケットを受信リストに次々に登録してゆく */
int in_recv_packet_que(BYTE *buffer, int size);

int send_packet();

DWORD chk_packet_len();

int read_packet(BYTE *buffer, int *size);


