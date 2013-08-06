

#include "ATAemu.h"
#include "ATAdrv.h"

static void register_handler(device_node node[], int n);

/* primary IDE—p */
static emulation_handler primary_ide_emu[] = {
    {PRIM_DATA,         NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_ERROR,        NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_FEATURES,     NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_SECTOR_CNT,   NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_SECTOR_NUM,   NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_CYLIN_LOW,    NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_CYLIN_HIGH,   NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_DEVICE_HEAD,  NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_STATUS,       NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_COMMAND,      NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_DEVICE_CTRL,  NULL, NULL, NULL, NULL, NULL, NULL},
    {PRIM_ALT_STATUS,   NULL, NULL, NULL, NULL, NULL, NULL}
};

static device_node emu_node1[] = {
    {&primary_ide_emu[0], NULL, NULL},
    {&primary_ide_emu[1], NULL, NULL},
    {&primary_ide_emu[2], NULL, NULL},
    {&primary_ide_emu[3], NULL, NULL},
    {&primary_ide_emu[4], NULL, NULL},
    {&primary_ide_emu[5], NULL, NULL},
    {&primary_ide_emu[6], NULL, NULL},
    {&primary_ide_emu[7], NULL, NULL},
    {&primary_ide_emu[8], NULL, NULL},
    {&primary_ide_emu[9], NULL, NULL},
    {&primary_ide_emu[10], NULL, NULL},
    {&primary_ide_emu[11], NULL, NULL}
};

/* secondary IDE—p */
static emulation_handler secondary_ide_emu[] = {
    {SECD_DATA,         NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_ERROR,        NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_FEATURES,     NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_SECTOR_CNT,   NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_SECTOR_NUM,   NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_CYLIN_LOW,    NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_CYLIN_HIGH,   NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_DEVICE_HEAD,  NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_STATUS,       NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_COMMAND,      NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_DEVICE_CTRL,  NULL, NULL, NULL, NULL, NULL, NULL},
    {SECD_ALT_STATUS,   NULL, NULL, NULL, NULL, NULL, NULL}
};

static device_node emu_node2[] = {
    {&secondary_ide_emu[0], NULL, NULL},
    {&secondary_ide_emu[1], NULL, NULL},
    {&secondary_ide_emu[2], NULL, NULL},
    {&secondary_ide_emu[3], NULL, NULL},
    {&secondary_ide_emu[4], NULL, NULL},
    {&secondary_ide_emu[5], NULL, NULL},
    {&secondary_ide_emu[6], NULL, NULL},
    {&secondary_ide_emu[7], NULL, NULL},
    {&secondary_ide_emu[8], NULL, NULL},
    {&secondary_ide_emu[9], NULL, NULL},
    {&secondary_ide_emu[10], NULL, NULL},
    {&secondary_ide_emu[11], NULL, NULL}
};

void init_ata_emulation(void)
{    
    /* primary “o˜^ */
    //register_handler(emu_node1, sizeof(emu_node1) / sizeof(device_node));
    
    /* secondary “o˜^ */
    //register_handler(emu_node2, sizeof(emu_node2) / sizeof(device_node));
    
}

static void register_handler(device_node node[], int n)
{   
    int i;  
    for (i = 0 ; i < n ; i++) {
        insert_resource(&node[i]);
    }
}
