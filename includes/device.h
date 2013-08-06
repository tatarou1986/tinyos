
#ifndef deviceH
#define deviceH

#include		"kernel.h"
#include		"resource.h"

typedef int		FILE;
typedef int		size_t;

void _sys_init_all_device();

void insert_resource(device_node *ptr);
emulation_handler *search_resource(WORD io_port_addr);

/* デバイスのオペレータ関数 */
FILE open(const char *name);
void close(FILE fd);
int ioctl(FILE dd, int request, ...);
size_t write(FILE dd, int write_size, const void *buffer);
size_t read(FILE dd, int read_size, void *buffer);
size_t seek(FILE dd, int pos, WORD flag);

#endif

