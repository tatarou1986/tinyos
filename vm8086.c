
#include		"vm8086.h"
#include		"device.h"
#include		"system.h"
#include		"gdebug.h"
#include		"vga.h"
#include		"i8259A.h"


static int _sys_read_boot_device();
void _sys_copy_user_code(WORD dest_index, void *dest, void *src, int n);
static void print_vm_bios_banner();
static void choice_boot_device();
static int check_magic_num(char *buffer);
static void main_loop();

void start_vm8086()
{
	
	char		buffer[512];
	FILE		fd;
	
	/* Ç≤à•éA */
	print_vm_bios_banner();
	
	/* VMÇÃãNìÆèÄîı */
	_sys_read_boot_device(buffer);
	
	_sys_copy_user_code(_USER_DS, (char*)START_OFFSET_ADDRESS, (char*)buffer, 512);
	
	asm volatile (
		"movl		%%cr4, %%eax		\n"
		"orl		$0x8, %%eax			\n"
		"movl		%%eax, %%cr4		\n"
	::);
	
	asm volatile (
		"pushl		$0x0	## push gs	\n"
		"pushl		$0x0	## push fs	\n"
		"pushl		$0x0	## push ds	\n"
		"pushl		$0x0	## push	es	\n"
		"pushl		$0x0	## push ss	\n"
		"pushl		$0x0	## push esp	\n"
		"## make EFLAGS					\n"
		"pushf							\n"
		"popl		%%eax				\n"
		"orl		$0x20000, %%eax		\n"
		"pushl		%%eax				\n"
		"pushl		$0x07C0	## push cs	\n"
		"pushl		$0x0	## push eip	\n"
		"xorl		%%eax, %%eax		\n"
		"xorl		%%ebp, %%ebp		\n"
		"xorl		%%edi, %%edi		\n"
		"xorl		%%esi, %%esi		\n"
		"xorl		%%edx, %%edx		\n"
		"xorl		%%ecx, %%ecx		\n"
		"xorl		%%ebx, %%ebx		\n"
		"iret		## go v8086 mode	\n"
	: /* no output */ : /* no input */ );
	
}


static int check_magic_num(char *buffer){
	
	WORD	*ptr = (WORD*)buffer;
	
	if(ptr[256-1]==0xAA55){
		return 1;
	}
	
	return -1;
	
}


static void main_loop(){
	
	FILE				fd;
	int					buf;
	
	if((fd=open("/dev/kbd0"))==-1){
		_sys_freeze("fatal error: Not find input device\n");
	}
	
	for(;;){
		
		buf = 0x0;
		read(fd, 1, &buf);
		
		if(buf!=0x0){
			_sys_printf(" %x ", buf);
		}
		
	}
	
}


static int _sys_read_boot_device(void *buffer){
	
	FILE		fd;
	
	_sys_printf("Booting from Hard Disk 0...");
	
	if((fd=open("/dev/ata00"))==-1){
		_sys_printf("false\n");
		_sys_freeze("fatal error: I/O open error\n");
	}
	
	seek(fd, 0, 0);
	
	if((read(fd, 512, (char*)buffer))==-1){
		_sys_printf("false\n");
		_sys_freeze("fatal error: I/O read error\n");
	}
	
	_sys_printf("ok\n");
	
	close(fd);
	
	return 1;
}


void _sys_copy_user_code(WORD dest_index, void *dest, void *src, int n){
	
	asm volatile (
		"pushf						\n"
		"pushw		%%es			\n"
		"movw		%%ax, %%es		\n"
		"cld						\n"
		"rep						\n"
		"movsw						\n"
		"popw		%%es			\n"
		"popf						\n"
	: /* no output */ :
	"a" (dest_index),
	"c" ((unsigned int)(n / 2)),
	"D" (dest),
	"S" (src)
	);
	
}

static void print_vm_bios_banner(){
	
	/* VGAçƒèâä˙âª */
	_sys_cls();
	
	_sys_set_color(VGA_CL_GREEN);
	
	_sys_printf("gfw BIOS v0.0.1,\n");
	_sys_printf("Copyright (C) 2005-2006, ddk\n");
	_sys_printf("Virtual BIOS. $Date %s %s $\n", __DATE__, __TIME__);
	
	_sys_set_color(VGA_CL_WHITE);
	
	_sys_putchar('\n');
	
}


