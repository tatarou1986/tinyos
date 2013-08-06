
#ifndef ATAdrvH
#define ATAdrvH


#include	"kernel.h"
#include	"resource.h"
#include	"device.h"

/* プライマリ */
#define		PRIM_DATA					0x01f0
#define		PRIM_ERROR					0x01f1
#define		PRIM_FEATURES				0x01f1
#define		PRIM_SECTOR_CNT				0x01f2
#define		PRIM_SECTOR_NUM				0x01f3
#define		PRIM_CYLIN_LOW				0x01f4
#define		PRIM_CYLIN_HIGH				0x01f5
#define		PRIM_DEVICE_HEAD			0x01f6
#define		PRIM_STATUS					0x01f7
#define		PRIM_COMMAND				0x01f7
#define		PRIM_DEVICE_CTRL			0x03f6
#define		PRIM_ALT_STATUS				0x03f6


/* セカンダリ */
#define		SECD_DATA					0x0170
#define		SECD_ERROR					0x0171
#define		SECD_FEATURES				0x0171
#define		SECD_SECTOR_CNT				0x0172
#define		SECD_SECTOR_NUM				0x0173
#define		SECD_CYLIN_LOW				0x0174
#define		SECD_CYLIN_HIGH				0x0175
#define		SECD_DEVICE_HEAD			0x0176
#define		SECD_STATUS					0x0177
#define		SECD_COMMAND				0x0177
#define		SECD_DEVICE_CTRL			0x0376
#define		SECD_ALT_STATUS				0x0376

/* 初期化時に使用する抽象オペレーター */
extern device_operator			ide_operator;

/* primary IDE用 */
extern device_operator			ata00_operator;
extern device_operator			ata01_operator;

/* secondary IDE用 */
extern device_operator			ata10_operator;
extern device_operator			ata11_operator;

#endif
