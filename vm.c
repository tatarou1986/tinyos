
#include "vm.h"
#include "cpuseg_emu.h"
#include "ctrl_reg_emu.h"
#include "system.h"
#include "gdebug.h"
#include "memory.h"
#include "page_emu.h"
#include "i8259A_emu.h"
#include "idt.h"
#include "ethernet.h"
#include "vga.h"

static void find_vm();
static void all_release_irq();

int test_send(char *data, char *mac_addr, int len);
static int get_packet(void *buffer, ethernet_addr *header, int length);
static void send_my_device_list(ethernet_addr *dest);

static void send_out(ethernet_addr *dest, BYTE out_type, WORD io_addr, DWORD out_data);

static void send_my_byte_io(ethernet_addr *dest);
static void send_my_word_io(ethernet_addr *dest);
static void send_my_dword_io(ethernet_addr *dest);

static void test_out_43(BYTE value);
static void test_out_42(BYTE value);
static void test_out_61(BYTE value);

static void io_vm_out_byte(void *buffer, ethernet_addr *header);
static void io_vm_out_word(void *buffer, ethernet_addr *header);
static void io_vm_out_dword(void *buffer, ethernet_addr *header);

static void play_elise();

static void beep_on();
static void beep_off();


static emulation_handler        vm_eth_out[] = {
    {0x43, &test_out_43, NULL, NULL, NULL, NULL, NULL},
    {0x42, &test_out_42, NULL, NULL, NULL, NULL, NULL},
    {0x61, &test_out_61, NULL, NULL, NULL, NULL, NULL}
};


void _sys_init_vm(void)
{
    _sys_printf("vm Initialize...\n");
    
    cpu_init_segment_emu();
    cpu_init_ctrlreg_emu();
    cpu_init_page_emu();
    
    //play_elise();
    
    //__STOP();
    
    /* ethernet上から他のVMを探す */
    find_vm();
    
    //__STOP();
    
    init_i8259A_emu();
    
    all_release_irq();
    
    _sys_printf("Done\n");  
}


static void find_vm(void)
{
    ethernet_addr to          = eth_addr("ffffffffffff");
    DWORD         packet_data = VM_PACKET_SIGN;
    int           i;
    
    //listen_8023_server(&get_packet, 0);
    
    /* こんにちは */
    /*
    for(i=0;i<50;i++){
        send_vm_data((void*)&packet_data, &to, sizeof(packet_data), VM_CMD_HELLO);
    }
    */
    //send_vm_data((void*)&packet_data, &to, sizeof(packet_data), VM_CMD_HELLO);
    //send_my_device_list(&to);
    
    /*
    start_listen_io(&vm_eth_out[0]);
    start_listen_io(&vm_eth_out[1]);
    start_listen_io(&vm_eth_out[2]);
    */
}


static void wait_vm_reply(void)
{
    int i = 0;
    
    _sys_cls();
    
    _sys_printf("------------------------");
    _sys_printf("searching...\\");
    
    for ( ; i < VM_TIMEOUT ; i++) {
        _sys_printf("\b|\b/\b-\b\\");
        u_sleep(1000);
    }
    
    _sys_printf("\bdone\n");
    _sys_printf("------------------------");
}


static void all_release_irq(void)
{
    _sys_release_irq_line(0);
    _sys_release_irq_line(1);
    _sys_release_irq_line(2);
    _sys_release_irq_line(3);
    _sys_release_irq_line(4);
    _sys_release_irq_line(5);
    _sys_release_irq_line(6);
    _sys_release_irq_line(7);
    _sys_release_irq_line(8);
    _sys_release_irq_line(9);
    _sys_release_irq_line(10);
    _sys_release_irq_line(11);
    _sys_release_irq_line(12);
    _sys_release_irq_line(13);
    _sys_release_irq_line(14);
    _sys_release_irq_line(15);
}



int send_vm_data(void *data, ethernet_addr *to, int len, BYTE op_code_flag)
{
    ethernet_addr from;
    vm_packet_hdr hdr;
    void *buffer;
    int  ret;
    
    from = get_host_mac();
    
    hdr.op_code     = op_code_flag;
    hdr.payload_len = len;
    hdr.sign        = VM_PACKET_SIGN;
    
    buffer = _sys_kmalloc(sizeof(vm_packet_hdr) + len);
    
    if (buffer == NULL) {
        return 0;
    }
    
    _sys_memcpy(buffer, &hdr, sizeof(vm_packet_hdr));
    _sys_memcpy((buffer + sizeof(vm_packet_hdr)), data, len);
    
    ret = send_8023(buffer, len + sizeof(vm_packet_hdr), to, &from);
    
    _sys_kfree(buffer);
    
    _sys_printf(" send ok\n");
    
    return ret;
}



int test_send(char *data, char *mac_addr, int len)
{
    ethernet_addr       to, from;
    BYTE                *buffer;
    
    to      = eth_addr(mac_addr);
    from    = get_host_mac();
    
    buffer = (BYTE*)_sys_kmalloc(len);
    
    _sys_memcpy(buffer, data, len);
    
    if (send_8023(buffer, len, &to, &from)) {
        return 1;
    }
    
    _sys_kfree(buffer);
    
    return 0;
}



static int get_packet(void *buffer, ethernet_addr *header, int length)
{
    vm_packet_hdr *hdr = (vm_packet_hdr*)buffer;
    DWORD         *tmp;
    
    if (hdr->sign == VM_PACKET_SIGN) {
        
        _sys_printf(" vm get packet: %d [bytes]\n", length);
        
        switch (hdr->op_code) {
            case VM_CMD_MYDEV:
                _sys_printf("-----------------------------------\n");
                /*
                _sys_printf(" From: %x:%x:%x:%x:%x:%x\n",
                    header->byte[0],
                    header->byte[1],
                    header->byte[2],
                    header->byte[3],
                    header->byte[4],
                    header->byte[5]
                );
                */
                _sys_printf(buffer + sizeof(vm_packet_hdr));
                _sys_printf("-----------------------------------\n");
                
                break;
                
            case VM_CMD_HELLO:
                _sys_printf(" ok...send my list\n");
                tmp = (buffer + sizeof(vm_packet_hdr));
                if (*tmp==VM_PACKET_SIGN) {
                    /* デバイスリストを送り返す */
                    send_my_device_list(header);
                }
                       break;
            case VM_CMD_GET_IN_B:
                /* VMのエミュレートを行いますよ */
                _sys_printf(" io in byte\n");
                break;
            case VM_CMD_OUT_B:
                io_vm_out_byte(buffer, header);
                break;
            case VM_CMD_OUT_W:
                io_vm_out_word(buffer, header);
                break;
            case VM_CMD_OUT_DW:
                io_vm_out_dword(buffer, header);
                break;
                
        }
        return 1;
    }
    return 0;
}

static void io_vm_out_byte(void *buffer, ethernet_addr *header)
{
    eth_io_hdr      *hdr = (eth_io_hdr*)(buffer + sizeof(vm_packet_hdr));
    _sys_printf(" out %x, %x\n", hdr->io_addr, hdr->value);
    outb(hdr->io_addr, hdr->value);
}

static void io_vm_out_word(void *buffer, ethernet_addr *header)
{
    eth_io_hdr      *hdr = (eth_io_hdr*)(buffer + sizeof(vm_packet_hdr));
    outw(hdr->io_addr, hdr->value);
}

static void io_vm_out_dword(void *buffer, ethernet_addr *header)
{
    eth_io_hdr      *hdr = (eth_io_hdr*)(buffer + sizeof(vm_packet_hdr));
    outl(hdr->io_addr, hdr->value);
}

static void send_my_device_list(ethernet_addr *dest)
{    
    int         i;
    char        *test[] = {
        "super device1\n",
        "super device2\n",
        "super device3\n",
        "super device4\n",
        "super device5\n"
    };
    
    for (i = 0 ; i < 5 ; i++) {        
        _sys_printf(" send!...");
        if (send_vm_data(test[i], dest, _sys_strlen(test[i]) + 1, 
                         VM_CMD_MYDEV)) {
            _sys_printf("ok\n");
        } else {
            _sys_printf("false\n");
        }
    }    
    
}


static void send_out(ethernet_addr *dest, BYTE out_type, 
                     WORD io_addr, DWORD out_data)
{    
    eth_io_hdr data;
    BYTE type;
    
    data.TYPE     = out_type;
    data.io_addr  = io_addr;
    data.value    = out_data;
    
    switch(out_type){
        case TYPE_IO_BYTE:
            type    = VM_CMD_OUT_B;
            break;
            
        case TYPE_IO_WORD:
            type    = VM_CMD_OUT_W;
            break;
            
        case TYPE_IO_DWORD:
            type    = VM_CMD_OUT_DW;
            break;
            
        default:
            return;
    }
    
    _sys_printf("send!\n");
    
    send_vm_data(&data, dest, sizeof(eth_io_hdr), type);
    
}


static void play_elise(void)
{    
    WORD    count;
    
    beep_off();
    
    /* ミ */
    outb(0x43, 0xb6);
    count = 1193180 / 275;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    
    u_sleep(1000);
    
    beep_off();
    
    /* レ */
    outb(0x43, 0xb6);
    count = 1193180 / 247;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    
    u_sleep(1000000);
    
    beep_off();
    
    /* ミ */
    outb(0x43, 0xb6);
    count = 1193180 / 275;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    
    u_sleep(1000000);
    
    beep_off();
    
    /* レ */
    outb(0x43, 0xb6);
    count = 1193180 / 247;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    
    u_sleep(1000000);
    
    beep_off();
    
    /* ミ */
    outb(0x43, 0xb6);
    count = 1193180 / 275;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    u_sleep(1000000);
    
    beep_off();
    
    /* シ */
    outb(0x43, 0xb6);
    count = 1193180 / 415;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    u_sleep(1000000);
    
    beep_off();
    
    /* レ */
    outb(0x43, 0xb6);
    count = 1193180 / 247;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    u_sleep(1000000);
    
    beep_off();
    
    /* ド */
    outb(0x43, 0xb6);
    count = 1193180 / 220;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    u_sleep(1000000);
    
    beep_off();
    
    /* ラ */
    outb(0x43, 0xb6);
    count = 1193180 / 367;
    outb(0x42, count & 0xff);
    outb(0x42, count >> 8);
    
    beep_on();
    u_sleep(2000000);
    
    beep_off();
    
}

static void beep_on(void)
{
    BYTE    value;
    
    value = inb(0x61);
    value |= 0x03;
    value &= 0x0f;
    
    _sys_printf("beep on; %x\n", value);
    
    outb(0x61, value);
}


static void beep_off(void)
{
    BYTE    value;
    
    value = inb(0x61);
    value &= 0xd;
    outb(0x61, value);
    
    _sys_printf("beep off; %x\n", value);
}


static void test_out_43(BYTE value)
{
    
    ethernet_addr       to = eth_addr("b0c420000001");
    //ethernet_addr     to = eth_addr("b0c455000000");
    
    _sys_printf("0x43 out\n");
    send_out(&to, TYPE_IO_BYTE, 0x43, value);
}

static void test_out_42(BYTE value)
{
    ethernet_addr       to = eth_addr("b0c420000001");
    
    _sys_printf("0x42 out\n");
    send_out(&to, TYPE_IO_BYTE, 0x42, value);
    
}

static void test_out_61(BYTE value)
{
    ethernet_addr to = eth_addr("b0c420000001");
    
    _sys_printf("0x61 out\n");
    send_out(&to, TYPE_IO_BYTE, 0x61, value);
    
}



