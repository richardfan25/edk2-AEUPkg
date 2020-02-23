#ifndef __ECGE_H__
#define __ECGE_H__

#include "base/isp.h"
#include "base/flash.h"

#define ECGEC_CB_ID					0xFE

#define ITE_VID						0x49
#define ITE_PID_8528				0x28
#define ITE_PID_8525				0x25
#define ITE_PID_8518				0x18

#define dFILE_BUFF_SIZE         	0x8000
/*==============================================================*/
#define ECGE_PMC_CMD_PORT			0x29A
#define ECGE_PMC_DAT_PORT			0x299

// ECG EC Sign String
#define ECGE_SIGN_ADDR				0x40
#define ECGE_SIGN_STR				{0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xB5, 0x85, 0x12, 0x5A, 0x5A, 0xAA, 0xAA}
#define ECGE_SIGN_LEN				14
// ECG EC ROM Header
#define ECGE_ROM_HEADER_ADDR		0x4000
#define ECGE_ROM_HEADER_LEN			17
#define ECGE_ROM_TIMEOUT			20000

// ECG EC ACPI RAM OFFSET
#define ECGE_ACPI_ADDR_VERTBL		0xF7				// Version Table
#define ECGE_ACPI_ADDR_KVER1		0xF8				// kernel major version offset
#define ECGE_ACPI_ADDR_KVER2		0xF9				// kernel minor version offset
#define ECGE_ACPI_ADDR_CHIP1		0xFA				// Chip Vendor code
#define ECGE_ACPI_ADDR_CHIP2		0xFB				// Chip ID code
#define ECGE_ACPI_ADDR_PJTID		0xFC				// Project Name ID
#define ECGE_ACPI_ADDR_PJTTYPE		0xFD				// Project Type code
#define ECGE_ACPI_ADDR_VER1			0xFE				// Project major version
#define ECGE_ACPI_ADDR_VER2			0xFF				// Project minor version

#define ECGE_ROM_PAGECOUNT          (FLASH_BLOCK_SIZE / FLASH_PAGE_SIZE)
/*==============================================================*/
#define IO_PCICFG					0xCF8
#define IO_PCIDAT					0xCFC
#define PCI_BUS_MAX					255
#define PCI_DEV_MAX					31
#define PCI_FUNC_MAX				7
#define PCI_INTEL_VID				0x8086

// PCI Device: Intel LPC Interface Controller(ISA Bridge)
#define PCI_LPC_ISA_CLASS			0x0601				// The PCI Class Code of ISA Bridge.
#define PCI_LPC_BUS					0x00
#define PCI_LPC_DEV					0x1F
#define PCI_LPC_FUNC				0x00
#define PCI_LPC_CFGREG(reg)			(1 << 31 | PCI_LPC_BUS << 16 | PCI_LPC_DEV << 11 | PCI_LPC_FUNC << 8 | (reg & 0xFC))
/*==============================================================*/
// PMC
/*==============================================================*/
#define ECGE_MBOX_RETRY				10000
// PMC commands list
#define ECGE_PCMD_WR_MBOX			0x50				// 50h-7Fh: write mailbox 
#define ECGE_PCMD_RD_MBOX			0xA0				// A0h-CFh: read mailbox 
#define ECGE_PCMD_ISP				0xDC				// DCh:		Enter ISP MODE

/*==============================================================*/
// ISP
/*==============================================================*/
// ISP commands list
// - Before use ISP commands, must set ISP mode via PMC command(0xdc).
#define ECGE_ISP_FOLLOW_EN 		0x11                	// ITE enable follow mode
#define ECGE_ISP_WR_CMD    		0x12
#define ECGE_ISP_WR_DATA   		0x13
#define ECGE_ISP_RD_DATA   		0x14
#define ECGE_ISP_FOLLOW_DIS		0x15
#define ECGE_ISP_SET_ADDR0 		0x16
#define ECGE_ISP_SET_ADDR1 		0x17
#define ECGE_ISP_SET_ADDR2 		0x18
#define ECGE_ISP_SET_ADDR3 		0x19
#define ECGE_ISP_EXIT_FLASH		0xFC                	// exit isp mode
#define ECGE_ISP_GO_EC_MAIN		0xFD                	// exit isp mode and goto ec_main
#define ECGE_ISP_WDT_RESET 		0xFE                	// enable wdt reset directly

/*==============================================================*/
// Mailbox
/*==============================================================*/
#define ECGE_MBOX_BUF_MAXLEN		45
// Mailbox commands list
#define ECGE_MBOX_CMD_F0h			0xF0				// F0h: Get EC firmware version and project name.

/*==============================================================*/
typedef struct _ite_pjt_t{
	uint8_t			kver[2];
	uint8_t			id;
	uint8_t			ver[4];
	uint8_t			name[PJT_NAME_SIZE + 1];
	uint8_t			chip[2];
} ite_pjt_t;


/*==============================================================*/
int ecge_mbox_check_busy(void);
int ecge_mbox_write_cmd(uint8_t mb_cmd);
int ecge_mbox_write_data(uint8_t offset, uint8_t *buf, uint8_t len);
int ecge_mbox_write_para(uint8_t para);
int ecge_mbox_read_data(uint8_t offset, uint8_t *buf, uint8_t len);
int ecge_mbox_read_para(uint8_t *para);
int ecge_mbox_read_stat(uint8_t *stat);

int ecge_ec_update(binary_t *bin, isp_flag_t upflag);
int ecge_get_embedic_info(embeded_t *fw_info);
int ecge_get_bin_info(binary_t *bin);
int ecge_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecge_ec_init(void);
void ecge_ec_exit(void);

int ecge_disable_port6064(void);

extern const codebase_t ecge_cb;
#endif //__ECGE_H__
