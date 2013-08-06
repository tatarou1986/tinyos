
#include "packet.h"
#include "system.h"
#include "memory.h"
#include "device.h"
#include "gdebug.h"
#include "ethernet.h"

static pkt_man_hdr *rv_root, *tx_root;
static FILE        eth_desc;
static DWORD       total_pkt_len;

int init_packet_que(void)
{    
    rv_root = tx_root = NULL;
    eth_desc = open("/dev/eth0");
    
    if (eth_desc==-1) {
        _sys_printf("cannot find useful ethernet device\n");
        return 0;
    }
    
    total_pkt_len = 0;
    return 1;
}


int in_trans_packet_que(const BYTE *buffer, int size)
{
    pkt_man_hdr     *ptr, *new_ptr;
    
    //_sys_printf(" trans que in\n");
    new_ptr = (pkt_man_hdr*)_sys_kmalloc(sizeof(pkt_man_hdr));
    
    if (new_ptr == NULL) {
        return 0;
    }
    
    new_ptr->next   = NULL;
    new_ptr->p_len  = size;
    new_ptr->buffer = (BYTE*)_sys_kmalloc(size);
    
    if ((new_ptr->buffer) == NULL) {
        return 0;
    }
    
    _sys_memcpy(new_ptr->buffer, buffer, size);
    
    ptr = tx_root;
    
    if (ptr != NULL) {
      while (ptr->next!=NULL) {
            ptr = ptr->next;
        }
        ptr->next = new_ptr;
    }else{
        tx_root = new_ptr;
    }
    
    //_sys_printf(" trans que ok\n");
    return 1;
}

int in_recv_packet_que(BYTE *buffer, int size)
{
    pkt_man_hdr     *ptr, *new_ptr;
    
    new_ptr = (pkt_man_hdr*)_sys_kmalloc(sizeof(pkt_man_hdr));
    
    if (new_ptr == NULL) {
        _sys_printf("recv malloc error!\n");
        return 0;
    }
    
    new_ptr->next   = NULL;
    new_ptr->p_len  = size;
    new_ptr->buffer = buffer;
    
    total_pkt_len   += size;
    
    ptr = rv_root;
    
    if (ptr != NULL) {
        while(ptr->next!=NULL){
            ptr = ptr->next;
        }
        ptr->next = new_ptr;
    } else {
        rv_root = new_ptr;
    }
    
    return 1;    
}



int send_packet(void)
{
    pkt_man_hdr *ptr, *tmp;
    
    /* デバイス依存ですねぇ.. */    
    for (ptr = tx_root ; !(ptr==NULL) ; ) {
        
        //_sys_printf(" send!\n");
        
        /* 一気にethに書き出す */
        write(eth_desc, ptr->p_len, ptr->buffer);
        
        /* パケットのバッファを片付ける */
        _sys_kfree(ptr->buffer);        
        tmp = ptr->next;
        _sys_kfree(ptr);
        ptr = tmp;
    }
    
    tx_root = NULL;
    
    return 1;
    
}

/*
int read_packet(BYTE *buffer, int *size){
    
    pkt_man_hdr     *ptr;
    DWORD           length = 0;
    
    for(ptr=rv_root;!(ptr==NULL);ptr=ptr->next){
        
        _sys_memcpy(buffer, ptr->buffer, ptr->p_len);
        
        length += ptr->p_len;
        buffer += ptr->p_len;
        
        total_pkt_len -= ptr->p_len;
    }
    
    *size = length;
    
    return length;
}
*/

int read_packet(BYTE *buffer, int *size)
{    
    pkt_man_hdr *ptr = rv_root;
    int         length = 0;
    
    if (ptr != NULL) {
      
        length = ptr->p_len;
        total_pkt_len -= length;
        
        _sys_memcpy(buffer, ptr->buffer, length);
        
        if (ptr->next != NULL) {
            rv_root = ptr->next;
        } else {
            rv_root = NULL;
        }
        _sys_kfree(ptr->buffer);
        _sys_kfree(ptr);
    }
    
    *size = length;
    return length;
}


void clear_recv_buffer(void)
{
    pkt_man_hdr *ptr, *tmp;
    
    for(ptr = rv_root ; !(ptr==NULL) ; ) {
        
        /* パケットのバッファを片付ける */
        _sys_kfree(ptr->buffer);
        tmp = ptr->next;
        _sys_kfree(ptr);
        ptr = tmp;
    }
    
    total_pkt_len = 0;
    rv_root       = NULL;
    
    return;
}

DWORD chk_packet_len(void)
{    
    return total_pkt_len;
}
