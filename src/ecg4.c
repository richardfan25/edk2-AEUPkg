#include <stdlib.h>
#include <string.h>

#include "platform/adaptate.h"
#include "base/rdc4.h"
#include "base/isp.h"
#include "base/flash.h"
#include "base/util.h"

#include "ecg4.h"
/*==============================================================*/

const chip_t ecg4_chips[1] ={
	{
		RDC4_VID,						/* vid */
		RDC4_PID_EIO_201,				/* pid */
	},
};

const codebase_t ecg4_cb = {
	sizeof(ecg4_chips)/sizeof(chip_t),	/* chip_num */
	&ecg4_chips[0],						/* chip_list */
	ECG4_CB_ID,						/* cbid */
	{									/* func */
		&ecg4_ec_init,						/* init */
		&ecg4_ec_exit,						/* exit */
		&ecg4_get_embedic_info,				/* get_embedic_info */
		&ecg4_get_bin_info,					/* get_binary_info */
		&ecg4_ec_update,					/* update_firmware */
		&ecg4_ec_check_model_name,			/* check_model_name */
	},
};

static ecg4_info_t *info = NULL;
static ecg4_info_t *bininfo = NULL;
static isp_flag_t 	update_flag;
/*==============================================================*/
int ecg4_pmc_get_stat(uint8_t pmcch, uint8_t *stat)
{
	if(pmcch == 0){
		if(rdc4_pmc_write_cmd(0x00) != 0){
			return -1;
		}
		if(rdc4_pmc_read_data(stat) != 0){
			return -1;
		}
	}
	else{
		if(rdc4_pmc_write_cmd(0x01) != 0){
			return -1;
		}
		if(rdc4_pmc_read_data(stat) != 0){
			return -1;
		}
	}
	return 0;
}
/*==============================================================*/
int ecg4_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf)
{
	uint8_t idx;
	
	if(rdc4_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc4_pmc_write_data(ctrl) != 0){
		return -1;
	}
	if(rdc4_pmc_write_data(did) != 0){
		return -1;
	}
	if(rdc4_pmc_write_data(len) != 0){
		return -1;
	}
	
	if((cmd & 0x01) != 0){
		// read
		for(idx = 0; idx < len; idx++){
			if(rdc4_pmc_read_data(buf + idx) != 0){
				return -1;
			}
		}
	}
	else{
		// write
		for(idx = 0; idx < len; idx++){
			if(rdc4_pmc_write_data(*(buf + idx)) != 0){
				return -1;
			}
		}
	}
	return 0;
}

/*==============================================================*/
int ecg4_get_embedic_info(embeded_t *fw_info)
{
	uint32_t	doc_ver;	
	uint8_t		buf[16];

	// document version
	if(ecg4_pmc_protocl(0x03, 0x0F, 0x0, 3, buf) != 0){
		return -1;
	}
	
	// [31:24] = 0
	// [23:16] = major ver
	// [15:8]  = minor ver
	// [7:0]   = revision
	buf[3] = 0;
	doc_ver = *(uint32_t *)buf;
	
	// Project name
	if(ecg4_pmc_protocl(0x53, 0x04, 0x10, PJT_NAME_SIZE, (uint8_t *)fw_info->project_name) != 0){
		return -1;
	}
	fw_info->project_name[PJT_NAME_SIZE] = '\0';

	// Chip Code
	if(ecg4_pmc_protocl(0x53, 0x12, 0x0, 12, (uint8_t *)fw_info->chip) != 0){
		return -1;
	}

	// Chip Detect added after version 1.0.18
	if (doc_ver >= 0x010012)
	{
		// Chip Detect
		if(strcmp((char*)fw_info->chip, "EIO-IS200") != 0)	//if not EIO-IS200
		{
			// Chip Detect
			if(ecg4_pmc_protocl(0x53, 0x15, 0x0, 12, (uint8_t *)buf) != 0){
				return -1;
			}
		
			//compare chip code and chip detect
			if(strcmp((char*)fw_info->chip, (char*)buf) != 0)
			{
				return -1;
			}
		}
	}

	// Firmware Ver
	if(ecg4_pmc_protocl(0x53, 0x22, 0x0, 16, (uint8_t *)fw_info->ver) != 0){
		return -1;
	}
	
	return 0;
}
/*==============================================================*/
// Get Project Information Of Binary File
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
void ecg4_print_bin_info(ecg4_info_t *h)
{
	uint8_t	cnt;
	
	fprintf(stderr, "       Chip Name : %s\n", h->chip);
	fprintf(stderr, "    Manufacturer : %s\n", h->mfu);
	fprintf(stderr, "    Project Name : %s\n", h->board);
	fprintf(stderr, "   Serial Number : %s\n", h->sn);
	fprintf(stderr, "   Platform Type : %s\n", h->plt_type);
	fprintf(stderr, "      Build Date : %s\n", h->fw.rel_date);
	fprintf(stderr, "Firmware Version : %c%d.%d_%04X\n", h->fw.type, h->fw.maj_ver, h->fw.min_ver, h->fw.svn_ver);
	fprintf(stderr, "Information Size : %d\n", h->head.size);
	fprintf(stderr, "Applicatopn size : 0x%X\n", h->isp_info.app_size);
	fprintf(stderr, "Bootloader size  : 0x%X\n", h->isp_info.boot_size);
	fprintf(stderr, "  Reserved Block : %d\n", h->isp_info.rsvd_cnt);
	for(cnt = 0; cnt < h->isp_info.rsvd_cnt; cnt++){
		fprintf(stderr, "Block%d address : 0x%08X\n", cnt, h->isp_info.rsvd_blk[cnt].st_addr);
		fprintf(stderr, "Block%d size    : 0x%X\n", cnt, h->isp_info.rsvd_blk[cnt].size);
	}
}
/*==============================================================*/
// Check Binary CheckSum(sum zero)
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecg4_check_bin_sum(binary_t *bin)
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
    for (offset = 0; offset < RDC4_FLASH_SIZE; offset += FLASH_PAGE_SIZE)
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
int ecg4_check_bin_info(ecg4_info_t *binfo)
{
	int			rtn = 0;
	int			i;
	uint8_t		c;
	uint32_t	rdate;
	
	if( (binfo->head.used != ECG4_ROM_USED) ||
		(binfo->head.id != ECG4_ROM_ID_INFO) ||
		(binfo->isp_info.rsvd_cnt > 4)){
		fprintf(stderr, "ERROR: Invalid Binary File.\n");
		rtn = -1;
	}
	else{
		 if(binfo->head.size != (sizeof(ecg4_info_t) - sizeof(ecg4_isp_rsvd_t) * (4 - binfo->isp_info.rsvd_cnt) - (sizeof(binfo->rom_init_chip_id)))){
			fprintf(stderr, "ERROR: Invalid Binary File(blk len).\n");
			rtn = -1;
		 }
		 
		// release date
		rdate = 0;
		for (i=0; i<8; i++)
		{
			rdate *= 10;
			c =  binfo->fw.rel_date[i];
			if (c >= '0' && c <= '9')
			{
				rdate += (c & 0xF);
			}
		}

		// add rom_init_chip_id checking after 2019/05/15
		if (rdate > 20190515)
		{
		 switch(binfo->rom_init_chip_id & 0xFF)
		 {
			 case RDC4_PID_EIO_IS200:
			 {
				 if(strcmp((char*)binfo->chip, "EIO-IS200") != 0)
				 {
					 fprintf(stderr, "ERROR: Invalid Binary File(chip id).\n");
					 rtn = -1;
				 }
				 break;
			 }
			 case RDC4_PID_EIO_201:
			 {
				 if(strcmp((char*)binfo->chip, "EIO-201") != 0)
				 {
					 fprintf(stderr, "ERROR: Invalid Binary File(chip id).\n");
					 rtn = -1;
				 }
				 break;
			 }
			 case RDC4_PID_EIO_211:
			 {
				 if(strcmp((char*)binfo->chip, "EIO-211") != 0)
				 {
					 fprintf(stderr, "ERROR: Invalid Binary File(chip id).\n");
					 rtn = -1;
				 }
				 break;
			 }
			 default:
			 {
				 fprintf(stderr, "ERROR: Invalid Binary File(chip id).\n");
				 rtn = -1;
				 break;
			 }
		 }
		}
	}
	return rtn;
}

/*==============================================================*/
int ecg4_isp_entry(void)
{
	uint8_t	data;
	
	// Enter ISP mode
	data = 0;
	if(rdc4_pmc_write_cmd(0x41) != 0)
		return -1;
	if(rdc4_pmc_write_data(0x55) != 0)
		return -1;
	if(rdc4_pmc_write_data(0x55) != 0)
		return -1;
	// Check EC is in ISP mode or not
	if(rdc4_pmc_read_data(&data) != 0)
		return -1;
	if(data != 0xAA)
		return -1;
	// Check PMC direct IO can be access
	if(rdc4_ecio_inpd(0xFF00) == 0x30313639)
		return 0;
	return 0;
}
/*==============================================================*/
void ecg4_isp_exit(void)
{
	// start WDT
#if defined(_WINDOW_) || defined(_LINUX_)
	if(gResetTime)
	{
		rdc4_wdt_en(gResetTime * 1000, 0x10);	// timeout: gResetTime sec, event: Reset EC
	}
	else
	{
		rdc4_wdt_en(10 * 1000, 0x10);	// timeout: 10 sec, event: Reset EC
	}
#else
	rdc4_wdt_en(1000, 0x10);	// timeout: 1sec, event: Reset EC
#endif
}

/*==============================================================*/
// Update EC binary
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecg4_check_rsvd(ecg4_isp_t *isp, uint32_t addr)
{
	uint8_t 	i;
	uint32_t	taddr;
	
	//reserved valid range is 0 ~ ECG4_ROM_INFO_ADDR
	if(addr >= ECG4_ROM_INFO_ADDR)
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
int ecg4_update_area(FILE *bFile, uint32_t start, uint32_t elen, uint32_t wlen)
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
		if((!update_flag.bits.fullerase) && ecg4_check_rsvd(&bininfo->isp_info, addr) == 0)
			continue;
		rdc4_fla_erase_sector(addr);
	}
	fprintf(stderr,"  Erase...\t\t\tDone.                \n");
		
	// Update/Verify Area
	fseek(bFile, start, SEEK_SET);
	end = start + wlen;
	//if((end & 0xFF) != 0)
	//	end += FLASH_PAGE_SIZE;
	for(addr = start; addr < end; addr += FLASH_PAGE_SIZE){
		fprintf(stderr,"  Program & Verify...\t\t%.1f%%\r", (float)addr / end * 100);
		if((!update_flag.bits.fullerase) && ecg4_check_rsvd(&bininfo->isp_info, addr) == 0){
			fseek(bFile, FLASH_PAGE_SIZE, SEEK_CUR);
			continue;
		}
		if(fread(buf, 1, FLASH_PAGE_SIZE, bFile) != FLASH_PAGE_SIZE){
			goto eErrFread;
		}
		// write ec
		if(rdc4_fla_program_page(addr, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
			goto eErrPP;
		}
		// verify
		if(rdc4_fla_read(addr, rbuf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
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
int ecg4_get_bin_info(binary_t *bin)
{
    uint32_t 	readcount;
	int			rtn = -1;
	char		ver_str[16];
	int			i;
	
	if(bininfo == NULL){
		fprintf(stderr, "ERROR: The EC module must be initialized.\n");
		goto exit;
	}
	// Get Sign
	fseek(bin->ctrl, ECG4_ROM_INFO_ADDR, SEEK_SET);
	readcount = (uint32_t)fread((uint8_t *)bininfo, 1, sizeof(ecg4_info_t), bin->ctrl);

	if(readcount != sizeof(ecg4_info_t)){
		fprintf(stderr, "ERROR: Read binary file fail.\n");
		goto exit;
	}
	
	// Get ROM Chip ID
	fseek(bin->ctrl, (ECG4_ROM_INIT_ADDR + ECG4_ROM_INIT_CHIP_ADDR), SEEK_SET);
	readcount = (uint32_t)fread((uint8_t *)&bininfo->rom_init_chip_id, 1, sizeof(bininfo->rom_init_chip_id), bin->ctrl);

	if(readcount != sizeof(bininfo->rom_init_chip_id)){
		fprintf(stderr, "ERROR: Read binary file rom fail.\n");
		goto exit;
	}
	
	// Check Sign
	if(ecg4_check_bin_info(bininfo) != 0)
		goto exit;
	
	// Firmware Version
	sprintf(ver_str, "%c%02d%02d%04d", bininfo->fw.type, bininfo->fw.maj_ver, bininfo->fw.min_ver, bininfo->fw.svn_ver);
	sprintf(bin->finfo.project_name, "%s", bininfo->board);
	sprintf(bin->finfo.ver, "%s", ver_str);
	sprintf(bin->finfo.chip, "%s", bininfo->chip);

	for (i=0; i<16; i++)
		bin->finfo.mfu[i] = bininfo->mfu[i];
		
	bin->finfo.mfu[16] = 0;

	//ecg4_print_bin_info(bininfo);
	
    rtn = 0;
exit:
	return rtn;
}

/*==============================================================*/
// Check binary and board is the same chip
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecg4_check_chip(binary_t *bin)
{
	embeded_t fw_info;
	
	if(ecg4_get_embedic_info(&fw_info) != 0)
	{
		fprintf(stderr, "ERROR: Check binary and board is the same chip fail.\n");
		return -1;
	}
	
	if(strcmp((char*)fw_info.chip, (char*)bin->finfo.chip) != 0)
	{
		fprintf(stderr, "ERROR: Check binary and board is different chip.\n");
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
// Update EC binary
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecg4_ec_update(binary_t *bin, isp_flag_t upflag)
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
	if(ecg4_check_bin_info(bininfo) != 0)
		return rtn;
	if(info == NULL){
		fprintf(stderr, "ERROR: The EC module must be initialized.\n");
		return rtn;
	}
	if(ecg4_check_bin_sum(bin) != 0)
		return rtn;
	
	if(ecg4_check_chip(bin) != 0)
	{
		return rtn;
	}
	
	// Enter EC ISP mode
	if(ecg4_isp_entry() != 0){
		fprintf(stderr, "ERROR: Failed to enter ISP mode.\n");
		return rtn;
	}
	// Init SPI for Flash
	if(rdc4_spi_init() != 0)
		goto exit;

	// Flash RDID
	rdc4_flash_info();

	// Load ECG FW Information table
	if(rdc4_fla_read(ECG4_ROM_INFO_ADDR, (uint8_t *)info, sizeof(ecg4_info_t)) != sizeof(ecg4_info_t))
		goto eErrFread;
	
	// Update Boot
	fprintf(stderr, "\nBootloader:\n");
	if(ecg4_update_area(bin->ctrl, ECG4_ROM_BOOT_ADDR, ECG4_ROM_BOOT_SIZE, bininfo->isp_info.boot_size) != 0)
		goto exit;	
	
	// Update Firmware
	fprintf(stderr, "Application:\n");
	if(ecg4_update_area(bin->ctrl, ECG4_ROM_APP_ADDR, ECG4_ROM_APP_SIZE, bininfo->isp_info.app_size) != 0)
		goto exit;
	
	// Update Text (except ECG FW Information table)
	fprintf(stderr, "Factory Information:\n");
	if(ecg4_update_area(bin->ctrl, ECG4_ROM_TEXT_ADDR, ECG4_ROM_TEXT_SIZE, ECG4_ROM_TEXT_SIZE - 0x1000) != 0)
		goto exit;	
	
	// Update Init
	fseek(bin->ctrl, ECG4_ROM_INIT_ADDR, SEEK_SET);
	if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
		goto eErrFread;
	}
	if(rdc4_fla_program_page(ECG4_ROM_INIT_ADDR, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
		goto eErrUP;
	}
	if(rdc4_fla_read(ECG4_ROM_INIT_ADDR, rbuf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
		goto eErrRead;
	if(memcmp(buf, rbuf, FLASH_PAGE_SIZE) != 0){
		goto eErrCmp;
	}
	
	// Update ECG FW Information table
	memcpy(bininfo->sn, info->sn, 16);
	if(rdc4_fla_program_page(ECG4_ROM_INFO_ADDR, (uint8_t *)bininfo, sizeof(ecg4_info_t)) != sizeof(ecg4_info_t)){
		goto eErrUP;
	}
	if(rdc4_fla_read(ECG4_ROM_INFO_ADDR, rbuf, sizeof(ecg4_info_t)) != sizeof(ecg4_info_t))
		goto eErrRead;
	if(memcmp((uint8_t *)bininfo, rbuf, sizeof(ecg4_info_t)) != 0)
		goto eErrCmp;
	
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

	rdc4_spi_uninit();
#if defined(_WINDOW_) || defined(_LINUX_)
	ecg4_isp_exit();
	OSPowerOff();
#else
	//sys_reset_notify();
	ecg4_isp_exit();
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
int ecg4_ec_check_model_name(binary_t *bin, embeded_t *fw_info)
{
	if(memcmp((const char *)bin->finfo.project_name, (const char *)fw_info->project_name, PJT_NAME_SIZE) != 0){
		fprintf(stderr, "ERROR: The firmware image file is broken.\n");
		return -1;
	}
	return 0;
}
/*==============================================================*/
int ecg4_ec_init(void)
{
	if(rdc4_ec_init() != 0)
		return -1;
	
	info = (ecg4_info_t *)malloc(sizeof(ecg4_info_t));
	if(info == NULL)
		return -1;
	
	bininfo = (ecg4_info_t *)malloc(sizeof(ecg4_info_t));
	if(bininfo == NULL){
		free(info);
		return -1;
	}

	return 0;
}

/*==============================================================*/
void ecg4_ec_exit(void)
{
	if(info != NULL)
		free(info);
	
	if(bininfo != NULL)
		free(bininfo);
}

