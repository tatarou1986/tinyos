
#ifndef __resource_H__
#define __resource_H__

#include	"kernel.h"
#include	"system.h"

typedef struct _device_operator {
	int				irq_num;
	const char		*device_name;
	int				device_type;
	
	/* デバイス操作用各種ハンドラ */
	void			(*start)();
	void			(*end)();
	int				(*ioctl)(int, va_list);
	int				(*seek)(int, WORD);
	int				(*read)(int, void*);
	int				(*write)(int, const void*);
} device_operator;


typedef struct _emulation_handler {
	WORD	io_address;
	
	/* エミュレーション用ハンドラ */
	void	(*io_write_byte)(BYTE);
	BYTE	(*io_read_byte)();
	void	(*io_write_word)(WORD);
	WORD	(*io_read_word)();
	void	(*io_write_dword)(DWORD);
	DWORD	(*io_read_dword)();
} emulation_handler;


typedef struct _device_node {
	emulation_handler		*handler;
	struct _device_node		*left;
	struct _device_node		*right;
} device_node;

#endif
