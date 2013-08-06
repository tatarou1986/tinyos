
#include "page.h"
#include "system.h"
#include "gdebug.h"

extern void start_kernel(void);
extern void _sys_set_pg0_paging();
extern void _sys_set_pg1_paging();

void startup_32(void)
{
    _sys_printf(" hello world\n");
    
    /* VMのページング */
    _sys_set_pg0_paging();
    
    /* Guset空間のページング */
    _sys_set_pg1_paging();
    
    /* カーネル処理へ */
    start_kernel();    
}
