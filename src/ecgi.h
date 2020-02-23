#ifndef __ECGI_H__
#define __ECGI_H__

#include "base/isp.h"
#include "base/flash.h"

#define ECGIC_CB_ID					0xFE

#define ITE_VID						0x49
#define ITE_PID_5121				0x21

#define dFILE_BUFF_SIZE         	0x8000
/*==============================================================*/
#define ECGI_PMC_CMD_PORT			0x29E
#define ECGI_PMC_DAT_PORT   		0x29F

// ECG EC Sign String
#define ECGI_SIGN_ADDR				0x80//0x40
#define ECGI_SIGN_STR				{0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xB5, 0x85, 0x12, 0x5A, 0x5A, 0xAA, 0xAA}
#define ECGI_SIGN_LEN				14
// ECG EC ROM Header
#define ECGI_ROM_HEADER_ADDR		0x15515//0x14E70//0x4000
#define ECGI_ROM_HEADER_LEN			17

/*==============================================================*/
// PMC
/*==============================================================*/
#define ECGI_MBOX_RETRY				10000

/*==============================================================*/
// ISP
/*==============================================================*/
#define ECGI_ISP_SPI_WRSR				0x01	// Write Status Register
#define ECGI_ISP_SPI_BYTE_PROGRAM		0x02	// To Program One Data Byte
#define ECGI_ISP_SPI_WRDI				0x04	// Write diaable
#define ECGI_ISP_SPI_READ_STATUS		0x05	// Read Status Register
#define ECGI_ISP_SPI_WREN				0x06	// Write Enable
#define ECGI_ISP_SPI_HIGH_SPEED_READ	0x0B	// High-Speed Read
#define ECGI_ISP_SPI_EWSR				0x50	// Enable Write Status Register
#define ECGI_ISP_SPI_RDID				0xAB	// Read ID
#define ECGI_ISP_SPI_DEVICE_ID			0x9F	// Manufacture ID command
#define ECGI_ISP_SPI_04KByteBE			0x20	// Erase  4 KByte block of memory array
#define ECGI_ISP_SPI_SectorErase1K		0xD7	// eFlash Sector Erase(1K bytes) Command

#define ECGI_ISP_GO_EC_MAIN				0xFC	// exit isp mode and goto ec_main
#define ECGI_ISP_WDT_RESET				0xFE	// enable wdt reset directly

#define ECGI_EmbedRomID		0xFF
#define ECGI_SSTID			0xBF
#define ECGI_BlockSize21_4K	0x1000

#define ECGI_ISP_STANDARD_FLASH	0x33
#define ECGI_ISP_SECURITY_FLASH	0x55

//security flash command
#define ECGI_ISP_SECURITY_INIT_MD2				0x01
#define ECGI_ISP_SECURITY_INPUT_DATA_FOR_MD2	0x02
#define ECGI_ISP_SECURITY_MD2_UPDATE			0x03
#define ECGI_ISP_SECURITY_MD2_FINISH			0x04
#define ECGI_ISP_SECURITY_INPUT_ENCRYPT_DATA	0x05
#define ECGI_ISP_SECURITY_DECRYPT_DATA			0x06
#define ECGI_ISP_SECURITY_CHECK_AND_START_FLASH	0xDC
#define ECGI_ISP_SECURITY_EXIT_WITHOUT_FLASH	0xFC

#define ECGI_ISP_SECURITY_SUCCESS_ACK 0xA0

#define ECGI_ISP_SECURITY_SIZE 0x1FC00

#define ECGI_ISP_SECURITY_MD2_BLOCK 16
/*==============================================================*/
// Mailbox
/*==============================================================*/
#define ECGI_MBOX_BUF_MAXLEN		45
// Mailbox commands list
#define ECGI_MBOX_CMD_F0h			0xF0				// F0h: Get EC firmware version and project name.
#define ECGI_MBOX_CMD_C0h			0xC0				// C0h: Clear 256 bytes buffer
//mailbox offset
#define MBX_OFFSET_CMD     			0x00
#define MBX_OFFSET_STA     			0x01
#define MBX_OFFSET_PARA    			0x02
#define MBX_OFFSET_DATA   			0x03
#define MBX_OFFSET_DATA_END  		0x2F
/*==============================================================*/
typedef struct _ecgi_pjt_t{
	uint8_t			kver[2];
	uint8_t			id;
	uint8_t			ver[4];
	uint8_t			name[PJT_NAME_SIZE + 1];
	uint8_t			chip[2];
} ecgi_pjt_t;


/*==============================================================*/
int ecgi_ec_update(binary_t *bin, isp_flag_t upflag);
int ecgi_get_embedic_info(embeded_t *fw_info);
int ecgi_get_bin_info(binary_t *bin);
int ecgi_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecgi_ec_init(void);
void ecgi_ec_exit(void);

extern const codebase_t ecgi_cb;
#endif //__ECGE_H__
