#ifndef __ECGS_H__
#define __ECGS_H__

#include "base/isp.h"

#define ECGSW_CB_ID				0x00
#define RDC_VID					0x52
#define ECGS_ROM_SIGN			"$ADV"
#define ECGS_ROM_MAGIC         0x55AA5AA5
#define ECGS_ROM_SIGN_LEN		4
#define ECGS_HEADER_SIZE		0x100
#define ECGS_PMC_RETRY			10000
/*==============================================================*/
// Binary information
typedef struct _ecgs_rom_header_t {
        char signature[4];
        uint32_t length;                        // Base information length
        uint32_t version;                       // version of rom_header_t

        char platform[32];

        struct {
                uint16_t yyyy;                  // ex. 2015
                uint8_t mm;                     // ex. 7  (July)
                uint8_t dd;                     // ex. 17 (17th)
        } date;

        uint32_t boot_st_addr;         			// Bootloader start address in ROM
        uint32_t boot_max_size;           		// Bootloader Maximum Size
        uint32_t app_st_addr;           		// Firmware start address in ROM
        uint32_t app_max_size;                  // Firmware Maximum Size
        uint32_t option_st_addr;       			// Firmware start address in ROM
        uint32_t option_max_size;         		// Firmware Maximum Size
        uint32_t size;                          // Code Size of bootloader or application
        uint32_t revision;                      // SVN revision
        uint32_t magic_num;
} ecgs_rom_header_t;
/*==============================================================*/
int ecgs_pmc_iwrite(uint8_t cmd, uint8_t idx, uint8_t offset, uint8_t len, uint8_t *buf);
int ecgs_pmc_iread(uint8_t cmd, uint8_t idx, uint8_t offset, uint8_t len, uint8_t *buf);
int ecgs_pmc_write(uint8_t cmd, uint8_t offset, uint8_t len, uint8_t *buf);
int ecgs_pmc_read(uint8_t cmd, uint8_t offset, uint8_t len, uint8_t *buf);
int ecgs_get_embedic_info(embeded_t *fw_info);
int ecgs_get_bin_info(binary_t *bin);
int ecgs_ec_update(binary_t *bin, isp_flag_t upflag);
int ecgs_ec_check_model_name(binary_t *bin, embeded_t *fw_info);
int ecgs_ec_init(void);
void ecgs_ec_exit(void);

extern const codebase_t ecgs_cb;

#endif // __ECGF_H__


