
#include        "device.h"
#include        "FDCdrv.h"
#include        "keyboard.h"
#include        "ATAdrv.h"
#include        "system.h"
#include        "vga.h"
#include        "pci-main.h"
#include        "dp8390.h"
#include        "clock.h"
#include        "gdebug.h"
#include        "idt.h"

/* ここにデバイスを登録してゆく */
static device_operator      *device_slot[] = {
    &fdc_operator,          /* floppy drive */
    &keyboard_operator,     /* keyboard device*/
    &ide_operator,
    &ata00_operator,        /* ide primary */
    &ata01_operator,        /* ide secondary */
    &ata10_operator,
    &ata11_operator,
    &pci_skelton,
    &eth0_operator,
    &clock_operator
};

static const char       *device_descriptor[] = {
    "/dev/floppy0",     /* floppy A */
    "/dev/kbd0",        /* PS/2 keyboard */
    "/dev/ide",         /* IDE初期化時に使用する抽象デバイス */
    "/dev/ata00",       /* ata[0:0] device */
    "/dev/ata01",       /* ata[0:1] device */
    "/dev/ata10",       /* ata[1:0] device */
    "/dev/ata11",       /* ata[1:1] device */
    "/dev/pci",
    "/dev/eth0",
    "/dev/clock"
};

static device_node  *root = NULL;
static void __init_all_resource(device_node *ptr);

/* 木に登録 */
void insert_resource(device_node *ptr)
{    
    device_node **node = &root;
    
    while (*node != NULL) {
        if (ptr->handler->io_address < (*node)->handler->io_address) {
            if ((*node)->left == NULL) {
                node = &((*node)->left);
                break;
            }
            node = &((*node)->left);
            
        } else {
            if ((*node)->right == NULL) {
                node = &((*node)->right);
                break;
            }
            node = &((*node)->right);
        }
    }
    
    (*node) = ptr;
    ptr->right = ptr->left = NULL;
    
    return;
}

/* 適切なエミュレーションオペレーターをサーチする */
emulation_handler *search_resource(WORD io_port_addr)
{    
    device_node *node = root;
    
    while (node != NULL) {
        if (io_port_addr < node->handler->io_address) {
            if (node->left == NULL) {
                return NULL;
            }
            node = node->left;
        } else if (io_port_addr > node->handler->io_address) {
            if(node->right == NULL){
                return NULL;
            }
            node = node->right;
        } else {
            break;
        }
    }
    return (node->handler);
}

void _sys_init_all_device(void)
{    
    int cnt = sizeof(device_slot) / sizeof(DWORD);
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if (device_slot[i]->start != NULL) {
            _sys_printf("initialize %s...\n", device_slot[i]->device_name);            
            /* 初期化シークエンスを呼び出す */
            device_slot[i]->start();            
            _sys_printf("Done\n");
        }        
    }   
}

FILE open(const char *name)
{    
    int cnt = sizeof(device_slot) / sizeof(DWORD);
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if (strstr(device_descriptor[i], name)) {
            if (device_slot[i]->irq_num != -1) {
                /* IRQを使用するみたいです */
                _sys_vm_reserve_irq_line((DWORD)device_slot[i]->irq_num);
            }
            return i;
        }
    }
    return -1;
}

void close(FILE fd)
{
    /* IRQを開放する */
    if (device_slot[fd]->irq_num != -1) {
        _sys_release_irq_line((DWORD)device_slot[fd]->irq_num);
    }    
}

int ioctl(FILE dd, int request, ...)
{    
    va_list arg;
    int len;
    
    if (dd == -1) {
        return -1;
    }
    
    if (device_slot[dd]->ioctl != NULL) {
        va_start(arg, request);
        len = device_slot[dd]->ioctl(request, arg);
        va_end(arg);
        return len;
    }
    
    return 0;
}

size_t write(FILE dd, int write_size, const void *buffer)
{   
    if (dd == -1) {
        return -1;
    }
    
    if (device_slot[dd]->write != NULL) {
        if (device_slot[dd]->write(write_size, buffer)) {
            return write_size;
        }
    }
    return -1;
}

size_t read(FILE dd, int read_size, void *buffer)
{   
    if (dd == -1) {
        return -1;
    }
    
    if (device_slot[dd]->read != NULL) {
        if (device_slot[dd]->read(read_size, buffer)) {
            return read_size;
        }
    }
    return -1;
}


size_t seek(FILE dd, int pos, WORD flag)
{   
    if (dd == -1) {
        return -1;
    }
    
    if (device_slot[dd]->seek != NULL) {
        if (device_slot[dd]->seek(pos, flag)) {
            return pos;
        }
    }
    
    return -1;
    
}

