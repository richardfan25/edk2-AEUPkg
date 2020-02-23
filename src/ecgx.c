#include <stdlib.h>
#include <string.h>

#include "platform/adaptate.h"
#include "base/rdc.h"
#include "base/isp.h"
#include "base/flash.h"
#include "base/util.h"

// ECGX : (X means cross)  Board=ECGS f/w ==(AEU ECGF f/w image) =======> Board=ECGF f/w

// Key  : EC RAM [FCh] = 0x5F (need to write 0x5F @ EC_RAM[FCh] for using ecgx code base)

#include "ecgs.h"
#include "ecgx.h"
/*==============================================================*/

const chip_t ecgx_chips ={
	RDC_VID,							/* vid */
	RDC_PID_EIO_IS200,					/* pid */
};

const codebase_t ecgx_cb = {
	sizeof(ecgx_chips)/sizeof(chip_t),	/* chip_num */
	&ecgx_chips,						/* chip_list */
	ECGXW_CB_ID,						/* cbid */
	{									/* func */
		&ecgx_ec_init,						/* init */
		&ecgx_ec_exit,						/* exit */
		&ecgx_get_embedic_info,				/* get_embedic_info */
		&ecgx_get_bin_info,					/* get_binary_info */
		&ecgx_ec_update,					/* update_firmware */
		&ecgx_ec_check_model_name,			/* check_model_name */
	},
};

static ecgx_info_t *info = NULL;
static ecgx_info_t *bininfo = NULL;
static isp_flag_t 	update_flag;
/*==============================================================*/
int ecgx_pmc_get_stat(uint8_t pmcch, uint8_t *stat)
{
	if(pmcch == 0){
		if(rdc_pmc_write_cmd(0x00) != 0){
			return -1;
		}
		if(rdc_pmc_read_data(stat) != 0){
			return -1;
		}
	}
	else{
		if(rdc_pmc_write_cmd(0x01) != 0){
			return -1;
		}
		if(rdc_pmc_read_data(stat) != 0){
			return -1;
		}
	}
	return 0;
}
/*==============================================================*/
int ecgx_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf)
{
	uint8_t idx;
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(ctrl) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(did) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(len) != 0){
		return -1;
	}
	
	if((cmd & 0x01) != 0){
		// read
		for(idx = 0; idx < len; idx++){
			if(rdc_pmc_read_data(buf + idx) != 0){
				return -1;
			}
		}
	}
	else{
		// write
		for(idx = 0; idx < len; idx++){
			if(rdc_pmc_write_data(*(buf + idx)) != 0){
				return -1;
			}
		}
	}
	return 0;
}

/*==============================================================*/
int ecgx_get_embedic_info(embeded_t *fw_info)
{
	uint8_t len = 0;
	char	name[PJT_NAME_SIZE + 1];
	char	ver[10];
	char	chip_name[16];

	// Project name
	if(ecgs_pmc_read(0x41, 0x06, 1, &len) != 0){
		return -1;
	}
	if(len > PJT_NAME_SIZE)
		len = PJT_NAME_SIZE;
	if(ecgs_pmc_read(0x41, 0x07, len, (uint8_t *)name) != 0){
		return -1;
	}
	name[PJT_NAME_SIZE] = '\0';
	// Chip Code
	if(ecgs_pmc_read(0x4A, 0x00, 16, (uint8_t *)chip_name) != 0){
		return -1;
	}
	
	// Firmware Version
	if(ecgs_pmc_read(0x4B, 0x00, 9, (uint8_t *)ver) != 0){
		return -1;
	}
	ver[9] = '\0';
	
	strcpy(fw_info->project_name, 	name);
	strcpy(fw_info->chip, 			chip_name);
	strcpy(fw_info->ver, 			ver);
	
	return 0;
}
/*==============================================================*/
// Get Project Information Of Binary File
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
void ecgx_print_bin_info(ecgx_info_t *h)
{
	uint8_t	cnt;
	
	fprintf(stderr, "Chip Name\t:\t%s\n", h->chip);
	fprintf(stderr, "Manufacturer\t:\t%s\n", h->mfu);
	fprintf(stderr, "Project Name\t:\t%s\n", h->board);
	fprintf(stderr, "Serial Number\t:\t%s\n", h->sn);
	fprintf(stderr, "Platform Type\t:\t%s\n", h->plt_type);
	fprintf(stderr, "Build Date\t:\t%s\n", h->fw.rel_date);
	fprintf(stderr, "Firmware Version :\t%c%d.%d_%04X\n", h->fw.type, h->fw.maj_ver, h->fw.min_ver, h->fw.svn_ver);
	fprintf(stderr, "Information Size :\t%d\n", h->head.size);
	fprintf(stderr, "Applicatopn size :\t0x%X\n", h->isp_info.app_size);
	fprintf(stderr, "Bootloader size\t:\t0x%X\n", h->isp_info.boot_size);
	fprintf(stderr, "Reserved Block\t:\t%d\n", h->isp_info.rsvd_cnt);
	for(cnt = 0; cnt < h->isp_info.rsvd_cnt; cnt++){
		fprintf(stderr, "Block%d address\t:\t0x%08X\n", cnt, h->isp_info.rsvd_blk[cnt].st_addr);
		fprintf(stderr, "Block%d size\t:\t0x%X\n", cnt, h->isp_info.rsvd_blk[cnt].size);
	}
}
/*==============================================================*/
// Check Binary CheckSum(sum zero)
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_check_bin_sum(binary_t *bin)
{
    int ret = 0;
    uint8_t *buf = NULL;
    uint32_t offset;
    uint32_t sum = 0;
	uint32_t idx = 0;
    uint16_t checksum = 0;

    if ((buf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory.\n");
        ret = -1;
        goto exit;
    }

    fseek(bin->ctrl, 0, SEEK_SET);
    for (offset = 0; offset < RDC_FLASH_SIZE; offset += FLASH_PAGE_SIZE)
    {
        if (fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE)
        {
            fprintf(stderr, "ERROR: Failed to load binary.\n");
            ret = -2;
            goto exit;
        }
        else
        {
			while(idx < FLASH_PAGE_SIZE){
				sum += *((uint16_t *)(buf + idx));
				idx += 2;
			}
			idx = 0;
        }
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    checksum = (uint16_t)~sum;
    if (checksum != 0)
    {
		fprintf(stderr, "ERROR: Invalid input file.\n");
        ret = -3;
        goto exit;
    }

exit:
    if (buf != NULL)
        free(buf);
    return ret;
}
/*==============================================================*/
// Check Project Information Of Binary File
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_check_bin_info(ecgx_info_t *binfo)
{
	int rtn = 0;
	
	if( (binfo->head.used != ECGX_ROM_USED) ||
		(binfo->head.id != ECGX_ROM_ID_INFO) ||
		(binfo->isp_info.rsvd_cnt > 4)){
		fprintf(stderr, "ERROR: Invalid Binary File.\n");
		rtn = -1;
	}
	else{
		 if(binfo->head.size != (sizeof(ecgx_info_t) - sizeof(ecgx_isp_rsvd_t) * (4 - binfo->isp_info.rsvd_cnt))){
			fprintf(stderr, "ERROR: Invalid Binary File(blk len).\n");
			rtn = -1;
		 }
	}
	return rtn;
}

/*==============================================================*/
int ecgx_isp_entry(void)
{
	uint8_t	data;
	
	// ecgx_isp_entry = ecgs_isp_entry
	
	// Enter ISP mode
	data = 0;
	if(rdc_pmc_write_cmd(0x40) != 0)
		return -1;
	if(rdc_pmc_write_data(0x55) != 0)
		return -1;
	if(rdc_pmc_write_data(0x55) != 0)
		return -1;
	// Check EC is in ISP mode or not
	if(rdc_pmc_read_data(&data) != 0)
		return -1;
	if(data != 0xAA)
		return -1;
	// Check PMC direct IO can be access
	if(rdc_ecio_inpd(0xFF00) == 0x30313639)
		return 0;
	return 0;
}
/*==============================================================*/
void ecgx_isp_exit(void)
{
	// start WDT
#if defined(_WINDOW_) || defined(_LINUX_)
	if(gResetTime)
	{
		rdc_wdt_en(gResetTime * 1000, 0x10);	// timeout: gResetTime sec, event: Reset EC
	}
	else
	{
		rdc_wdt_en(10 * 1000, 0x10);	// timeout: 10 sec, event: Reset EC
	}
#else
	rdc_wdt_en(1000, 0x10);	// timeout: 1sec, event: Reset EC
#endif
}

/*==============================================================*/
// Update EC binary
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_check_rsvd(ecgx_isp_t *isp, uint32_t addr)
{
	uint8_t 	i;
	uint32_t	taddr;
	
	//reserved valid range is 0 ~ ecgx_ROM_INFO_ADDR
	if(addr >= ECGX_ROM_INFO_ADDR)
		return -1;
	
	for(i = 0; i < isp->rsvd_cnt; i++){
		if(isp->rsvd_blk[i].size == 0)
			continue;
		
		taddr = isp->rsvd_blk[i].st_addr & 0xFFFFF;
		if( (addr >= taddr) && (addr < taddr + isp->rsvd_blk[i].size) ){
			return 0;
		}
	}
	return -1;
	
}

/*==============================================================*/
// Update EC binary
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_update_area(FILE *bFile, uint32_t start, uint32_t elen, uint32_t wlen)
{
	int			rtn = -1;
	uint32_t	addr;
	uint32_t	end;
	uint8_t 	*buf = NULL;
	uint8_t 	*rbuf = NULL;

	if((buf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	if((rbuf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	
	// Erase Area
	end = start + elen;
	for(addr = start; addr < end; addr += FLASH_SECTOR_SIZE){
		fprintf(stderr,"  Erase...\t\t\t%.1f%%\r", (float)addr / end * 100);
		rdc_fla_erase_sector(addr);
	}
	fprintf(stderr,"  Erase...\t\t\tDone.                \n");
		
	// Update/Verify Area
	fseek(bFile, start, SEEK_SET);
	end = start + wlen;
	//if((end & 0xFF) != 0)
	//	end += FLASH_PAGE_SIZE;
	for(addr = start; addr < end; addr += FLASH_PAGE_SIZE){
		fprintf(stderr,"  Program & Verify...\t\t%.1f%%\r", (float)addr / end * 100);
		if(fread(buf, 1, FLASH_PAGE_SIZE, bFile) != FLASH_PAGE_SIZE){
			goto eErrFread;
		}
		// write ec
		if(rdc_fla_program_page(addr, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
			goto eErrPP;
		}
		// verify
		if(rdc_fla_read(addr, rbuf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
			goto eErrFRD;
		
		if(memcmp(buf, rbuf, FLASH_PAGE_SIZE) != 0)
			goto eErrCmp;
	}
	fprintf(stderr,"  Program & Verify...\t\tDone.                \n");
	rtn = 0;
	
exit:
	if(buf != NULL)
		free(buf);
	if(rbuf != NULL)
		free(rbuf);
	return rtn;

eErrFread:
	fprintf(stderr, "ERROR: Failed to load binary.\n");
	goto exit;
eErrPP:
	fprintf(stderr, "ERROR: Failed to download the binary to EC.\n");
	goto exit;
eErrFRD:
	fprintf(stderr, "ERROR: Failed to read from EC.\n");
	goto exit;
eErrCmp:
	fprintf(stderr, "ERROR: Verify failed.\n");
	goto exit;
eErrAlloc:
	fprintf(stderr, "ERROR: Failed to allocate memory.\n");
	goto exit;
}

/*==============================================================*/
// Get Project Information Of Binary File
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_get_bin_info(binary_t *bin)
{
    uint32_t 	readcount;
	int			rtn = -1;
	char		ver_str[16];
	
	if(bininfo == NULL){
		fprintf(stderr, "ERROR: The EC module must be initialized.\n");
		goto exit;
	}
	// Get Sign
	fseek(bin->ctrl, ECGX_ROM_INFO_ADDR, SEEK_SET);
	readcount = (uint32_t)fread((uint8_t *)bininfo, 1, sizeof(ecgx_info_t), bin->ctrl);

	if(readcount != sizeof(ecgx_info_t)){
		fprintf(stderr, "ERROR: Read binary file fail.\n");
		goto exit;
	}
	
	// Check Sign
	if(ecgx_check_bin_info(bininfo) != 0)
		goto exit;
	
	// Firmware Version
	sprintf(ver_str, "%c%02d%02d%04d", bininfo->fw.type, bininfo->fw.maj_ver, bininfo->fw.min_ver, bininfo->fw.svn_ver);
	sprintf(bin->finfo.project_name, "%8s", bininfo->board);
	sprintf(bin->finfo.ver, "%9s", ver_str);
	sprintf(bin->finfo.chip, "%9s", bininfo->chip);
	sprintf(bin->finfo.mfu, "%9s", "Advantech");
	
		
	//ecgx_print_bin_info(bininfo);
	
    rtn = 0;
exit:
	return rtn;
}
/*==============================================================*/
// Update EC binary
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_ec_update(binary_t *bin, isp_flag_t upflag)
{
	int			rtn = -1;
	uint8_t 	*buf = NULL;
	uint8_t 	*rbuf = NULL;
	uint8_t		count = 50;
	
	update_flag = upflag;
	if((buf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	if((rbuf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	if(bininfo == NULL){
		fprintf(stderr, "ERROR: The EC module must be initialized.\n");
		return rtn;
	}
	if(ecgx_check_bin_info(bininfo) != 0)
		return rtn;
	if(info == NULL){
		fprintf(stderr, "ERROR: The EC module must be initialized.\n");
		return rtn;
	}
	if(ecgx_check_bin_sum(bin) != 0)
		return rtn;
	// Enter EC ISP mode
	if(ecgx_isp_entry() != 0){
		fprintf(stderr, "ERROR: Failed to enter ISP mode.\n");
		return rtn;
	}
	// Init SPI for Flash
	if(rdc_spi_init() != 0)
		goto exit;

	// Flash RDID
	rdc_flash_info();

	// Load ECG FW Information table
	//if(rdc_fla_read(ECGX_ROM_INFO_ADDR, (uint8_t *)info, sizeof(ecgx_info_t)) != sizeof(ecgx_info_t))
	//	goto eErrFread;
	
	// Update Boot
	fprintf(stderr, "\nBootloader:\n");
	if(ecgx_update_area(bin->ctrl, ECGX_ROM_BOOT_ADDR, ECGX_ROM_BOOT_SIZE, bininfo->isp_info.boot_size) != 0)
		goto exit;	
	
	// Update Firmware
	fprintf(stderr, "Application:\n");
	if(ecgx_update_area(bin->ctrl, ECGX_ROM_APP_ADDR, ECGX_ROM_APP_SIZE, bininfo->isp_info.app_size) != 0)
		goto exit;
	
	// Update Text (except ECG FW Information table)
	fprintf(stderr, "Factory Information:\n");

	//if(ecgx_update_area(bin->ctrl, ECGX_ROM_TEXT_ADDR, ECGX_ROM_TEXT_SIZE, ECGX_ROM_TEXT_SIZE - 0x1000) != 0)
	// update info : no need to keep info in this code base
	if(ecgx_update_area(bin->ctrl, ECGX_ROM_TEXT_ADDR, ECGX_ROM_TEXT_SIZE, ECGX_ROM_TEXT_SIZE) != 0)
		goto exit;	
	
	// Update Init
	fseek(bin->ctrl, ECGX_ROM_INIT_ADDR, SEEK_SET);
	if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
		goto eErrFread;
	}
	if(rdc_fla_program_page(ECGX_ROM_INIT_ADDR, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
		goto eErrUP;
	}
	if(rdc_fla_read(ECGX_ROM_INIT_ADDR, rbuf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
		goto eErrRead;
	if(memcmp(buf, rbuf, FLASH_PAGE_SIZE) != 0){
		goto eErrCmp;
	}
	
	// Update ECG FW Information table
	// no need to keep sn because there is no sn in SW-codebase.
	//memcpy(bininfo->sn, info->sn, 16);
	if(rdc_fla_program_page(ECGX_ROM_INFO_ADDR, (uint8_t *)bininfo, sizeof(ecgx_info_t)) != sizeof(ecgx_info_t)){
		goto eErrUP;
	}

	// no need to compare sn for this code base
	/*
	if(rdc_fla_read(ECGX_ROM_INFO_ADDR, rbuf, sizeof(ecgx_info_t)) != sizeof(ecgx_info_t))
		goto eErrRead;
	if(memcmp((uint8_t *)bininfo, rbuf, sizeof(ecgx_info_t)) != 0)
		goto eErrCmp;
	*/
	
    rtn = 0;

	fprintf(stderr, "\n\nEC firmware update successful.");                                                                   // isp done
	fprintf(stderr, "\n***\tPlease Remove All Power Source,\t\t***");
	fprintf(stderr, "\n***\tTo Complete the Update Process.\t\t***\n\n");

exit:
	if(upflag.bits.no_ack == 0)
	{
		while(count > 1)
		{
			fprintf(stderr, "\r\t%d\t", count / 10);
			if(CheckKey())
				break;
			sys_delay(100);	// 100ms
			count--;
		}
	}
	
	if(rbuf != NULL)
		free(rbuf);
	if(buf != NULL)
		free(buf);

	rdc_spi_uninit();
#if defined(_WINDOW_) || defined(_LINUX_)
	ecgx_isp_exit();
	OSPowerOff();
#else
	//sys_reset_notify();
	ecgx_isp_exit();
	sys_reset();
#endif
	return rtn;

eErrUP:
	fprintf(stderr, "ERROR: Failed to Update firmware.\n");
	goto exit;
eErrAlloc:
	fprintf(stderr, "ERROR: Failed to allocate memory.\n");
	goto exit;
eErrFread:
	fprintf(stderr, "ERROR: Failed to load binary.\n");
	goto exit;
eErrRead:
	fprintf(stderr, "ERROR: Failed to load firmware.\n");
	goto exit;
eErrCmp:
	fprintf(stderr, "ERROR: Verify failed.\n");
	goto exit;
}
/*==============================================================*/
// Check Model Name
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgx_ec_check_model_name(binary_t *bin, embeded_t *fw_info)
{
	if(memcmp((const char *)bin->finfo.project_name, (const char *)fw_info->project_name, PJT_NAME_SIZE) != 0){
		fprintf(stderr, "ERROR: The firmware image file is broken.\n");
		return -1;
	}
	return 0;
}
/*==============================================================*/
int ecgx_ec_init(void)
{
	if(rdc_ec_init() != 0)
		return -1;
	
	info = (ecgx_info_t *)malloc(sizeof(ecgx_info_t));
	if(info == NULL)
		return -1;
	
	bininfo = (ecgx_info_t *)malloc(sizeof(ecgx_info_t));
	if(bininfo == NULL){
		free(info);
		return -1;
	}

	return 0;
}

/*==============================================================*/
void ecgx_ec_exit(void)
{
	if(info != NULL)
		free(info);
	
	if(bininfo != NULL)
		free(bininfo);
}
