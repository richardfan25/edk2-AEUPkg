#ifndef __ECG4_H__
#define __ECG4_H__

#include "base/isp.h"

#define ECG4_CB_ID				0x84
#define RDC4_VID				0x52

// ECG4 ROM : EIO-xxx as super I/O
#define ECG4_ROM_USED			0x55
#define ECG4_ROM_APP_ADDR		0x00000
#define ECG4_ROM_APP_SIZE		0xE0000

#define ECG4_ROM_TEXT_ADDR		0xE0000
#define ECG4_ROM_TEXT_SIZE		0x10000

#define ECG4_ROM_INFO_SIZE		256
#define ECG4_ROM_INFO_REL_DATE	0x65		// 8 chars

#define ECG4_ROM_BOOT_ADDR		0xF0000
#define ECG4_ROM_BOOT_SIZE		0xFF00

#define ECG4_ROM_INIT_ADDR		0xFFF00
#define ECG4_ROM_INIT_SIZE		0x100
#define ECG4_ROM_INIT_CHIP_ADDR	0xC0

#define ECG4_ROM_ID_PSTG		10				// Private Storage
#define ECG4_ROM_ID_GSTG		11				// General Storage
#define ECG4_ROM_ID_CFIG		12				// Configuration
#define ECG4_ROM_ID_NVRM		13				// NVRAM
#define ECG4_ROM_ID_HISY		14				// History
#define ECG4_ROM_ID_INFO		15				// Information

#define SET4_ROM_ADDR(ROMID)	(ROMID * 0x1000 + ECG4_ROM_TEXT_ADDR)
#define ECG4_ROM_INFO_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_INFO )
#define ECG4_ROM_HISY_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_HISY )
#define ECG4_ROM_PSTG_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_PSTG )
#define ECG4_ROM_GSTG_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_GSTG )
#define ECG4_ROM_CFIG_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_CFIG )
#define ECG4_ROM_NVRM_ADDR		SET4_ROM_ADDR( ECG4_ROM_ID_NVRM )

// PMC
#define ECG4_PMC0_CMD_PORT		0x4C6
#define ECG4_PMC0_DAT_PORT		0x4C2

// ECGFW ROM Information Struct
typedef struct _ecg4_isp_rsvd_t{
	uint32_t	st_addr;
	uint32_t	size;
} ecg4_isp_rsvd_t;

typedef struct _ecg4_isp_t{
	uint32_t		app_size;
	uint32_t		boot_size;
	uint8_t			rsvd_cnt;
	ecg4_isp_rsvd_t rsvd_blk[4];
} ecg4_isp_t;


typedef	struct _ecg4_header_t{	// struct max leng is FL_HEAD_LEN
	uint8_t		used;				// 0x55 used, others unused
	uint8_t		id;					// Identity Number 
	uint16_t	size;				// data length of this flash_data struct, max is 256
	uint32_t	addr;				// current position of flash pointr, is writable(All of this area are 0xff)
	uint16_t 	crc;				// sum zero
} ecg4_header_t;

#pragma pack(1)
typedef struct	_ecg4_fw_ver_t{ // INFO_FW_LEN
	char			type;
	uint8_t			maj_ver;
	uint8_t			min_ver;
	uint16_t		svn_ver;
	uint8_t			rel_date[10];
} ecg4_fw_ver_t ;
#pragma pack()

typedef struct _ecg4_info_t{
	ecg4_header_t 	head;
	uint8_t			mfu[16];
	uint8_t			board[16];
	uint32_t		board_id;
	uint8_t			sn[16];
	uint8_t			plt_type[16];
	uint32_t		plt_ver;
	uint8_t			chip[12];
	ecg4_fw_ver_t	fw;
	uint32_t		eapi_ver;
	uint32_t		eapi_pnp_id;
	ecg4_isp_t		isp_info;
	uint32_t		rom_init_chip_id;
}ecg4_info_t;

// PMC commands list

int ecg4_pmc_get_stat(uint8_t pmcch, uint8_t *stat);
int ecg4_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf);
int ecg4_get_embedic_info(embeded_t *fw_info);
int ecg4_get_bin_info(binary_t *bin);
int ecg4_ec_update(binary_t *bin, isp_flag_t upflag);
int ecg4_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecg4_ec_init(void);
void ecg4_ec_exit(void);

extern const codebase_t ecg4_cb;
#endif // __ECG4_H__


