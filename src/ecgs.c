#include <stdlib.h>
#include <string.h>

#include "platform/adaptate.h"
#include "base/rdc.h"
#include "base/isp.h"
#include "base/flash.h"
#include "base/util.h"
#include "ecgs.h"
/*==============================================================*/
const chip_t ecgs_chips ={
	RDC_VID,							/* vid */
	RDC_PID_EIO_IS200,					/* pid */
};

const codebase_t ecgs_cb = {
	sizeof(ecgs_chips)/sizeof(chip_t),	/* chip_num */
	&ecgs_chips,						/* chip_list */
	ECGSW_CB_ID,						/* cbid */
	{									/* func */
		&ecgs_ec_init,						/* init */
		&ecgs_ec_exit,						/* exit */
		&ecgs_get_embedic_info,				/* get_embedic_info */
		&ecgs_get_bin_info,					/* get_binary_info */
		&ecgs_ec_update,					/* update_firmware */
		&ecgs_ec_check_model_name,			/* check_model_name */
	},
};

/*==============================================================*/
int ecgs_pmc_iwrite(uint8_t cmd, uint8_t idx, uint8_t offset, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(idx) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(offset) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(len) != 0){
		return -1;
	}
	
	for(i = 0; i < len; i++){
		if(rdc_pmc_write_data(*(buf + i)) != 0){
			return -1;
		}
	}

	return 0;
}

/*==============================================================*/
int ecgs_pmc_iread(uint8_t cmd, uint8_t idx, uint8_t offset, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(idx) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(offset) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(len) != 0){
		return -1;
	}
	
	for(i = 0; i < len; i++){
		if(rdc_pmc_read_data(buf + i) != 0){
			return -1;
		}
	}

	return 0;
}

/*==============================================================*/
int ecgs_pmc_write(uint8_t cmd, uint8_t offset, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(offset) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(len) != 0){
		return -1;
	}
	
	for(i = 0; i < len; i++){
		if(rdc_pmc_write_data(*(buf + i)) != 0){
			return -1;
		}
	}

	return 0;
}

/*==============================================================*/
int ecgs_pmc_read(uint8_t cmd, uint8_t offset, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(offset) != 0){
		return -1;
	}
	if(rdc_pmc_write_data(len) != 0){
		return -1;
	}
	
	for(i = 0; i < len; i++){
		if(rdc_pmc_read_data(buf + i) != 0){
			return -1;
		}
	}

	return 0;
}
/*==============================================================*/
int ecgs_get_embedic_info(embeded_t *fw_info)
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
// Check Binary CheckSum(sum zero)
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgs_check_bin_sum(binary_t *bin)
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
// Check Input header is valid or not
// Input:   bHeader: rdc rom header point
//
// Output:  0 : header is valid
//			others: header is invalid
/*==============================================================*/
int ecgs_check_header(ecgs_rom_header_t *bHeader)
{
	if((memcmp(bHeader->signature, ECGS_ROM_SIGN, 4) != 0) ||
		(bHeader->length != sizeof(ecgs_rom_header_t)) ||
		(bHeader->magic_num != ECGS_ROM_MAGIC)){
		return -1;
	}
	return 0;
}
/*==============================================================*/
// Print Header infor
// Input:   bHeader: rdc rom header point
//
// Output:  None
/*==============================================================*/
void ecgs_print_header(ecgs_rom_header_t *bHeader)
{
	fprintf(stderr, "Sign: %c%c%c%c\n", bHeader->signature[0], bHeader->signature[1], bHeader->signature[2], bHeader->signature[3]);
	fprintf(stderr, "Header Length: %d\n", bHeader->length);
	fprintf(stderr, "Header Version: 0x%X\n", bHeader->version);
	fprintf(stderr, "Project: %s\n", bHeader->platform);
	fprintf(stderr, "Date: %d.%d.%d\n", bHeader->date.yyyy, bHeader->date.mm, bHeader->date.dd);
	fprintf(stderr, "boot st addr: 0x%X\n", bHeader->boot_st_addr);
	fprintf(stderr, "boot max addr: 0x%X\n", bHeader->boot_max_size);
	fprintf(stderr, "app st addr: 0x%X\n", bHeader->app_st_addr);
	fprintf(stderr, "app max addr: 0x%X\n", bHeader->app_max_size);
	fprintf(stderr, "option st addr: 0x%X\n", bHeader->option_st_addr);
	fprintf(stderr, "option max addr: 0x%X\n", bHeader->option_max_size);
	fprintf(stderr, "size: 0x%X\n", bHeader->size);
	fprintf(stderr, "revision: 0x%X\n", bHeader->revision);
	fprintf(stderr, "magic: 0x%X\n", bHeader->magic_num);
}
/*==============================================================*/
// Get Project Information Of Binary File
// Input:   bin: binary file control point
//
// Output:  Others: bin header point
//		    NULL: fail
// Note: must be free the header point if it is not needed
/*==============================================================*/
ecgs_rom_header_t *ecgs_get_header(binary_t *bin)
{
    uint32_t 	readcount;
	ecgs_rom_header_t *header = (ecgs_rom_header_t *)malloc(sizeof(ecgs_rom_header_t));

	if(header == NULL){
		fprintf(stderr, "ERROR: Failed to malloc.\n");
		return NULL;
	}
	// Get Sign
    fseek(bin->ctrl, 0, SEEK_SET);
	readcount = (uint32_t)fread((uint8_t *)header, 1, sizeof(ecgs_rom_header_t), bin->ctrl);
	
    if((readcount != sizeof(ecgs_rom_header_t)) ||
		(ecgs_check_header(header) != 0)){
        fprintf(stderr, "ERROR: Read binary file fail.\n");
		free(header);
        return NULL;
    }
	return header;
}

/*==============================================================*/
// Get Project Information Of Binary File
// Input:   bin: binary file control point
//
// Output:  Others: bin header point
//		    NULL: fail
// Note: must be free the header point if it is not needed
/*==============================================================*/
ecgs_rom_header_t *ecgs_get_bootheader(binary_t *bin)
{
    uint32_t 	readcount;
	ecgs_rom_header_t *header = ecgs_get_header(bin);

	if(header == NULL){
		return NULL;
	}
	// Get Sign
    fseek(bin->ctrl, header->boot_st_addr & (~0xFFF00000), SEEK_SET);
	readcount = (uint32_t)fread((uint8_t *)header, 1, sizeof(ecgs_rom_header_t), bin->ctrl);
	
    if((readcount != sizeof(ecgs_rom_header_t)) ||
		(ecgs_check_header(header) != 0)){
        fprintf(stderr, "ERROR: Read binary file fail.\n");
		free(header);
        return NULL;
    }
	
	return header;
}

/*==============================================================*/
// Get Project Information Of Binary File
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgs_get_bin_info(binary_t *bin)
{
	int		rtn = -1;
	uint8_t	ver_str[10];
	
    ecgs_rom_header_t *bHeader = ecgs_get_header(bin);
	
	if(ecgs_check_bin_sum(bin) != 0)
		goto exit;
	
	memset(ver_str, '?', sizeof(ver_str));
	dec_to_str((uint16_t)bHeader->revision, &ver_str[5], 4);	
	ver_str[9] = '\0';
	
	sprintf(bin->finfo.project_name, "%8s", bHeader->platform);
	sprintf(bin->finfo.ver, "%9s", ver_str);
	sprintf(bin->finfo.chip, "%9s", "EIO-IS200");
	sprintf(bin->finfo.mfu, "%9s", "Advantech");
	
    rtn = 0;
exit:
	if(bHeader != NULL){
		free(bHeader);
	}
		
	return rtn;
}

/*==============================================================*/
int ecgs_isp_entry(void)
{
	uint8_t	data;
	
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
void ecgs_isp_exit(void)
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
static int ecgs_isp_erase_option(binary_t *bin)
{
	uint32_t addr;
	uint32_t start;
	ecgs_rom_header_t *h = ecgs_get_header(bin);
	if(h == NULL)
		return -1;
	
	start = h->option_st_addr & (~0xFFF00000);
	for(addr = start; addr < (start + h->option_max_size); addr += FLASH_SECTOR_SIZE){
		rdc_fla_erase_sector(addr);
	}
	
	free(h);
	return 0;
}

/*==============================================================*/
static int ecgs_isp_update_boot(binary_t *bin)
{
	int 		rtn = -1;
	uint8_t 	*buf = NULL;
	uint8_t 	*vbuf = NULL;
	uint32_t 	offset;
	uint32_t 	start;
	ecgs_rom_header_t *h;
	uint32_t	max;

	h = ecgs_get_bootheader(bin);
	if(h == NULL)
		return -1;
	
	start = h->boot_st_addr & (~0xFFF00000);
	max = start + h->boot_max_size;

	printf("\nBootloader:\n");
	// Erase Boot
	for(offset = start; offset < max; offset += FLASH_SECTOR_SIZE){
		printf("  Erase ...\t\t%.1f%%\r", (float)offset / max * 100);
		rdc_fla_erase_sector(offset);
		fflush(stdout);
	}
	printf("  Erase ...\t\tDone.                \n");

	// Program Boot code
	if((buf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	fseek(bin->ctrl, start, SEEK_SET);
	max = h->size + FLASH_PAGE_SIZE;
	for(offset = 0; offset < max ; offset += FLASH_PAGE_SIZE){
		printf("  Programming...\t%.1f%%\r", (float)offset / max * 100);
		// read binary
		if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
			goto eErrFread;
		}

		// write boot
		if(rdc_fla_program_page(start + offset, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
			goto eErrPP;
		}
	}
	printf("  Programming...\tDone.                \n");
	
	// Program RDC init code
	fseek(bin->ctrl, RDC_INIT_SEC_OFFSET, SEEK_SET);
	if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
		goto eErrFread;
	}
	if(rdc_fla_program_page(RDC_INIT_SEC_OFFSET, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
		goto eErrPP;
	}
	
	// Verify Boot code
	if((vbuf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}

	fseek(bin->ctrl, start, SEEK_SET);
	max = h->size + FLASH_PAGE_SIZE;
	for(offset = 0; offset < max; offset += FLASH_PAGE_SIZE){
		printf("  Verify...\t\t%.1f%%\r", (float)offset / max * 100);
		// read ec
		if(rdc_fla_read(start + offset, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
			goto eErrFRD;
		// read bin
		if(fread(vbuf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE)
			goto eErrFread;
		// compare
		if(memcmp(buf, vbuf, FLASH_PAGE_SIZE) != 0)
		{
			goto eErrCmp;
		}
	}
	printf("  Verify...\t\tDone.                \n");
	// Verify RDC init code
	fseek(bin->ctrl, RDC_INIT_SEC_OFFSET, SEEK_SET);
	// read ec
	if(rdc_fla_read(RDC_INIT_SEC_OFFSET, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
		goto eErrFRD;
	// read bin
	if(fread(vbuf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE)
		goto eErrFread;
	// compare
	if(memcmp(buf, vbuf, FLASH_PAGE_SIZE) != 0)
	{
		goto eErrCmp;
	}
	
	rtn = 0;
exit:
	if(buf != NULL)
		free(buf);
	if(vbuf != NULL)
		free(vbuf);
	if(h != NULL)
		free(h);

	return rtn;
	
eErrAlloc:
	fprintf(stderr, "ERROR: Failed to allocate memory.\n");
	goto exit;
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
}

/*==============================================================*/
static int ecgs_isp_update_app(binary_t *bin)
{
	uint8_t *buf = NULL;
	uint8_t *vbuf = NULL;
	uint32_t offset;
	uint32_t start;
	uint32_t max;
	int rtn = -1;
	ecgs_rom_header_t *h = ecgs_get_header(bin);
	
	if(h == NULL)
		return -1;
	
	// Erase APP
	printf("Application:\n");
	start = h->app_st_addr & (~0xFFF00000);
	max = start + h->app_max_size;
	for(offset = start; offset < max; offset += FLASH_SECTOR_SIZE){
		fprintf(stderr,"  Erase...\t\t%.1f%%\r", (float)offset / max * 100);
		rdc_fla_erase_sector(offset);
	}
	fprintf(stderr,"  Erase...\t\tDone.                \n");
	// Program APP code
	if((buf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	fseek(bin->ctrl, start + ECGS_HEADER_SIZE, SEEK_SET);
	max = h->size + FLASH_PAGE_SIZE;
	for(offset = ECGS_HEADER_SIZE; offset < max ; offset += FLASH_PAGE_SIZE){
		fprintf(stderr,"  Programming...\t%.1f%%\r", (float)offset / max * 100);
		// read bin
		if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
			goto eErrFread;
		}
		// write ec
		if(rdc_fla_program_page(start + offset, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
			goto eErrPP;
		}
	}
	fprintf(stderr,"  Programming...\tDone.                \n");
	
	// Program Header
	fseek(bin->ctrl, start, SEEK_SET);
	// read bin
	if(fread(buf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE){
		goto eErrFread;
	}
	// write ec header
	if(rdc_fla_program_page(start, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
		goto eErrPP;
	}

	// Verify App & Header code
	if((vbuf = (uint8_t *)malloc(FLASH_PAGE_SIZE)) == NULL){
		goto eErrAlloc;
	}
	fseek(bin->ctrl, start, SEEK_SET);
	max = h->size + FLASH_PAGE_SIZE;
	for(offset = 0; offset < max; offset += FLASH_PAGE_SIZE){
		fprintf(stderr,"  Verify...\t\t%.1f%%\r", (float)offset / max * 100);
		// read ec
		if(rdc_fla_read(start + offset, buf, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
			goto eErrFRD;
		// read bin
		if(fread(vbuf, 1, FLASH_PAGE_SIZE, bin->ctrl) != FLASH_PAGE_SIZE)
			goto eErrFread;
		// compare
		if(memcmp(buf, vbuf, FLASH_PAGE_SIZE) != 0)
			goto eErrCmp;
	}
	fprintf(stderr,"  Verify...\t\tDone.                \n");
	
	rtn = 0;
exit:
	if(buf != NULL)
		free(buf);
	if(vbuf != NULL)
		free(vbuf);
	if(h != NULL)
		free(h);
	return rtn;
	
eErrAlloc:
	fprintf(stderr, "ERROR: Failed to allocate memory.\n");
	goto exit;
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
}

/*==============================================================*/
int ecgs_ec_update(binary_t *bin, isp_flag_t upflag)
{
	int rtn = -1;
	uint8_t		count = 50;
	
	if(ecgs_check_bin_sum(bin) != 0)
		return rtn;

	// Enter EC ISP mode
	if(ecgs_isp_entry() != 0){
		fprintf(stderr, "ERROR: Failed to enter ISP mode.\n");
		return rtn;
	}

	// Init SPI for Flash
	if(rdc_spi_init() != 0)
		goto exit;

	// Flash RDID
	rdc_flash_info();

	// erase option
	if(ecgs_isp_erase_option(bin) != 0){
		fprintf(stderr, "ERROR: Failed to erase option.\n");
		goto exit;
	}

	// update boot area
	if(ecgs_isp_update_boot(bin) != 0){
		fprintf(stderr, "ERROR: Failed to update boot.\n");
		goto exit;
	}
	
	// update app area
	if(ecgs_isp_update_app(bin) != 0){
		fprintf(stderr, "ERROR: Failed to update firmware.\n");
		goto exit;
	}

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
	
	rdc_spi_uninit();
#if defined(_WINDOW_) || defined(_LINUX_)
	ecgs_isp_exit();
	OSPowerOff();
#else
	//sys_reset_notify();
	ecgs_isp_exit();
	sys_reset();
#endif
	return rtn;
}
/*==============================================================*/
// Check Model Name
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgs_ec_check_model_name(binary_t *bin, embeded_t *fw_info)
{
	if(memcmp((const char *)bin->finfo.project_name, (const char *)fw_info->project_name, PJT_NAME_SIZE) != 0){
		fprintf(stderr, "ERROR: The firmware image file is broken.\n");
		return -1;
	}
	return 0;
}
/*==============================================================*/
int ecgs_ec_init(void)
{
	return rdc_ec_init();
}

/*==============================================================*/
void ecgs_ec_exit(void)
{
	;
}

