#ifndef __ECGX_H__
#define __ECGX_H__

#include "base/isp.h"

#define ECGXW_CB_ID				0x5F
#define RDC_VID					0x52

// ECGX ROM
#define ECGX_ROM_USED			0x55
#define ECGX_ROM_APP_ADDR		0x00000
#define ECGX_ROM_APP_SIZE		0xE0000

#define ECGX_ROM_TEXT_ADDR		0xE0000
#define ECGX_ROM_TEXT_SIZE		0x10000

#define ECGX_ROM_BOOT_ADDR		0xF0000
#define ECGX_ROM_BOOT_SIZE		0xFF00

#define ECGX_ROM_INIT_ADDR		0xFFF00
#define ECGX_ROM_INIT_SIZE		0x100

#define ECGX_ROM_ID_PSTG		10				// Private Storage
#define ECGX_ROM_ID_GSTG		11				// General Storage
#define ECGX_ROM_ID_CFIG		12				// Configuration
#define ECGX_ROM_ID_NVRM		13				// NVRAM
#define ECGX_ROM_ID_HISY		14				// History
#define ECGX_ROM_ID_INFO		15				// Information

#define SET_ECGX_ROM_ADDR(ROMID)	(ROMID * 0x1000 + ECGX_ROM_TEXT_ADDR)
#define ECGX_ROM_INFO_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_INFO )
#define ECGX_ROM_HISY_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_HISY )
#define ECGX_ROM_PSTG_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_PSTG )
#define ECGX_ROM_GSTG_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_GSTG )
#define ECGX_ROM_CFIG_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_CFIG )
#define ECGX_ROM_NVRM_ADDR			SET_ECGX_ROM_ADDR( ECGX_ROM_ID_NVRM )

// PMC
#define ECGX_PMC0_CMD_PORT		0x66
#define ECGX_PMC0_DAT_PORT		0x62

// ECGFW ROM Information Struct
typedef struct _ecgx_isp_rsvd_t{
	uint32_t	st_addr;
	uint32_t	size;
} ecgx_isp_rsvd_t;

typedef struct _ecgx_isp_t{
	uint32_t		app_size;
	uint32_t		boot_size;
	uint8_t			rsvd_cnt;
	ecgx_isp_rsvd_t rsvd_blk[4];
} ecgx_isp_t;


typedef	struct _ecgx_header_t{	// struct max leng is FL_HEAD_LEN
	uint8_t		used;				// 0x55 used, others unused
	uint8_t		id;					// Identity Number 
	uint16_t	size;				// data length of this flash_data struct, max is 256
	uint32_t	addr;				// current position of flash pointr, is writable(All of this area are 0xff)
	uint16_t 	crc;				// sum zero
} ecgx_header_t;

#pragma pack(1)
typedef struct	_ecgx_fw_ver_t{ // INFO_FW_LEN
	char			type;
	uint8_t			maj_ver;
	uint8_t			min_ver;
	uint16_t		svn_ver;
	uint8_t			rel_date[10];
} ecgx_fw_ver_t ;
#pragma pack()

typedef struct _ecgx_info_t{
	ecgx_header_t 	head;
	uint8_t			mfu[16];
	uint8_t			board[16];
	uint32_t		board_id;
	uint8_t			sn[16];
	uint8_t			plt_type[16];
	uint32_t		plt_ver;
	uint8_t			chip[12];
	ecgx_fw_ver_t	fw;
	uint32_t		eapi_ver;
	uint32_t		eapi_pnp_id;
	ecgx_isp_t		isp_info;
}ecgx_info_t;

// PMC commands list

int ecgx_pmc_get_stat(uint8_t pmcch, uint8_t *stat);
int ecgx_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf);
int ecgx_get_embedic_info(embeded_t *fw_info);
int ecgx_get_bin_info(binary_t *bin);
int ecgx_ec_update(binary_t *bin, isp_flag_t upflag);
int ecgx_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecgx_ec_init(void);
void ecgx_ec_exit(void);

extern const codebase_t ecgx_cb;
#endif // __ECGX_H__


