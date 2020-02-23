#include <stdlib.h>
#include <string.h>

#include "platform/adaptate.h"
#include "base/pmc.h"
#include "base/flash.h"
#include "base/util.h"
#include "base/isp.h"
#include "base/acpi.h"
#include "base/ite.h"
#include "ecge.h"

/*==============================================================*/

const chip_t ecge_chips[3] ={
	{
		ITE_VID,						/* vid */
		ITE_PID_8528,					/* pid */
	},
	{
		ITE_VID,						/* vid */
		ITE_PID_8525,					/* pid */
	},
	{
		ITE_VID,						/* vid */
		ITE_PID_8518,					/* pid */
	},
};
 
const codebase_t ecge_cb = {
	sizeof(ecge_chips)/sizeof(chip_t),	/* chip_num */
	&ecge_chips[0],						/* chip_list */
	ECGEC_CB_ID,						/* cbid */
	{									/* func */
		&ecge_ec_init,						/* init */
		&ecge_ec_exit,						/* exit */
		&ecge_get_embedic_info,				/* get_target_info */
		&ecge_get_bin_info,					/* get_binary_info */
		&ecge_ec_update,					/* update_firmware */
		&ecge_ec_check_model_name,			/* check_model_name */
	},
};

static pmc_port_t 	ecge_pmc = {ECGE_PMC_CMD_PORT, ECGE_PMC_DAT_PORT};
static uint8_t 		rom_id[FLASH_ID_LEN];
static uint8_t		ecge_flag = 0;
#ifdef _DOS_
/*==============================================================*/
// Enable 60/64 port
//
// Input:       none
// Output:      none
/*==============================================================*/
int ecge_enable_port6064(void)
{
	uint32_t pci_u32 = 0;
	int 	ret;

	// Find PCI Device - Intel ISA Bridge
	outpd(0xcf8, PCI_LPC_CFGREG(0x00));
	pci_u32 = inpd(0xcfc);
	if((pci_u32 & 0x0000FFFF) == PCI_INTEL_VID)						// Check VID
	{
		outpd(0xcf8, PCI_LPC_CFGREG(0x08));
		pci_u32 = inpd(0xcfc);
		if(((pci_u32 & 0xFFFF0000) >> 16) != PCI_LPC_ISA_CLASS)				// PCI Class Code
			goto eErrEnd;
	}
	else
		goto eErrEnd;

	// check kbc
	outpd(0xcf8, PCI_LPC_CFGREG(0x82));
	pci_u32 = inpd(0xcfc);
	if(pci_u32 & 0x04000000){
		//fprintf(stderr, "KBC Decode Already Enable\n");
		ret = 0;
		goto eEnd;
	}
	
	// enable kbc
	outpd(0xcf8, PCI_LPC_CFGREG(0x82));
	outpd(0xcfc, pci_u32 | 0x04000000);
	ret = 1;
	goto eEnd;

eErrEnd:
	fprintf(stderr, "ERROR: The Intel ISA bridge is not found!\n");
	ret = -1;
eEnd:
	sys_delay(5);
	if (inp(0x64) != 0xFF)
	{
		fprintf(stderr, "Enable legacy KBC.\n");
		while ((inp(0x64) & 2));
		outp(0x64, 0xAE);
	}
	return ret;
}
/*==============================================================*/
// Diable 60/64 port
// 
// Input:       none
// Output:      none
/*==============================================================*/
int ecge_disable_port6064(void)
{
	uint32_t pci_u32 = 0;

	if (inp(0x64) != 0xFF)
	{
		fprintf(stderr, "Disable legacy KBC.\n");
		while ((inp(0x64) & 2));
		outp(0x64, 0xAD);
		sys_delay(5);
	}
	// Find PCI Device - Intel ISA Bridge
	outpd(0xcf8, PCI_LPC_CFGREG(0x00));
	pci_u32 = inpd(0xcfc);
	if((pci_u32 & 0x0000FFFF) == PCI_INTEL_VID)								// Check VID
	{
		outpd(0xcf8, PCI_LPC_CFGREG(0x08));
		pci_u32 = inpd(0xcfc);
		if(((pci_u32 & 0xFFFF0000) >> 16) != PCI_LPC_ISA_CLASS)				// PCI Class Code
			goto eEnd;
	}
	else
		goto eEnd;

	// check kbc
	outpd(0xcf8, PCI_LPC_CFGREG(0x82));
	pci_u32 = inpd(0xcfc);
	if(!(pci_u32 & 0x04000000)){
		//fprintf(stderr, "KBC Decode Already Disable\n");
		return 0;
	}
	
	// disable kbc
	outpd(0xcf8, PCI_LPC_CFGREG(0x82));
	outpd(0xcfc, pci_u32 & (~0x04000000));
	pci_u32 = inpd(0xcfc);
	
	return 1;
eEnd:
	fprintf(stderr, "ERROR: The Intel ISA bridge is not found!\n");
	return -1;
}
#endif // _DOS_
/*==============================================================*/
// ECG-EC Mailbox Function
/*==============================================================*/
int ecge_mbox_check_busy(void)
{
	uint8_t  dumy 	= 0xFF;
	uint16_t retry	= 0;
	
	if (ecge_flag)
	{
		fprintf(stderr, "ERROR: The mailbox is unsupported.\n");
		return -1;
	}
	while(retry < ECGE_MBOX_RETRY){
		if(ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_RD_MBOX) != 0)
			return -1;
		if(ite_pmc_read_data(&ecge_pmc, &dumy) != 0)
			return -1;
		if(dumy == 0)
			return 0;
		
		sys_delay(1);
		retry++;
	}
	fprintf(stderr, "ERROR: The mailbox is no response.\n");
	return -1;
}

/*==============================================================*/
int ecge_mbox_write_cmd(uint8_t mb_cmd)
{
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	if(ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_WR_MBOX) != 0)
		return -1;
	
	if(ite_pmc_write_data(&ecge_pmc, mb_cmd) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int ecge_mbox_read_data(uint8_t offset, uint8_t *buf, uint8_t len)
{
	uint8_t idx;
	
	if(offset >= ECGE_MBOX_BUF_MAXLEN){
		fprintf(stderr, "ERROR: The data length for reading the mailbox is invalid.\n");
		return -1;
	}
	
	if(len > (ECGE_MBOX_BUF_MAXLEN - offset)){
		fprintf(stderr, "ERROR: The data length for reading the mailbox is too long.\n");
		return -1;
	}
	
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	offset = ECGE_PCMD_RD_MBOX + 0x3 + offset;			// Read mbox + data byte offset + user read offset
	for(idx = 0; idx < len; idx++){
		if(ite_pmc_write_cmd(&ecge_pmc, offset + idx) != 0)
			return -1;
		if(ite_pmc_read_data(&ecge_pmc, buf + idx) != 0)
			return -1;
	}
	return 0;
	
}

/*==============================================================*/
int ecge_mbox_write_data(uint8_t offset, uint8_t *buf, uint8_t len)
{
	uint8_t idx;
	
	if(offset >= ECGE_MBOX_BUF_MAXLEN){
		fprintf(stderr, "ERROR: The data length for writing the mailbox is invalid.\n");
		return -1;
	}
	
	if(len > (ECGE_MBOX_BUF_MAXLEN - offset)){
		fprintf(stderr, "ERROR: The data length for writing the mailbox is too long.\n");
		return -1;
	}
	
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	offset = ECGE_PCMD_WR_MBOX + 0x3 + offset;			// Read mbox + data byte offset + user read offset
	for(idx = 0; idx < len; idx++){
		if(ite_pmc_write_cmd(&ecge_pmc, offset + idx) != 0)
			return -1;
		if(ite_pmc_write_data(&ecge_pmc, *(buf + idx)) != 0)
			return -1;
	}
	return 0;
}

/*==============================================================*/
int ecge_mbox_read_para(uint8_t *para)
{
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	if(ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_WR_MBOX + 0x02) != 0)
		return -1;
	
	if(ite_pmc_read_data(&ecge_pmc, para) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int ecge_mbox_write_para(uint8_t para)
{
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	if(ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_WR_MBOX + 0x02) != 0)
		return -1;
	
	if(ite_pmc_write_data(&ecge_pmc, para) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int ecge_mbox_read_stat(uint8_t *stat)
{
	if(ecge_mbox_check_busy() != 0)
		return -1;
	
	if(ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_WR_MBOX + 0x01) != 0)
		return -1;
	
	if(ite_pmc_read_data(&ecge_pmc, stat) != 0)
		return -1;
	
	return 0;
}
/*==============================================================*/
// ECG-EC ISP Function
/*==============================================================*/
/*==============================================================*/
int ecge_isp_entry(void)
{
	return ite_pmc_write_cmd(&ecge_pmc, ECGE_PCMD_ISP);
}
/*==============================================================*/
int ecge_isp_write_cmd(uint8_t cmd)
{
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_WR_CMD) != 0)                         // send cmd
        return -1;
    if(ite_pmc_write_cmd(&ecge_pmc, cmd) != 0)
        return -1;
    
    return 0;
}
/*==============================================================*/
int ecge_isp_write_data(uint8_t *data, uint8_t lens)
{
    uint8_t idx;
    
    if(data == NULL){                                                               // check data length
        fprintf(stderr, "ERROR: Input data is NULL\n");
        return -1;
    }

    for(idx = 0; idx < lens; idx++){                                // send data
        if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_WR_DATA) != 0)
            return -1;
        if(ite_pmc_write_cmd(&ecge_pmc, *(data + idx)) != 0)
            return -1;      
    }
    
    return 0;
}
/*==============================================================*/
int ecge_isp_read_data(uint8_t cmd, uint8_t *buf, uint8_t lens)
{
    uint8_t idx;
    
    if(buf == NULL){
        fprintf(stderr, "ERROR: CMD(0x%x) input buf is NULL\n",cmd);
        return -1;
    }
    if(ecge_isp_write_cmd(cmd) != 0)                                                // send cmd
        return -1;
            
    
    for(idx = 0; idx < lens; idx++){
        if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_RD_DATA) != 0)
            return -1;
        if(ite_pmc_read_data(&ecge_pmc, buf + idx) != 0)
            return -1;
    }
    return 0;
}
/*==============================================================*/
int ecge_isp_check_write_en(void)
{
    uint16_t        retry = 0;
    uint8_t         data = 0;

    while(1){
        if(ecge_isp_read_data(FLASH_CMD_RDSR, &data, 1) != 0){                // read rom status
            fprintf(stderr, "ERROR: Failed to access ROM\n");
            return -1;
        }
        if((data & FLASH_RDSR_BIT_WREN) != 0){                                     // check wren bit
            return 0;
        }
        if(++retry > ECGE_ROM_TIMEOUT){                                          // check timeout
            fprintf(stderr, "ERROR: ISP check channel timeout.\n");  
            return -1;
        }
        //sys_delay(1);                                                     		// 10ms
    }
}

/*==============================================================*/
int ecge_isp_write_enable(void)
{
	uint8_t wdata;
	
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_FOLLOW_EN) != 0){                                     // enable ITE SPI follow mode
        fprintf(stderr, "ERROR: Enable follow mode fail\n");
        return -1;
    }
    if(rom_id[0] == FLASH_TYPE_SST){
        if(ecge_isp_write_cmd(FLASH_CMD_EWSR) != 0){
            fprintf(stderr, "ERROR: Enable SST status fail\n");
            return -1;
        }
    }
    
    if(ecge_isp_write_cmd(FLASH_CMD_WRSR) != 0){                                    // set idx to WRSTA
        fprintf(stderr, "ERROR: Failed to access write status.\n");
        return -1;
    }
    
    wdata = 0x00;                                                                         // clear protect: write 0x00 to wrsta
    if(ecge_isp_write_data(&wdata, 1) != 0){
        fprintf(stderr, "ERROR: Failed to clear write status.\n");
        return -1;
    }
    if(ecge_isp_write_cmd(FLASH_CMD_WREN) != 0){                                    // enable write to rom
        fprintf(stderr, "ERROR: Failed to enable write flash.\n");
        return -1;
    }
    
    return 0;
}

/*==============================================================*/
int ecge_isp_write_disable(void)
{
    if(ecge_isp_write_cmd(FLASH_CMD_WRDI) != 0)
        return -1;
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_FOLLOW_DIS) != 0)
        return -1;
    
    return 0;
}

/*==============================================================*/
int ecge_isp_read_rom_id(void)
{
    if(ecge_isp_write_enable() != 0){                                        // enable write to rom
        return -1;
    }

    if(ecge_isp_read_data(FLASH_CMD_RDID, rom_id, FLASH_ID_LEN) != 0){                 // Get ROM ID
        fprintf(stderr, "ERROR: Failed to get ROM ID.\n");
        return -1;
    }
            
    if(ecge_isp_write_disable() != 0){                                                       // disable write to rom
        return -1;
    }
    return 0;
}

/*==============================================================*/
int ecge_isp_check_busy(void)
{
    uint16_t        retry = 0;
    uint8_t         data = 0;
    //fprintf(stderr, "check rom busy...\n");
    while(1){
        if(ecge_isp_read_data(FLASH_CMD_RDSR, &data, 1) != 0)         // read rom status
            return -1;
                
        if((data & FLASH_RDSR_BIT_BUSY) == 0){                                     // check wren bit
            return 0;
        }
        if(++retry > ECGE_ROM_TIMEOUT){                                          // check timeout
            fprintf(stderr, "ERROR: ISP check busy timeout.\n");     
            return -1;
        }
        //sys_delay(1);                                                     // 10ms
    }
}
/*==============================================================*/
int ecge_isp_rom_erase(uint16_t block)
{
	uint8_t	wdata[3];
    //fprintf(stderr, "write enable\n");     
    if(ecge_isp_write_enable() != 0){            // enable write to rom
        return -1;
    }
    //fprintf(stderr, "check write enable\n");     
    if(ecge_isp_check_write_en() != 0){
        return -1;
    }
    
    //fprintf(stderr, "send erase command\n");     
    if(ecge_isp_write_cmd(FLASH_CMD_BE) != 0)
        return -1;
    
    wdata[0] = (uint8_t)block;
    wdata[1] = 0x00;
    wdata[2] = 0x00;
    //fprintf(stderr, "write data\n");     
    if(ecge_isp_write_data(wdata, 3) != 0)
        return -1;
    
    //fprintf(stderr, "check busy\n");     
    if(ecge_isp_check_busy() != 0)
        return -1;                                                                                      // busy timeout
    
    //fprintf(stderr, "write disable\n");     
    if(ecge_isp_write_disable() != 0)                                                    // disable write to rom
        return -1;
            
    return 0;
}
/*==============================================================*/
void ecge_isp_wdt_reset(void)
{
    ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_WDT_RESET);
}

/*==============================================================*/
int ecge_isp_block_program(uint16_t block, uint8_t *buf1, uint8_t *buf2)
{
    uint16_t pagecnt;
    uint16_t idx;
    uint8_t	 wdata[3];
	
    for(pagecnt = 0; pagecnt < ECGE_ROM_PAGECOUNT; pagecnt++){
        if(ecge_isp_write_enable() != 0)             // enable write to rom
            return -1;
        if(ecge_isp_check_write_en() != 0){
            fprintf(stderr, "Check write enable fail.\n");
            return -1;
        }

                
        if(ecge_isp_write_cmd(FLASH_CMD_PP) != 0)
            return -1;
        wdata[0] = (uint8_t)block;
        wdata[1] = (uint8_t)pagecnt;
        wdata[2] = 0x00;
        if(ecge_isp_write_data(wdata, 3) != 0)
            return -1;
        
        if((pagecnt * FLASH_PAGE_SIZE) < dFILE_BUFF_SIZE){
            for(idx = 0; idx < FLASH_PAGE_SIZE; idx++){
                if(ecge_isp_write_data(buf1 + pagecnt * FLASH_PAGE_SIZE + idx, 1) != 0)
                    return -1;
            }
        }
        else {
            for(idx = 0; idx < FLASH_PAGE_SIZE; idx++){
                if(ecge_isp_write_data(buf2 + pagecnt * FLASH_PAGE_SIZE + idx - dFILE_BUFF_SIZE, 1) != 0)
                    return -1;
            }
        }
        
        if(ecge_isp_check_busy() != 0)
            return -1;                                                                                      // busy timeout
        if(ecge_isp_write_disable() != 0)                                                    // disable write to rom
            return -1;
    }
    return 0;
}

/*==============================================================*/
int ecge_isp_block_program_sst(uint16_t block, uint8_t *buf1, uint8_t *buf2)
{
    uint32_t idx;
	uint8_t	 wdata[3];
	
    //fprintf(stderr, "program sst rom block(%d)\n",block);
    if(ecge_isp_write_enable() != 0)         // enable write to rom
		return -1;
	if(ecge_isp_check_write_en() != 0){
		fprintf(stderr, "Check write enable fail.\n");
        return -1;
    }
        
    for(idx = 0; idx < FLASH_BLOCK_SIZE; idx+=2){
        //fprintf(stderr, "block idx%d...\n",idx);
        ecge_isp_write_cmd(FLASH_CMD_AAIP);
        if(idx == 0){                                                                           // send block address
            wdata[0] = (uint8_t)block;                                                                       // adr2
            wdata[1] = 0x00;                                                                 // adr1
            wdata[2] = 0x00;                                                                 // adr0
            if(ecge_isp_write_data(wdata, 3) != 0)
                return -1;
        }
        if(idx < dFILE_BUFF_SIZE){
            if(ecge_isp_write_data(buf1 + idx, 2) != 0)
                return -1;
        }
        else{
            if(ecge_isp_write_data(buf2 + idx - dFILE_BUFF_SIZE, 2) != 0)
                return -1;
        }
        if(ecge_isp_check_busy() != 0)
            return -1;                                                                              // busy timeout
    }
    
    if(ecge_isp_write_disable() != 0)                                                    // disable write to rom
        return -1;
    
    return 0;
}

/*==============================================================*/
int ecge_isp_programming(uint16_t block, uint8_t *buf1, uint8_t *buf2)
{
    if(rom_id[0] == FLASH_TYPE_SST)
        return ecge_isp_block_program_sst(block, buf1, buf2);
    else
        return ecge_isp_block_program(block, buf1, buf2);
}

/*==============================================================*/
int ecge_isp_rom_verify(uint16_t block, uint8_t *buf1, uint8_t *buf2)
{
    uint32_t    addr0;
    uint32_t    addr1;
	uint8_t		*chkbuf1 = (uint8_t *)malloc(dFILE_BUFF_SIZE);
    uint8_t		*chkbuf2 = (uint8_t *)malloc(dFILE_BUFF_SIZE);   
	int		rtn = -1;
	
	if((chkbuf1 == NULL) || (chkbuf2 == NULL)){
		fprintf(stderr, "ERROR: Failed to allocate memory.\n");
		return rtn;
	}
	
    for(addr0 = 0; addr0 < dFILE_BUFF_SIZE; addr0++){                               // init check buf
        *(chkbuf1 + addr0) = 0xff;
        *(chkbuf2 + addr0) = 0xff;
    }
    
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR3) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, 0x00) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR2) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)block) != 0)
        goto exit;

    for(addr1 = 0; addr1 < ECGE_ROM_PAGECOUNT; addr1++){                                 // read
        if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR1) != 0)
            goto exit;
        if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr1) != 0)
            goto exit;

        if(addr1 * FLASH_PAGE_SIZE < dFILE_BUFF_SIZE){
            for(addr0 = 0; addr0 < FLASH_PAGE_SIZE; addr0++){
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_RD_DATA) != 0)
                    goto eReadECRom;
                if(ite_pmc_read_data(&ecge_pmc, chkbuf1 + (addr1 * FLASH_PAGE_SIZE + addr0)) != 0)
                    goto eReadECRom;
            }
        }
        else{
            for(addr0 = 0; addr0 < FLASH_PAGE_SIZE; addr0++){
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_RD_DATA) != 0)
                    goto eReadECRom;
                if(ite_pmc_read_data(&ecge_pmc, chkbuf2 + (addr1 * FLASH_PAGE_SIZE + addr0 - dFILE_BUFF_SIZE)) != 0)
                    goto eReadECRom;
            }
        }
    }
    
    for(addr0 = 0; addr0 < FLASH_BLOCK_SIZE; addr0++){
        if(addr0 < dFILE_BUFF_SIZE){
            if(*(buf1 + addr0) != *(chkbuf1 + addr0)){
                goto eVerifyRom;
            }
        }
        else{
            if(*(buf2 + addr0 - dFILE_BUFF_SIZE) != *(chkbuf2 + addr0 - dFILE_BUFF_SIZE)){
                goto eVerifyRom;
            }
        }
    }
	rtn = 0;
	goto exit;
	
eVerifyRom:
    if(addr0 < dFILE_BUFF_SIZE)
        fprintf(stderr, "\ncheckbuf=0x%02x\tfilebuf=0x%02x",*(chkbuf1 + addr0),*(buf1 + addr0));
    else
        fprintf(stderr, "\ncheckbuf=0x%02x\tfilebuf=0x%02x",*(chkbuf2 + addr0 - dFILE_BUFF_SIZE),*(buf2 + addr0 - dFILE_BUFF_SIZE));
	
eReadECRom:
    fprintf(stderr, "\nVerify Fail\n");
    fprintf(stderr, "Address: 0x%08x\n",block * 0x10000 + addr0);
	
	
exit:
	if(chkbuf1 != NULL)
		free(chkbuf1);
	if(chkbuf2 != NULL)
		free(chkbuf2);
    return rtn;
}

/*==============================================================*/
int ecge_isp_check_erase(uint16_t block)
{
    uint32_t    addr0;
    uint32_t    addr1;
	uint8_t		*chkbuf1 = (uint8_t *)malloc(dFILE_BUFF_SIZE);
    uint8_t		*chkbuf2 = (uint8_t *)malloc(dFILE_BUFF_SIZE);   
	int		rtn = -1;
	
	if((chkbuf1 == NULL) || (chkbuf2 == NULL)){
		fprintf(stderr, "ERROR: Failed to allocate memory.\n");
		return rtn;
	}
	
    for(addr0 = 0; addr0 < dFILE_BUFF_SIZE; addr0++){                               // init check buf
        *(chkbuf1 + addr0) = 0xff;
        *(chkbuf2 + addr0) = 0xff;
    }
    
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR3) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, 0x00) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR2) != 0)
        goto exit;
    if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)block) != 0)
        goto exit;

    for(addr1 = 0; addr1 < ECGE_ROM_PAGECOUNT; addr1++){                                 // read
        if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR1) != 0)
            goto exit;
        if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr1) != 0)
            goto exit;

        if((addr1 * FLASH_PAGE_SIZE) < dFILE_BUFF_SIZE){
            for(addr0 = 0; addr0 < FLASH_PAGE_SIZE; addr0++){
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_RD_DATA) != 0)
                    goto eReadECRom;
                if(ite_pmc_read_data(&ecge_pmc, chkbuf1 + (addr1 * FLASH_PAGE_SIZE + addr0)) != 0)
                    goto eReadECRom;
				if(*(chkbuf2 + (addr1 * FLASH_PAGE_SIZE + addr0)) != 0xFF)
					goto eVerifyRom;
			}
        }
        else{
            for(addr0 = 0; addr0 < FLASH_PAGE_SIZE; addr0++){
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_SET_ADDR0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, (uint8_t)addr0) != 0)
                    goto eReadECRom;
                if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_RD_DATA) != 0)
                    goto eReadECRom;
                if(ite_pmc_read_data(&ecge_pmc, chkbuf2 + (addr1 * FLASH_PAGE_SIZE + addr0 - dFILE_BUFF_SIZE)) != 0)
                    goto eReadECRom;
            }
			if(*(chkbuf2 + (addr1 * FLASH_PAGE_SIZE + addr0 - dFILE_BUFF_SIZE)) != 0xFF)
				goto eVerifyRom;
        }
	}
 	rtn = 0;
	goto exit;
	
eVerifyRom:
    if((addr1 * FLASH_PAGE_SIZE) < dFILE_BUFF_SIZE)
        fprintf(stderr, "\ncheckbuf=0x%02x",*(chkbuf2 + (addr1 * 0x100 + addr0)));
    else
        fprintf(stderr, "\ncheckbuf=0x%02x",*(chkbuf2 + (addr1 * 0x100 + addr0 - dFILE_BUFF_SIZE)));
	
eReadECRom:
    fprintf(stderr, "\nVerify Fail\n");
    fprintf(stderr, "Address: 0x%08x\n",block * 0x10000 + addr1 * 0x100 + addr0);
	
exit:
	if(chkbuf1 != NULL)
		free(chkbuf1);
	if(chkbuf2 != NULL)
		free(chkbuf2);
    return rtn;
}

/*==============================================================*/
// ECG-EC General Function
/*==============================================================*/
embeded_t ecge_ite_pjt_decode(ite_pjt_t *pjt)
{
	embeded_t infotemp;
	
	sprintf(infotemp.project_name, "%8s", pjt->name);
	
	//sprintf(infotemp.ver, "%c%02X%02X%04X",pjt->ver[3], pjt->ver[1], pjt->ver[2], (uint16_t)pjt->kver[0] << 8 | pjt->kver[1]);
	sprintf(infotemp.ver, "%c%02X%02X%c%02X%02X", pjt->chip[0], pjt->chip[1], pjt->id, pjt->ver[3], pjt->ver[1], pjt->ver[2]);
		
	infotemp.id = pjt->id;

	switch(pjt->chip[0]){
		case 'I':
			if(pjt->chip[1] == 0x18){
				sprintf(infotemp.chip, "%10s", "ITE-IT8518");
			}
			else if(pjt->chip[1] == 0x25){
				sprintf(infotemp.chip, "%10s", "ITE-IT8525");
			}
			else if(pjt->chip[1] == 0x28){
				sprintf(infotemp.chip, "%10s", "ITE-IT8528");
			}
			else{
				sprintf(infotemp.chip, "%10s", " ");
			}
			break;
		default:
			sprintf(infotemp.chip, "%10s", " ");
			break;
	}
	
	return infotemp;
}
/*==============================================================*/
int ecge_mobx_get_embedic_info(ite_pjt_t *pjt)
{
	uint8_t		r_idx  = 0;
//	uint8_t		ver[8];

	if(ecge_mbox_write_cmd(ECGE_MBOX_CMD_F0h) != 0){
		return -1;
	}
	
	// Project name
	if(ecge_mbox_read_data(r_idx, pjt->name, PJT_NAME_SIZE) != 0){
		return -1;
	}
	pjt->name[PJT_NAME_SIZE] = '\0';

	// Version table 
	r_idx = PJT_NAME_SIZE;
	if(ecge_mbox_read_data(r_idx, pjt->ver, 1) != 0){
		return -1;
	}

	// Kernel Version
	r_idx++;												// skip kernel version
	//if(ecge_mbox_read_data(r_idx, pjt->kver, 2) != 0){
	//	return -1;
	//}
	
	// Chip Code
	r_idx += 2;
	if(ecge_mbox_read_data(r_idx, pjt->chip, 2) != 0){
		return -1;
	}

	// Project ID
	r_idx += 2;
	if(ecge_mbox_read_data(r_idx, &pjt->id, 1) != 0){
		return -1;
	}

	// Firmware Ver
	r_idx++;
	if(ecge_mbox_read_data(r_idx, &pjt->ver[3], 1) != 0){
		return -1;
	}
	r_idx++;
	if(ecge_mbox_read_data(r_idx, &pjt->ver[1], 2) != 0){
		return -1;
	}

	// old firmware version : A01011317
	// new firmware version : I2895A0101 (I 28 95 A 01 01)
	
	//	uint8_t	tab_code;		// table code
	//	uint8_t kver_major;		// kernel version
	//	uint8_t	kver_minor;
	//	uint8_t chip_vendor;	// 'I'=ITE
	//	uint8_t chip_id;		// 28h=8528
	//	uint8_t	prj_id;			// project id
	//	uint8_t type;			// project type : V=formal, X=test, A=OEM...
	//	uint8_t ver_major;		// project version
	//	uint8_t ver_minor;

//	r_idx = PJT_NAME_SIZE + 3;
//	if(ecge_mbox_read_data(r_idx, ver, 6) != 0)
//		return -1;
//sprintf(pjt->ver, "%c%02X%02X%c%02X%02X", ver[0], ver[1], ver[2], ver[3], ver[4], ver[5]);


	return 0;
}
/*==============================================================*/
int ecge_acpi_get_embedic_info(ite_pjt_t *pjt)
{
//	uint8_t		ver[8];

	// Version table 
	if (ReadACPIByte(ECGE_ACPI_ADDR_VERTBL, pjt->ver) != 0)
		return -1;

	// Chip Code
	if (ReadACPIByte(ECGE_ACPI_ADDR_CHIP1, &pjt->chip[0]) != 0)
		return -1;
	if (ReadACPIByte(ECGE_ACPI_ADDR_CHIP2, &pjt->chip[1]) != 0)
		return -1;

	// Project ID
	if (ReadACPIByte(ECGE_ACPI_ADDR_PJTID, &pjt->id) != 0)
		return -1;

	// Firmware Ver
	if (ReadACPIByte(ECGE_ACPI_ADDR_VER1, &pjt->ver[1]) != 0)
		return -1;
	if (ReadACPIByte(ECGE_ACPI_ADDR_VER2, &pjt->ver[2]) != 0)
		return -1;
	if (ReadACPIByte(ECGE_ACPI_ADDR_PJTTYPE, &pjt->ver[3]) != 0)
		return -1;
/*
	sprintf(ver, "%c%02X%02X%c%02X%02X",
			pjt->chip[0], pjt->chip[1], pjt->id,
			pjt->ver[3], pjt->ver[1], pjt->ver[2]);

	sprintf(pjt->ver, "%s", ver);
*/
	
	return 0;
}
/*==============================================================*/
int ecge_get_embedic_info(embeded_t *fw_info)
{
	ite_pjt_t	pjt;
	uint16_t	kver;
	pjt.kver[0] = 0;
	pjt.kver[1] = 0;

	if(ite_read_ec_ram(ECGE_ACPI_ADDR_KVER1, &pjt.kver[0]) != 0)
		return -1;

	if(ite_read_ec_ram(ECGE_ACPI_ADDR_KVER2, &pjt.kver[1]) != 0)
		return -1;

	kver = pjt.kver[0] << 8 | pjt.kver[1];

	if (kver >= 0x1105)
	{
		if (ecge_mobx_get_embedic_info(&pjt) != 0)
			return -1;
	}
	else
	{
		ecge_flag = 1;									// unsupport mbox, must be use general pmc access
		if (ecge_acpi_get_embedic_info(&pjt) != 0)
			return -1;
	}

	*fw_info = ecge_ite_pjt_decode(&pjt);
	return 0;
}
/*==============================================================*/
int ecge_get_bin_info(binary_t *bin)
{
	uint8_t		itestr[ECGE_SIGN_LEN];
	uint8_t		itestr_def[ECGE_SIGN_LEN] = ECGE_SIGN_STR;
	ite_pjt_t	pjt;
	
	// Check ITE String
	fseek(bin->ctrl, ECGE_SIGN_ADDR, SEEK_SET);
	if(fread(itestr, 1, ECGE_SIGN_LEN, bin->ctrl) != ECGE_SIGN_LEN){
		fprintf(stderr, "ERROR: Read binary file fail.\n");
        return -1;
	}
	if(memcmp(itestr_def, itestr, ECGE_SIGN_LEN) != 0){
		fprintf(stderr, "ERROR: Invalid binary file.\n");
        return -1;
	}
	
	// Get ITE EC ROM Header
    fseek(bin->ctrl, ECGE_ROM_HEADER_ADDR, SEEK_SET);
    if(fread((uint8_t *)&pjt, 1, ECGE_ROM_HEADER_LEN, bin->ctrl) != ECGE_ROM_HEADER_LEN){           // 15 = 2(kver) + 1(id) + 4(ver) + 8(name) + 1(chip vendor) + 1(chip id)
        fprintf(stderr, "ERROR: Read binary file fail.\n");
        return -1;
    }
	pjt.chip[1] = pjt.chip[0];							//align chip id
	pjt.chip[0] = pjt.name[PJT_NAME_SIZE];				//align chip vendor
	pjt.name[PJT_NAME_SIZE] = '\0';                            // let name to string

    bin->finfo = ecge_ite_pjt_decode(&pjt);
    rewind(bin->ctrl);                                                  // init file cursor postion
    return 0;
}

/*==============================================================*/
int ecge_ec_update(binary_t *bin, isp_flag_t upflag)
{
    int         rtn = -1;
	int			flag = 0;		// bit0 = 1: dis_kbc, bit1 = 1: enter ISP mode
    uint16_t    blk;
    uint16_t    cnt;
    uint16_t    blocknum;
    uint8_t		*fbuf1 = (uint8_t *)malloc(dFILE_BUFF_SIZE);
    uint8_t		*fbuf2 = (uint8_t *)malloc(dFILE_BUFF_SIZE);
	uint8_t		count = 50;

	if((fbuf1 == NULL) || (fbuf2 == NULL)){
		fprintf(stderr, "ERROR: Failed to allocate memory.\n");
		goto eFlash;
	}

#ifdef _DOS_
	flag = ecge_disable_port6064();
	if(flag == -1){
		return -1;
	}
#endif //_DOS_

	if(ecge_isp_entry() != 0){                                                      // enter ISP mode
        fprintf(stderr, "ERROR: Enter ISP mode fail.\n");
		goto eFlash;
    }
	flag |= 0x02;
	
	if(ecge_isp_read_rom_id() != 0){                         // get new rom_id
        fprintf(stderr, "ERROR: ISP get ROM ID fail\n");
        goto eFlash;
    }

	blocknum = (uint16_t)(bin->fsize / FLASH_BLOCK_SIZE);
	printf("Application:\n");
    for(blk = 0; blk < blocknum; blk++){
		fprintf(stderr,"  Erase ...\t\t\t%.1f%%\r", (float)blk / blocknum * 100);

        if(ecge_isp_rom_erase(blk) != 0)                                             // erase rom
            goto eFlash;
    }
	fprintf(stderr,"  Erase ...\t\t\tDone.                \n");
	
    for(blk = 0; blk < blocknum; blk++){
		fprintf(stderr,"  Program & Verify...\t\t%.1f%%\r", (float)blk / blocknum * 100);
		if(fread(fbuf1, 1, dFILE_BUFF_SIZE, bin->ctrl) != dFILE_BUFF_SIZE){	// read file
			fprintf(stderr, "ERROR: Read bin file fail.\n(1-%d)", blk);
			goto eFlash;
		}
		if(fread(fbuf2, 1, dFILE_BUFF_SIZE, bin->ctrl) != dFILE_BUFF_SIZE){
			fprintf(stderr, "ERROR: Read bin file fail.\n(2-%d)", blk);
			goto eFlash;
		}

		for(cnt = 0; cnt < dFILE_BUFF_SIZE; cnt++){                             // check file data 
            if((*(fbuf1 + cnt) != 0xff) || (*(fbuf2 + cnt) != 0xff))
                break;
        }

        if(cnt != dFILE_BUFF_SIZE){                                                             // skip if all byte is 0xff in block
            //fprintf(stderr, "\nProgramming...");
            if(ecge_isp_programming(blk, fbuf1, fbuf2) != 0)                                     // programming
                goto eFlash;
                                
            //fprintf(stderr, "\nVerify...");
            if(ecge_isp_rom_verify(blk, fbuf1, fbuf2) != 0)                                          // verify
                goto eFlash;
        }
    }
	fprintf(stderr,"  Program & Verify...\t\tDone.                \n");

    rtn = 0;

	fprintf(stderr, "\n\nEC firmware update successful.");                                                                   // isp done
	fprintf(stderr, "\n***\tPlease Remove All Power Source,\t\t***");
	fprintf(stderr, "\n***\tTo Complete the Update Process.\t\t***\n\n");

eFlash:
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
#if defined(_WINDOW_) || defined(_LINUX_)
	if(flag & 0x02)
	{
		switch(gResetMode)
		{
			case 0:
			case 1:		//os power off
			{
				ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_GO_EC_MAIN);
				OSPowerOff();
				break;
			}
			case 2:		// exit ISP mode and goto ec_main
			{
				if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_GO_EC_MAIN) != 0)
				{
					fprintf(stderr, "ERROR: Exit ISP mode fail.\n");
				}
				sys_delay(200);
				break;
			}
			case 3:		//platform reset
			{
				ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_GO_EC_MAIN);
				sys_reset();
				break;
			}
			case 4:		//kbc reset
			{
				ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_GO_EC_MAIN);
				KBCReset();
				break;
			}
			case 5:		//ec reset
			{
				ecge_isp_wdt_reset();
				break;
			}
		}
	}
#else
	if(flag & 0x02){
		sys_reset_notify();
		if(ite_pmc_write_cmd(&ecge_pmc, ECGE_ISP_GO_EC_MAIN) != 0){                                          // exit ISP mode and goto ec_main
			fprintf(stderr, "ERROR: Exit ISP mode fail.\n");
		}
		sys_delay(200);
	}

#ifdef _DOS_
	if(flag & 0x01)
		ecge_enable_port6064();
#endif // _DOS_	
	
	if(flag & 0x02)
		sys_reset();
#endif
                                                                                                       // flash error
	if(fbuf1 != NULL)
		free(fbuf1);
	if(fbuf2 != NULL)
		free(fbuf2);
    return rtn;
}
/*==============================================================*/
// Check Model Name
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecge_ec_check_model_name(binary_t *bin, embeded_t *fw_info)
{
	if(memcmp((const char *)bin->finfo.project_name, (const char *)fw_info->project_name, PJT_NAME_SIZE) != 0){
		fprintf(stderr, "ERROR: The firmware image file is broken.\n");
		return -1;
	}
	return 0;
}
/*==============================================================*/
int ecge_ec_init(void)
{
	if(pmc_open_port(&ecge_pmc) != 0){
        return -1;
    }
	return 0;
}

/*==============================================================*/
void ecge_ec_exit(void)
{
	pmc_close_port(&ecge_pmc);
}

