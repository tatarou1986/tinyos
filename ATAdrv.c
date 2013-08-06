
#include "ATAdrv.h"
#include "ATAemu.h"
#include "io.h"
#include "idt.h"
#include "system.h"
#include "vga.h"
#include "memory.h"
#include "gdebug.h"

#define DEVICE_ENABLE_RESET  0x6
#define DEVICE_DISABLE_RESET 0x2

#define ATA_CMD_DEVICE_RESET 0x08
#define ATA_CMD_IDENTIFY_DEV 0xec

/* #define T_400NS_WAIT 3 */
/* #define T_5MS_WAIT   3000 */
#define T_400NS_WAIT 3
#define T_5MS_WAIT   3000

//最大繰り返し数
#define MAX_WAIT_COUNT 500
#define RETRY_MAX      500

#define IDE_DEVICE_NUM 4

#define MASTER_DRV 0
#define SLAVE_DRV  1

typedef enum {
    DEVICE_CANNOT_USE,
    DEVICE_UNKNOW,
    DEVICE_ATA,
    DEVICE_ATAPI
} DEVICE_TYPE;

typedef enum {
    INTR_UNKNOW,            /* わけわかめ */
    
    INTR_STATUS_READY,      /* statusレジスタが読めるようになりました */
    INTR_STATUS_WAIT,       /* statusレジスタが読めるまで待機中 */
    
} INTR_TYPE;

/*
 *
 * prime_or_second:
 *  primary is 0, secondary is 1
 * device_num:
 * master is 0, slave is 1
 * type:
 *   ATA or ATAPI or unknow or not connected
*/
typedef struct _DRIVE {
    WORD            prime_or_second;        /* primary:0 secondary:1    */
    WORD            device_num;             /* device:1                 */
    BYTE            pio_mode_num;           /* pio mode num             */
    BYTE            subcmd_code;            /* pio sub cmd              */
    int             seek_pos;
    
    WORD            logical_cylinder_num;
    WORD            logical_head_num;
    WORD            logical_sector_num;
    
    char            id[41];
    DEVICE_TYPE     type;
} DRIVE;

typedef struct _ata_regs {
    BYTE        features;
    BYTE        sector_cnt;
    BYTE        sector_num;
    BYTE        cylinder_low;
    BYTE        cylinder_high;
    BYTE        dev_head;
    BYTE        dev_ctrl;
    BYTE        command;
} ata_regs;

//すべてのドライブを初期化
static void _sys_init_alldrivies();

//コマンドを送る
static int send_ide_cmd(ata_regs *regs, WORD prime_or_second);

static int send_non_data_cmd(ata_regs *regs, WORD prime_or_second);
static int send_data_out_cmd(ata_regs *regs, WORD prime_or_second, const void *buffer);
static int send_data_in_cmd(ata_regs *regs, WORD prime_or_second, int cnt, void *buffer);

static int _sys_ATA_select_device(BYTE dev_head_value, WORD prime_or_second);
static int _sys_IDE_idle(DRIVE *drv, BYTE stanby_time);
static int _sys_ATA_init_dev_param(DRIVE *drv);


static int read_sector_lba(int device_index, int lba, BYTE cnt, void *buffer);
static int write_sector_lba(int device_index,  int lba, BYTE cnt, const void *buffer);
static int _sys_ATA_readsector(int device_index, BYTE sector_num, BYTE head_num, WORD cylinder_num, BYTE cnt, void *buffer);
static int _sys_ATA_writesector(int device_index, BYTE sector_num, BYTE head_num, WORD cylinder_num, BYTE cnt, const void *buffer);


static DEVICE_TYPE _sys_ATA_getdevtype(BYTE cylinder_low, BYTE cylinder_high);
static int IDE_set_transfermode(DRIVE *drv);

static int wait_Status(WORD status_reg_port, BYTE mask, BYTE expected);
static int wait_BSYDRQclear(WORD status_reg_port);
static int check_DRQflag(WORD status_reg_port);
static int check_BSYflag(WORD status_reg_port);
static int wait_intrq(WORD prime_or_second);
static int _sys_wait_BSYclear(WORD status_reg_port);
static void read_data(DWORD read_len, WORD data_port, void *buffer);

static void _sys_ATA_softreset();
static void _sys_checkall_IDE_dev(DRIVE *drv);
static void _sys_subcheck_all_IDE_dev(DRIVE *drv);

static void analysis_identify_info(DRIVE *drv, WORD *data);

static void primary_irq_handler(void);
static void secondary_irq_handler(void);

/* operator */
static int seek_ata00(int pos, WORD flag);
static int read_ata00(int read_size, void *buffer);
static int write_ata00(int write_size, const void *buffer);
static int ioctl_ata00(int request, va_list arg);

static int seek_ata01(int pos, WORD flag);
static int read_ata01(int read_size, void *buffer);
static int write_ata01(int write_size, const void *buffer);

static int seek_ata10(int pos, WORD flag);
static int read_ata10(int read_size, void *buffer);
static int write_ata10(int write_size, const void *buffer);

static int seek_ata11(int pos, WORD flag);
static int read_ata11(int read_size, void *buffer);
static int write_ata11(int write_size, const void *buffer);


device_operator ide_operator = {
    -1, /* IRQは使用しない */
    "ATA/ATAPI device",
    0,
    &_sys_init_alldrivies,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


device_operator     ata00_operator = {
    14,
    "ata[0:0]",
    1,  /* ブロックデバイス */
    NULL,
    NULL,
    &ioctl_ata00,
    &seek_ata00,
    &read_ata00,
    &write_ata00
};

device_operator     ata01_operator = {
    14,
    "ata[0:1]",
    1,  /* ブロックデバイス */
    NULL,
    NULL,
    NULL,
    &seek_ata01,
    &read_ata01,
    &write_ata01
};


device_operator     ata10_operator = {
    15,
    "ata[0:0]",
    1,  /* ブロックデバイス */
    NULL,
    NULL,
    NULL,
    &seek_ata10,
    &read_ata10,
    &write_ata10
};


device_operator     ata11_operator = {
    15,
    "ata[0:1]",
    1,  /* ブロックデバイス */
    NULL,
    NULL,
    NULL,
    &seek_ata11,
    &read_ata11,
    &write_ata11
};


volatile int intr_flag[2] = {0, 0}; /* master and slave */

void     *handlers[] = {&primary_irq_handler, &secondary_irq_handler};

//デバイス管理用構造体
DRIVE drives[IDE_DEVICE_NUM] = {
    {0, 0, 0, 0, 0, 0, 0, 0, "", DEVICE_UNKNOW},        /* primary - master */
    {0, 1, 0, 0, 0, 0, 0, 0, "", DEVICE_UNKNOW},        /* primary - slave */
    {1, 0, 0, 0, 0, 0, 0, 0, "", DEVICE_UNKNOW},        /* secondary - master */
    {1, 1, 0, 0, 0, 0, 0, 0, "", DEVICE_UNKNOW}         /* secondary - slave */
};


//すべてのIDE接続デバイスをIDLE状態へ持ってゆく
static void _sys_init_alldrivies(){
    
    int             i, j;
    
    /* IRQハンドラに登録 */
    _sys_set_irq_handle(14, &primary_irq_handler);
    _sys_set_irq_handle(15, &secondary_irq_handler);
    
    /* エミュレーションシステム初期化 */
    init_ata_emulation();
    
    /* すべてのIDEデバイスをソフトウェアリセット */
    _sys_ATA_softreset();
    
    /* すべての機器の生死判定とデバイスタイプ判定を行う */
    _sys_checkall_IDE_dev(drives);
    
    /* IDENTIFY DEVICEを発行して */
    _sys_subcheck_all_IDE_dev(drives);
    
    for (i = 0 ; i < IDE_DEVICE_NUM ; i++) {
        
        _sys_printf(" IDE[%d:%d]...", drives[i].prime_or_second, drives[i].device_num);
        
        switch (drives[i].type) {
            case DEVICE_CANNOT_USE:
                _sys_printf("false\n");
                break;
            case DEVICE_UNKNOW:
                _sys_printf("unknow\n");
                break;
            case DEVICE_ATA:
                _sys_printf("ATA %s \n", drives[i].id);
                
                /* set tranfer mode */
                /* now PIO mode only!! */
                for (j = 0 ; j < RETRY_MAX ; j++) {
                    if (IDE_set_transfermode(&drives[i])) {
                        _sys_printf(" (PIO mode %d set)", drives[i].pio_mode_num);
                        break;
                    }
                }
                
                if (_sys_ATA_init_dev_param(&drives[i])) {
                    _sys_printk(" CHS OK ");
                }
                
                if (_sys_IDE_idle(&drives[i], 0x0)) {
                    _sys_printk(" IDLE");
                }
                
                _sys_putchar('\n');
                
                break;
                
            case DEVICE_ATAPI:
                _sys_printf("ATAPI %s\n", drives[i].id);
                /* set tranfer mode (only PIO) */
                for (j = 0 ; j < RETRY_MAX ; j++) {
                    if (IDE_set_transfermode(&drives[i])) {
                        _sys_printf(" (PIO mode %d set)", drives[i].pio_mode_num);
                        break;
                    }
                }
                
                /* 
                 * ATAPIデバイスはとりあえずIDLEモードには移行しない
                 * エラーが出る
                */
                if (_sys_IDE_idle(&drives[i], 0)) {
                    _sys_printf(" IDLE");
                }
                _sys_putchar('\n');
                break;
        }
    }
    return;
}


static int seek_ata00(int pos, WORD flag)
{    
    flag = 0x0;
    drives[0].seek_pos = pos;
    return 1;
}

static int read_ata00(int read_size, void *buffer)
{
    int cnt = read_size / 512;
    //_sys_printf("%d\n", drives[0].seek_pos);
    if(!(read_sector_lba(0, drives[0].seek_pos, cnt, buffer))){
        return 0;
    }
    return 1;
    
}

static int write_ata00(int write_size, const void *buffer)
{
    int cnt = write_size / 512;
    if (!(write_sector_lba(0, drives[0].seek_pos, 
                           cnt, buffer))) {
        return 0;
    }
    return 1;
}

static int ioctl_ata00(int request, va_list arg)
{
    DWORD   sector, track, head;
    switch (request) {
        case 0x03:
            break;
        case 0x04:
            track   = va_arg(arg, DWORD);
            head    = va_arg(arg, DWORD);
            sector  = va_arg(arg, DWORD);
            drives[0].seek_pos = (head + track * 4) * 17 + sector;
            break;
        default:
            return 0;
    }
    return 1;
}

static int seek_ata01(int pos, WORD flag)
{
    flag = 0x0;
    drives[1].seek_pos = pos;
    return 1;
}

static int read_ata01(int read_size, void *buffer)
{
    int cnt = read_size / drives[1].logical_sector_num;
    int i;

    for (i = 0 ; i < cnt ; i++) {
        if (!(read_sector_lba(1, drives[1].seek_pos+i, 
                              0, buffer))) {
            return 0;
        }
    }
    return 1;
}

static int write_ata01(int write_size, const void *buffer)
{
    int cnt = write_size / drives[1].logical_sector_num;
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if (!(write_sector_lba(1, drives[1].seek_pos+i, 
                               0, buffer))) {
            return 0;
        }
    }
    return 1;
}

static int seek_ata10(int pos, WORD flag)
{
    flag = 0x0;
    drives[2].seek_pos = pos;
    return 1;
}

static int read_ata10(int read_size, void *buffer)
{
    int cnt = read_size / drives[2].logical_sector_num;
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if(!(read_sector_lba(2, drives[2].seek_pos+i, 0, buffer))){
            return 0;
        }
    }
    
    return 1;
    
}

static int write_ata10(int write_size, const void *buffer)
{
    int cnt = write_size / drives[2].logical_sector_num;
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if(!(write_sector_lba(2, drives[2].seek_pos+i, 0, buffer))) {
            return 0;
        }
    }
    return 1;
}

static int seek_ata11(int pos, WORD flag)
{
    flag = 0x0;
    drives[3].seek_pos = pos;
    return 1;
    
}

static int read_ata11(int read_size, void *buffer)
{
    int cnt = read_size / drives[3].logical_sector_num;
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if (!(read_sector_lba(3, drives[3].seek_pos + i, 
                              0, buffer))) {
            return 0;
        }
    }
    
    return 1;
    
}

static int write_ata11(int write_size, const void *buffer)
{
    int cnt = write_size / drives[3].logical_sector_num;
    int i;
    
    for (i = 0 ; i < cnt ; i++) {
        if (!(write_sector_lba(3, drives[3].seek_pos + i,
                               0, buffer))) {
            return 0;
        }
    }
    return 1;
}

static void primary_irq_handler(void)
{
    intr_flag[0] = 1;
}

static void secondary_irq_handler(void)
{
    intr_flag[1] = 1;
}

static int send_ide_cmd(ata_regs *regs, WORD prime_or_second)
{
    /*
     * DEVICE HEADレジスタは
     * _sys_ATA_select_deviceにて設定してくれ
     */

    /* ここのleaveとかでエラーが起こっている */
    if (prime_or_second == 0) {
        /* gccのバグ？ */
        /* primary */
        outb(PRIM_FEATURES, regs->features);
        outb(PRIM_SECTOR_CNT, regs->sector_cnt);
        outb(PRIM_SECTOR_NUM, regs->sector_num);
        outb(PRIM_CYLIN_LOW, regs->cylinder_low);
        outb(PRIM_CYLIN_HIGH, regs->cylinder_high);
        outb(PRIM_DEVICE_CTRL, regs->dev_ctrl);
        outb(PRIM_COMMAND, regs->command);
        //__STOP();
        //_sys_printf("command: %x\n", regs->command);
        
    } else if (prime_or_second == 1) {
        /* secondary */
        outb(SECD_FEATURES, regs->features);
        outb(SECD_SECTOR_CNT, regs->sector_cnt);
        outb(SECD_SECTOR_NUM, regs->sector_num);
        outb(SECD_CYLIN_LOW, regs->cylinder_low);
        outb(SECD_CYLIN_HIGH, regs->cylinder_high);
        outb(SECD_DEVICE_CTRL, regs->dev_ctrl);
        //コマンド実行
        outb(SECD_COMMAND, regs->command);
        //outb(SECD_COMMAND, tmp_value);
    } else {
        return 0;
    }    
    return 1;
}

static int send_data_out_cmd(ata_regs *regs, WORD prime_or_second, 
                             const void *buffer)
{
    WORD status_reg_port, data_reg_port;
    WORD *ptr;
    ptr = (WORD*)buffer;
    switch (prime_or_second) {
        case 0:
            /* primary */
            status_reg_port = PRIM_STATUS;
            data_reg_port   = PRIM_DATA;
            break;
        case 1:
            /* secondary */
            status_reg_port = SECD_STATUS;
            data_reg_port   = SECD_DATA;
            break;
        default:
            return 0;
    }
    
    if (!_sys_ATA_select_device(regs->dev_head, 
                                prime_or_second)) {
        return 0;
    }
    
    if (!send_ide_cmd(regs, prime_or_second)) {
        return 0;
    }
    
    delay(T_400NS_WAIT);
    
    /* BSY=0 & DRQ=1 */
    wait_Status(status_reg_port, 0x88, 0x08);
    
    for (;;) {        
        outw(data_reg_port, *ptr);        
        wait_Status(status_reg_port, 0x88, 0x80);
        
        if (!check_DRQflag(status_reg_port)) {
            break;
        }
        wait_intrq(prime_or_second);
        ptr++;
    }
    
    wait_intrq(prime_or_second);
    inb(status_reg_port);
    return 1;
}

/*
static int send_data_in_cmd(ata_regs *regs, WORD prime_or_second, void *buffer){
    
    WORD    status_reg_port, data_reg_port, alt_status;
    WORD    *ptr;
    
    ptr = (WORD*)buffer;
    
    switch(prime_or_second){
        case 0:
            status_reg_port = PRIM_STATUS;
            data_reg_port   = PRIM_DATA;
            alt_status      = PRIM_ALT_STATUS;
            break;
        case 1:
            status_reg_port = SECD_STATUS;
            data_reg_port   = SECD_DATA;
            alt_status      = SECD_ALT_STATUS;
            break;
        default:
            return 0;
    }
    
    if(!_sys_ATA_select_device(regs->dev_head, prime_or_second)){
        return 0;
    }
    
    if(!send_ide_cmd(regs, prime_or_second)){
        return 0;
    }
    
    delay(T_400NS_WAIT);
    
    for(;;){
        
        if(!_sys_wait_BSYclear(status_reg_port)){
            return 0;
        }
        
        if(!wait_intrq(prime_or_second)){
            return 0;
        }
    
        if(!check_DRQflag(status_reg_port)){
            return 0;
        }
        
        inb(status_reg_port);
        
        do{
            
            *ptr = inw(data_reg_port);      //バースト読み込み
            ptr++;
            
        }while(check_DRQflag(status_reg_port));
        
        if(check_BSYflag(status_reg_port)==0){
            break;
        }
        
    }
    
    inb(alt_status);
    
    inb(status_reg_port);
    
    return 1;
    
}
*/

static int send_data_in_cmd(ata_regs *regs, WORD prime_or_second, 
                            int count, void *buffer)
{    
    WORD status_reg_port, data_reg_port, alt_status;
    int  i;
    BYTE c;
    
    switch (prime_or_second) {
        case 0:
            /* primary */
            status_reg_port = PRIM_STATUS;
            data_reg_port   = PRIM_DATA;
            alt_status      = PRIM_ALT_STATUS;
            break;
        case 1:
            /* secondary */
            status_reg_port = SECD_STATUS;
            data_reg_port   = SECD_DATA;
            alt_status      = SECD_ALT_STATUS;
            break;
        default:
            return 0;
    }
    
    if (!_sys_ATA_select_device(regs->dev_head, 
                                prime_or_second)) {
        return 0;
    }
    
    if (!send_ide_cmd(regs, prime_or_second)) {
        return 0;
    }
    
    delay(T_400NS_WAIT);
    
    for (i = 0 ; i < count ; i++) {
        
        if (!wait_Status(status_reg_port, 
                         0x88, 0x08)) {
            return 0;
        }
        
        if (!wait_intrq(prime_or_second)) {
            return 0;
        }
        
        inb(status_reg_port);
        
        read_data(256, data_reg_port, buffer);
        buffer += 512;
        
        if ((inb(status_reg_port) & 0x88) == 0x00) {
            break;
        }
        
    }
    
    /* lat status register 空読み */
    inb(alt_status);
    inb(status_reg_port);
    
    return 1;
    
}

static int send_non_data_cmd(ata_regs *regs, WORD prime_or_second)
{    
    WORD status_reg_port;
    
    switch (prime_or_second) {
        case 0:
            /* primary */
            status_reg_port = PRIM_STATUS;
            break;
        case 1:
            status_reg_port = SECD_STATUS;
            break;
        default:
            return 0;
    }
    
    if (!_sys_ATA_select_device(regs->dev_head, 
                                prime_or_second)) {
        return 0;
    }
    
    if (!send_ide_cmd(regs, prime_or_second)) {
        return 0;
    }
    
    //400ns以上待つ
    delay(T_400NS_WAIT);
    
    if (!_sys_wait_BSYclear(status_reg_port)) {
        return 0;
    }
    
    if (!wait_intrq(prime_or_second)) {
        return 0;
    }
    
    //ちょっとここ通りますよ
    inb(status_reg_port);
    
    return 1;
    
}


/*
    
デバイスを選択する

BSYクリアウェイト処理あり

dev_head_value:
    device/headレジスタの値
prime_or_second:
    primary is 0, secondary is 1
*/
static int _sys_ATA_select_device(BYTE dev_head_value, WORD prime_or_second)
{
    
    WORD            in_port, dev_port;
    int             i;
    
    switch(prime_or_second){
        case 0: //primary
            in_port = PRIM_ALT_STATUS; dev_port = PRIM_DEVICE_HEAD;
            break;
        case 1: //secondary
            in_port = SECD_ALT_STATUS; dev_port = SECD_DEVICE_HEAD;
            break;
        default:
            return 0;
    }
    
    for(i=0;i<RETRY_MAX;i++){
        
        /* BSYFlagおよびDRQFlagがクリアであればOK */
        if((inb(in_port) & 0x88)!=0){
            continue;
        }
        
        outb(dev_port, dev_head_value);
    
        //400ns以上待つ..てるか！？
        delay(T_400NS_WAIT);
        
        //BSYFlagおよびDRQFlagがクリアであればOK
        if(!(inb(in_port) & 0x88)){
            return 1;
        }
    
    }
    
    return 0;
        
}


/* 接続されているすべてのIDEデバイスの生死判定を行う */
static void _sys_checkall_IDE_dev(DRIVE *drv)
{    
    BYTE reg_value;
    BYTE cylh_value, cyll_value;
    WORD status_reg, err_reg, dh_reg, cylh_reg, cyll_reg;
    int  i, j;
    
    for (i = 0 ; i < IDE_DEVICE_NUM ; i++){
        if (drv[i].prime_or_second == 0){
            /* primary */
            status_reg = PRIM_STATUS; err_reg = PRIM_ERROR;
            dh_reg  = PRIM_DEVICE_HEAD; cylh_reg = PRIM_CYLIN_HIGH;
            cyll_reg = PRIM_CYLIN_LOW;
        } else if (drv[i].prime_or_second == 1){
            /* secondary */
            status_reg = SECD_STATUS; err_reg = SECD_ERROR;
            dh_reg  = SECD_DEVICE_HEAD; cylh_reg = SECD_CYLIN_HIGH;
            cyll_reg = SECD_CYLIN_LOW;
        } else {
            continue;
        }
    
        for (j = 0 ; j < RETRY_MAX ; j++) {
            
            outb(dh_reg, drv[i].device_num << 4);
            //400ns待つ
            delay(T_400NS_WAIT);
            reg_value = inb(status_reg);

            if (reg_value == 0xFF) {
                //こいつは使えん
                drv[i].type = DEVICE_CANNOT_USE;
                break;
            }
            
            //BSYフラグ立つかな？
            if ((_sys_wait_BSYclear(status_reg)) != 1) {
                //だめだなぁ
                drv[i].type = DEVICE_CANNOT_USE;
                break;
            }
            
            reg_value = inb(err_reg);
            
            if (drv[i].device_num == MASTER_DRV) {
                if ((reg_value & 0x7F) != 1) {
                    drv[i].type = DEVICE_CANNOT_USE;
                    //こいつは使えない
                    break;
                }
                
                if ((inb(dh_reg) & 0x10) == 0) {
                    //よし大丈夫だ
                    cyll_value = inb(cyll_reg);
                    cylh_value = inb(cylh_reg);
                    
                    //これを基にしてATAかATAPIかを見分ける
                    drv[i].type = _sys_ATA_getdevtype(cyll_value, cylh_value);
                }
                
                break;
                
            } else if (drv[i].device_num == SLAVE_DRV){
                
                if (reg_value != 1) {
                    drv[i].type = DEVICE_CANNOT_USE;
                    break;
                }
                
                if ((inb(dh_reg) & 0x10) != 0) {
                    //よし大丈夫だ
                    cyll_value = inb(cyll_reg);
                    cylh_value = inb(cylh_reg);
                    //これを基にしてATAかATAPIかを見分ける
                    drv[i].type = _sys_ATA_getdevtype(cyll_value, cylh_value);
                }
                break;
            } else {
                //なめてんのかね
                drv[i].type = DEVICE_CANNOT_USE;
            }
        }
    }
}


static void _sys_subcheck_all_IDE_dev(DRIVE *drv)
{    
    WORD     dh_reg, status_reg;
    char     buffer[512];
    int i, j;
    ata_regs reg;
    
    for (i = 0 ; i < IDE_DEVICE_NUM ; i++) {
        if (drv[i].prime_or_second == 0) {
            /* primary */
            dh_reg      = PRIM_DEVICE_HEAD;
            status_reg  = PRIM_STATUS;
        } else if(drv[i].prime_or_second == 1) {
            /* secondary */
            dh_reg      = SECD_DEVICE_HEAD;
            status_reg  = SECD_STATUS;
        } else {
            continue;
        }
        
        /* 何度か繰り返す */
        for (j = 0 ; j < 4 ; j++) {
        
            outb(dh_reg, (BYTE)(drv[i].device_num << 4));
        
            delay(T_400NS_WAIT);
            
            /*
            if(!_sys_wait_BSYclear(status_reg)){
                //だめだなぁ
                drv[i].type = DEVICE_CANNOT_USE;
                continue;
            }
            */
            if (!wait_Status(status_reg, 0x88, 0x00)) {
                drv[i].type = DEVICE_CANNOT_USE;
                continue;
            }
            
            if ((inb(dh_reg) & 0x10) != (drv[i].device_num << 4)) {
                drv[i].type = DEVICE_CANNOT_USE;
                continue;
            }
        
            reg.features      = 0x0;
            reg.sector_cnt    = 0x0;
            reg.sector_num    = 0x0;
            reg.cylinder_low  = 0x0;
            reg.cylinder_high = 0x0;
            reg.dev_ctrl      = 0x0;
        
            if (drv[i].type == DEVICE_ATA) {
                reg.command = 0xec;
            } else if (drv[i].type == DEVICE_ATAPI) {
                reg.command = 0xa1;
            } else if (drv[i].type == DEVICE_UNKNOW) {
                reg.command = 0xec;
            } else {
                continue;
            }
        
            if ((send_data_in_cmd(&reg, drv[i].prime_or_second, 
                                  1, (char*)buffer)) == 1) {
                analysis_identify_info(&drv[i], (WORD*)buffer);
            } else {
                _sys_printf("fuck off: %d\n", i);
                drv[i].type = DEVICE_CANNOT_USE;
            }
        }
    }
}


static void analysis_identify_info(DRIVE *drv, WORD *data)
{    
    BYTE *value;
    
    /* どうやら型番の番号だけビッグエンディアンのよう...意味わからん設計だ... */
    //get ID
    _sys_memcpy(drv->id, (data+27), 40);
    change_endian((WORD*)drv->id, 20);
    drv->id[40] = '\0';
    
    /* 53th bit enable */
    if (*(data+53) & 0x02) {
        //PIO mode enable
        value = (char*)(data+64);
        
        drv->pio_mode_num   = 0x2;      /* 8-bit PIO transfer mode */
        
        if (*value & 0x01) drv->pio_mode_num = 3;        /* PIO mode num 3 */
        if (*value & 0x02) drv->pio_mode_num = 4;        /* PIO mode num 4 */
        
        //_sys_printf("value : %x\n", *value);
        
    } else {
        /* when PIO mode 3 or 4 not supported that set PIO mode 2 */
        //drv->pio_mode_num     = 0x2;      /* PIO mode 2 only */
        drv->pio_mode_num   = 0x2;
    }
    
    //_sys_printf("value : %x\n", drv->pio_mode_num);
    
    /* setting device param */
    drv->logical_cylinder_num = *(data+1);
    drv->logical_head_num     = *(data+3);
    drv->logical_sector_num   = *(data+6);
}



/* 転送モード設定 */
static int IDE_set_transfermode(DRIVE *drv)
{    
    ata_regs regs;
    
    if (drv->type == DEVICE_UNKNOW || 
        drv->type == DEVICE_CANNOT_USE){
        return 0;
    }
    
    regs.features       = drv->pio_mode_num;
    regs.sector_cnt     = drv->subcmd_code;
    regs.sector_num     = 0x0;
    regs.cylinder_low   = 0x0;
    regs.cylinder_high  = 0x0;
    regs.dev_head       = (BYTE)(drv->device_num << 4);
    regs.dev_ctrl       = 0x0;
    
    regs.command        = 0xEF;     /* features cmd */
    
    if (!send_non_data_cmd(&regs, drv->prime_or_second)) {
        return 0;
    }
    
    return 1;
}



static int _sys_IDE_idle(DRIVE *drv, BYTE stanby_time)
{    
    ata_regs regs;
    
    if (drv->type == DEVICE_UNKNOW || 
        drv->type == DEVICE_CANNOT_USE) {
        return 0;
    }
    
    regs.features       = 0x0;  
    regs.sector_num     = 0x0;
    regs.cylinder_low   = 0x0;
    regs.cylinder_high  = 0x0;
    regs.dev_head       = (BYTE)(drv->device_num << 4);
    regs.dev_ctrl       = 0x0;
    
    if (drv->type == DEVICE_ATAPI) {
        regs.sector_cnt     = 0x0;
        regs.command        = 0xe1;
    } else if (drv->type == DEVICE_ATA) {        
        regs.sector_cnt     = stanby_time;
        regs.command        = 0xe3;        
    } else {
        return 0;
    }
    
    if (!send_non_data_cmd(&regs, drv->prime_or_second)) {
        return 0;
    }
    
    return 1;
}



static int _sys_ATA_init_dev_param(DRIVE *drv)
{    
    ata_regs regs;
    
    if (drv->type == DEVICE_UNKNOW || 
        drv->type == DEVICE_CANNOT_USE) {
        return 0;
    }
    
    regs.features       = 0x0;
    regs.sector_cnt     = (BYTE)drv->logical_sector_num;
    regs.sector_num     = 0x0;
    regs.cylinder_low   = 0x0;
    regs.cylinder_high  = 0x0;
    regs.dev_head       = (BYTE)(((drv->device_num << 4) & 0x10) | ((drv->logical_head_num - 1) & 0x0f));
    regs.dev_ctrl       = 0x0;
    
    /* ata command */
    regs.command        = 0x91;
    
    if (!send_non_data_cmd(&regs, drv->prime_or_second)) {
        return 0;
    }
    
    return 1;
    
}


static int _sys_ATA_readsector(int device_index, BYTE sector_num, BYTE head_num, 
                               WORD cylinder_num, BYTE cnt, void *buffer)
{
    
    ata_regs        regs;
    
    if(drives[device_index].type==DEVICE_UNKNOW || drives[device_index].type==DEVICE_CANNOT_USE){
        return 0;
    }
    
    regs.features       = 0x0;
    regs.sector_cnt     = cnt;
    regs.sector_num     = sector_num;
    regs.cylinder_low   = (BYTE)(cylinder_num & 0x00ff);
    regs.cylinder_high  = (BYTE)((cylinder_num >> 8) & 0x00ff);
    regs.dev_head       = (BYTE)((drives[device_index].device_num << 4) | (head_num & 0x0f));
    regs.dev_ctrl       = 0x0;
    
    regs.command        = 0x20;
    
    if (!send_data_in_cmd(&regs, drives[device_index].prime_or_second, 
                          cnt, buffer)) {
        return 0;
    }
    
    return 1;
    
}

static int _sys_ATA_writesector(int device_index, BYTE sector_num, BYTE head_num, 
                                WORD cylinder_num, BYTE cnt, const void *buffer)
{    
    ata_regs regs;
    
    if (drives[device_index].type==DEVICE_UNKNOW || drives[device_index].type==DEVICE_CANNOT_USE){
        return 0;
    }
    
    regs.features       = 0x0;
    regs.sector_cnt     = cnt;
    regs.sector_num     = sector_num;
    regs.cylinder_low   = (BYTE)(cylinder_num & 0x00ff);
    regs.cylinder_high  = (BYTE)((cylinder_num >> 8) & 0x00ff);
    regs.dev_head       = (BYTE)((drives[device_index].device_num << 4) | (head_num & 0x0f));
    regs.dev_ctrl       = 0x0;
    
    regs.command        = 0x30;
    
    if(!send_data_out_cmd(&regs, drives[device_index].prime_or_second, buffer)){
        return 0;
    }
    
    return 1;
    
}

static int read_sector_lba(int device_index, int lba, 
                           BYTE cnt, void *buffer) 
{    
    ata_regs regs;
    
    if (drives[device_index].type == DEVICE_UNKNOW || 
        drives[device_index].type == DEVICE_CANNOT_USE) {
        return 0;
    }
    
    regs.features       = 0x0;
    regs.sector_cnt     = cnt;
    regs.sector_num     = (BYTE)lba;
    regs.cylinder_low   = (BYTE)(lba >> 8);
    regs.cylinder_high  = (BYTE)(lba >> 16);
    regs.dev_head       = (BYTE)((drives[device_index].device_num << 4) | 0x40 | ((lba >> 24) & 0xf));
    regs.dev_ctrl       = 0x0;
    
    regs.command        = 0x20;
    
    if (!send_data_in_cmd(&regs, drives[device_index].prime_or_second, 
                          cnt, buffer)){
        return 0;
    }
    
    return 1;
}


static int write_sector_lba(int device_index, int lba, 
                            BYTE cnt, const void *buffer)
{    
    ata_regs regs;
    
    if (drives[device_index].type == DEVICE_UNKNOW || 
        drives[device_index].type == DEVICE_CANNOT_USE) {
        return 0;
    }
    
    regs.features       = 0x0;
    regs.sector_cnt     = cnt;
    regs.sector_num     = (BYTE)lba;
    regs.cylinder_low   = (BYTE)(lba >> 8);
    regs.cylinder_high  = (BYTE)(lba >> 16);
    regs.dev_head       = (BYTE)((drives[device_index].device_num << 4) 
                                 | 0x40 | ((lba >> 24) & 0xf));
    regs.dev_ctrl       = 0x0;
    regs.command        = 0x30;
    
    if (!send_data_out_cmd(&regs, drives[device_index].prime_or_second, 
                           buffer)) {
        return 0;
    }
    
    return 1;
}


static DEVICE_TYPE _sys_ATA_getdevtype(BYTE cylinder_low, BYTE cylinder_high)
{    
    if (cylinder_low == 0x14 && cylinder_high == 0xEB) {
        return DEVICE_ATAPI;
    } else if (cylinder_low == 0x0 && cylinder_high == 0x0) {
        return DEVICE_ATA;
    } else {
        return DEVICE_UNKNOW;
    }
}


/* すべてのATAデバイスをソフトウェアリセットする */
static void _sys_ATA_softreset(void)
{    
    //ソフトウェアリセット
    outb(PRIM_DEVICE_CTRL, DEVICE_ENABLE_RESET);
    outb(SECD_DEVICE_CTRL, DEVICE_ENABLE_RESET);
    
    //5ms待つ
    delay(T_5MS_WAIT);
    
    //リセット解除
    outb(PRIM_DEVICE_CTRL, DEVICE_DISABLE_RESET);
    outb(SECD_DEVICE_CTRL, DEVICE_DISABLE_RESET);
    
    //5ms待つ
    delay(T_5MS_WAIT);
}

static void read_data(DWORD read_len, WORD data_port, void *buffer)
{
    DWORD i;
    WORD *p = (WORD*)buffer;
    for (i = 0 ; i < read_len ; i++) {
        *p = inw(data_port);
        p++;
    }
}


//BSYフラグがクリアされるまで待つ
static int _sys_wait_BSYclear(WORD status_reg_port)
{    
    int i = 0;
    for ( ; i < MAX_WAIT_COUNT ; i++) {
        if (!(inb(status_reg_port) & 0x80)) {
            //フラグが立ってない
            return 1;
        }
        delay(1);
    }    
    return 0;
}

//BSYフラグとDRQフラグがたっていれば１そうでなければ0
static int wait_BSYDRQclear(WORD status_reg_port)
{    
    int i = 0;    

    for ( ; i < MAX_WAIT_COUNT ; i++) {
        if (!(inb(status_reg_port) & 0x88)) {
            //フラグが立ってない
            return 1;
        }
        delay(1);
    }
    
    return 0;
    
}

/* IRQ割り込みを待つ */
static int wait_intrq(WORD prime_or_second)
{    
    int i;
    
    for (i = 0 ; i < MAX_WAIT_COUNT ; i++){
        
        if (intr_flag[prime_or_second]) {
            intr_flag[prime_or_second] = 0;
            return 1;
        }
        
        //少し待つ
        delay(T_400NS_WAIT);
    }
    
    intr_flag[prime_or_second] = 0;
    
    return 0;
    
}

static int wait_Status(WORD status_reg_port, BYTE mask, BYTE expected)
{    
    BYTE status;
    int  timer = 0;
    
    do {
        if (timer >= MAX_WAIT_COUNT) {
            return 0;
        }
        status = inb(status_reg_port);
        timer++;
    } while ((status & mask) != expected);
    
    return 1;
}


/* DRQフラグがたっていれば１そうでなければ0 */
static int check_DRQflag(WORD status_reg_port)
{    
    if (inb(status_reg_port) & 0x08) {
        return 1;
    }
    
    return 0;
}


static int check_BSYflag(WORD status_reg_port)
{    
    if (inb(status_reg_port) & 0x80) {
        return 1;
    }
    return 0;
}

