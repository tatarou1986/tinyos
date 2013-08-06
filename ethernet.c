
#include        "ethernet.h"
#include        "system.h"
#include        "packet.h"
#include        "memory.h"
#include        "gdebug.h"
#include        "i8259A.h"
#include        "task.h"


#define     ETHER_CRC_POLY_LE       0xedb88320


int (*eth_handler)(void*, ethernet_addr*, int);
void print_mac_addr(BYTE *mac_addr);

static DWORD            *crc32_table;
static ethernet_addr    _eth_mac_addr;


#define __USE_CRC_TABLE

#ifdef __USE_CRC_TABLE

/*
 * This is for reference.  We have a table-driven version
 * of the little-endian crc32 generator, which is faster
 * than the double-loop.
*/
DWORD ether_crc32_le(const BYTE *buf, int len)
{   
    static const DWORD crctab[] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    };
    
    DWORD   crc;
    int     i;
    
    crc = 0xffffffffU;/* initial value */
    
    for (i = 0; i < len; i++) {
        crc ^= buf[i];
        crc = (crc >> 4) ^ crctab[crc & 0xf];
        crc = (crc >> 4) ^ crctab[crc & 0xf];
    }
    
    return (crc);
}

#else

DWORD ether_crc32_le(const BYTE *buf, int len)
{   
    DWORD       c, crc, carry;
    DWORD       i, j;
    
    crc = 0xffffffffU;/* initial value */
    
    for(i = 0; i < len; i++){
        c = buf[i];
        
        for (j = 0; j < 8; j++) {
            carry = ((crc & 0x01) ? 1 : 0) ^ (c & 0x01);
            crc >>= 1;
            c >>= 1;
            
            if(carry){
                crc = (crc ^ ETHER_CRC_POLY_LE);
            }
        }
    }
    
    return (crc);
}

#endif

int _sys_init_ethernet(void)
{
    /* packet queを初期化 */
    init_packet_que();
    /* ハンドラ初期化 */
    eth_handler = NULL;
    _sys_printf("ethernet ok\n");
    return 1;
}

int make_8023_client(void)
{
    return 1;    
}

/* 送信します！ */
int send_8023(void *buffer, int n, ethernet_addr *to, ethernet_addr *from)
{
    void        *buf, *ptr;
    int         i, cnt, len, real_len;
    int         header_size = sizeof(_802_3_raw) + 4;
    _802_3_raw  hdr;
    DWORD       crc;
    
    if (n <= 0) {
        return 0;
    }
    
    if((buf = _sys_kmalloc(MAX_PACKET_DATA_SIZE + header_size)) == NULL){
        _sys_printf("send malloc error!\n");
        return 0;
    }
    
    cnt = n / MAX_PACKET_DATA_SIZE;
    
    /* header下準備 */
    
    _sys_memcpy(hdr._dest_mac_addr, to->byte, MAC_ADDR_SIZE);
    _sys_memcpy(hdr._src_mac_addr, from->byte, MAC_ADDR_SIZE);
    
    for (i = 0,ptr = buffer ; i < cnt ; i++, 
           ptr += MAX_PACKET_DATA_SIZE) {
        
        hdr._length = htons(MAX_PACKET_DATA_SIZE);
        
        /* ヘッダーコピー */
        _sys_memcpy(buf, &hdr, sizeof(_802_3_raw));
        
        /* payload転送 */
        _sys_memcpy((buf + sizeof(_802_3_raw)), ptr, MAX_PACKET_DATA_SIZE);
        
        /* crcチェック */
        crc = ether_crc32_le(buf, sizeof(_802_3_raw) + MAX_PACKET_DATA_SIZE) ^ 0xffffffffU;
        
        /* CRCヘッダを書き込む */
        _sys_memcpy((buf + sizeof(_802_3_raw) + MAX_PACKET_DATA_SIZE), &crc, sizeof(DWORD));
        
        if(!(in_trans_packet_que(buf, MAX_PACKET_DATA_SIZE + header_size))){
            _sys_printf("send malloc error!\n");
            return 0;
        }
    }
    
    if ((cnt * MAX_PACKET_DATA_SIZE) != n) {
        
        /* len分追加で転送する */
        real_len = len = n - (cnt * MAX_PACKET_DATA_SIZE);
                
        if(real_len < MIN_PACKET_DATA_SIZE){
            len = MIN_PACKET_DATA_SIZE;
        }
        
        hdr._length = htons(len);
        
        /* ヘッダーコピー */
        _sys_memcpy(buf, &hdr, sizeof(_802_3_raw));
        
        /* payload転送 */
        _sys_memcpy((buf + sizeof(_802_3_raw)), ptr, real_len);
        
        /* crcチェック */
        crc = ether_crc32_le(buf, sizeof(_802_3_raw) + len) ^ 0xffffffffU;
        
        /* CRCヘッダを書き込む */
        _sys_memcpy(buf + sizeof(_802_3_raw) + len, (void*)&crc, sizeof(DWORD));
        
        if (!(in_trans_packet_que(buf, len + header_size))) {
            _sys_printf("send malloc error!\n");
            return 0;
        }
        
    }
    //_sys_printf(" uhaaa1\n");
    
    /* パケットを転送する */
    send_packet();
    
    //_sys_printf(" uhaaa2\n");
    _sys_kfree(buf);
    
    //_sys_printf(" uhaaa3\n");
    
    return n;
}

/* データが着たらたたき起こされる */
int listen_8023_server(int (*f)(void*, ethernet_addr*, int), DWORD flag)
{
    eth_handler = f;
    return 1;
}

int destroy_8023_server(void)
{
}

int destroy_8023_client(void)
{
}

DWORD htonl(DWORD hostlong)
{
    WORD hi, low;    
    hi = htons((hostlong >> 16) & 0xffff);
    low = htons(hostlong & 0xffff);    
    return ((low << 16) | hi);    
}

WORD htons(WORD hostshort)
{
    return (((hostshort >> 0x8) & 0xff) | ((hostshort << 8) & 0xff00));    
}

DWORD ntohl(DWORD netlong)
{
    return htonl(netlong);    
}

WORD ntohs(WORD netshort)
{    
    return htons(netshort);    
}

void print_mac_addr(BYTE *mac_addr)
{
    _sys_printf("%x:%x:%x:%x:%x:%x\n",
                mac_addr[0],mac_addr[1], mac_addr[2], 
                mac_addr[3],mac_addr[4],mac_addr[5]);    
}

ethernet_addr eth_addr(char *addr_str)
{    
    ethernet_addr   addr;    
    addr.byte[0] = (hex2bin(addr_str[0]) << 4) | hex2bin(addr_str[1]);
    addr.byte[1] = (hex2bin(addr_str[2]) << 4) | hex2bin(addr_str[3]);
    addr.byte[2] = (hex2bin(addr_str[4]) << 4) | hex2bin(addr_str[5]);
    addr.byte[3] = (hex2bin(addr_str[6]) << 4) | hex2bin(addr_str[7]);
    addr.byte[4] = (hex2bin(addr_str[8]) << 4) | hex2bin(addr_str[9]);
    addr.byte[5] = (hex2bin(addr_str[10]) << 4) | hex2bin(addr_str[11]);    
    return addr;    
}

ethernet_addr get_host_mac(void)
{
    return _eth_mac_addr;
}


void regist_eth_addr(ethernet_addr mac)
{
    _eth_mac_addr = mac;    
}

void start_packet_store(void)
{
    BYTE            *buf;
    int             length;
    _802_3_raw      *packet_header;
    ethernet_addr   hdr;
    
    /* check用のコード */
    if (!check_irq_state(11)) {
        /* まだEthernetの割り込み終わってない */
        delete_task();
        /* 再登録 */
        in_task_que(&start_packet_store);
        return;
    }    
    
    if (!chk_packet_len()) {
        return;
    }
    
    if ((buf = (BYTE*)_sys_kmalloc(1518)) == NULL) {
        _sys_printf("packet malloc error!!");
        return;
    }
    
    /* 全部読み出す */
    while ((read_packet(buf, &length)) != 0) {
        packet_header = (_802_3_raw*)buf;
        _sys_memcpy((void*)&hdr, (buf + sizeof(ethernet_addr)), sizeof(ethernet_addr));
        if(eth_handler!=NULL){
            eth_handler((buf + sizeof(_802_3_raw)), &hdr, length);
        }
    }    
    _sys_kfree(buf);    
}
