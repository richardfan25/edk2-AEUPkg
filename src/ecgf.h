#ifndef __ECGF_H__
#define __ECGF_H__

#include "base/isp.h"

#define ECGFW_CB_ID				0x80
#define RDC_VID					0x52

// ECGFW ROM
#define ECGF_ROM_USED			0x55
#define ECGF_ROM_APP_ADDR		0x00000
#define ECGF_ROM_APP_SIZE		0xE0000

#define ECGF_ROM_TEXT_ADDR		0xE0000
#define ECGF_ROM_TEXT_SIZE		0x10000

#define ECGF_ROM_INFO_SIZE		256
#define ECGF_ROM_INFO_REL_DATE	0x65		// 8 chars

#define ECGF_ROM_BOOT_ADDR		0xF0000
#define ECGF_ROM_BOOT_SIZE		0xFF00

#define ECGF_ROM_MODEL			0xFFE00		// firmware compatible model name list
#define ECGF_ROM_MODEL_SIZE		(256)

#define ECGF_ROM_INIT_ADDR		0xFFF00
#define ECGF_ROM_INIT_SIZE		0x100
#define ECGF_ROM_INIT_CHIP_ADDR	0xC0

#define ECGF_ROM_ID_PSTG		10				// Private Storage
#define ECGF_ROM_ID_GSTG		11				// General Storage
#define ECGF_ROM_ID_CFIG		12				// Configuration
#define ECGF_ROM_ID_NVRM		13				// NVRAM
#define ECGF_ROM_ID_HISY		14				// History
#define ECGF_ROM_ID_INFO		15				// Information

#define SET_ROM_ADDR(ROMID)		(ROMID * 0x1000 + ECGF_ROM_TEXT_ADDR)
#define ECGF_ROM_INFO_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_INFO )
#define ECGF_ROM_HISY_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_HISY )
#define ECGF_ROM_PSTG_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_PSTG )
#define ECGF_ROM_GSTG_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_GSTG )
#define ECGF_ROM_CFIG_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_CFIG )
#define ECGF_ROM_NVRM_ADDR		SET_ROM_ADDR( ECGF_ROM_ID_NVRM )

// PMC
#define ECGF_PMC0_CMD_PORT		0x66
#define ECGF_PMC0_DAT_PORT		0x62

// ECGFW ROM Information Struct
typedef struct _ecgf_isp_rsvd_t{
	uint32_t	st_addr;
	uint32_t	size;
} ecgf_isp_rsvd_t;

typedef struct _ecgf_isp_t{
	uint32_t		app_size;
	uint32_t		boot_size;
	uint8_t			rsvd_cnt;
	ecgf_isp_rsvd_t rsvd_blk[4];
} ecgf_isp_t;


typedef	struct _ecgf_header_t{	// struct max leng is FL_HEAD_LEN
	uint8_t		used;				// 0x55 used, others unused
	uint8_t		id;					// Identity Number 
	uint16_t	size;				// data length of this flash_data struct, max is 256
	uint32_t	addr;				// current position of flash pointr, is writable(All of this area are 0xff)
	uint16_t 	crc;				// sum zero
} ecgf_header_t;

#pragma pack(1)
typedef struct	_ecgf_fw_ver_t{ // INFO_FW_LEN
	char			type;
	uint8_t			maj_ver;
	uint8_t			min_ver;
	uint16_t		svn_ver;
	uint8_t			rel_date[10];
} ecgf_fw_ver_t ;
#pragma pack()

typedef struct _ecgf_info_t{
	ecgf_header_t 	head;
	uint8_t			mfu[16];
	uint8_t			board[16];
	uint32_t		board_id;
	uint8_t			sn[16];
	uint8_t			plt_type[16];
	uint32_t		plt_ver;
	uint8_t			chip[12];
	ecgf_fw_ver_t	fw;
	uint32_t		eapi_ver;
	uint32_t		eapi_pnp_id;
	ecgf_isp_t		isp_info;
	uint32_t		rom_init_chip_id;
}ecgf_info_t;

// PMC commands list

int ecgf_pmc_get_stat(uint8_t pmcch, uint8_t *stat);
int ecgf_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf);
int ecgf_get_embedic_info(embeded_t *fw_info);
int ecgf_get_bin_info(binary_t *bin);
int ecgf_ec_update(binary_t *bin, isp_flag_t upflag);
int ecgf_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecgf_ec_init(void);
void ecgf_ec_exit(void);

extern const codebase_t ecgf_cb;
#endif // __ECGF_H__


