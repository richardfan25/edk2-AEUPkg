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
#include "ecgi.h"

/*==============================================================*/

const chip_t ecgi_chips[1] ={
	{
		ITE_VID,						/* vid */
		ITE_PID_5121,					/* pid */
	},
};
 
const codebase_t ecgi_cb = {
	sizeof(ecgi_chips)/sizeof(chip_t),	/* chip_num */
	&ecgi_chips[0],						/* chip_list */
	ECGIC_CB_ID,						/* cbid */
	{									/* func */
		&ecgi_ec_init,						/* init */
		&ecgi_ec_exit,						/* exit */
		&ecgi_get_embedic_info,				/* get_target_info */
		&ecgi_get_bin_info,					/* get_binary_info */
		&ecgi_ec_update,					/* update_firmware */
		&ecgi_ec_check_model_name,			/* check_model_name */
	},
};

static pmc_port_t 	ecgi_pmc = {ECGE_PMC_CMD_PORT, ECGE_PMC_DAT_PORT};
static uint8_t 		rom_id[FLASH_ID_LEN + 1];
static uint8_t		ecgi_flag = 0;
static uint8_t		ecgi_hwoffset = 0;
static uint8_t		*ecgi_filebuffer = NULL;
#ifdef _DOS_
/*==============================================================*/
// Enable 60/64 port
//
// Input:       none
// Output:      none
/*==============================================================*/
int ecgi_enable_port6064(void)
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
	fprintf(stderr, "ERROR: Fail to find Intel ISA Bridge.\n");
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
int ecgi_disable_port6064(void)
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
	fprintf(stderr, "ERROR: Fail to find Intel ISA Bridge.\n");
	return -1;
}
#endif // _DOS_
/*==============================================================*/
// ECG-EC Mailbox Function
/*==============================================================*/
int ecgi_mbox_check_busy(void)
{
	uint8_t	 u8temp;
	uint16_t cnt;
	
	for(cnt = 0; cnt < ECGI_MBOX_RETRY; cnt++)
	{
		inp(ecgi_pmc.data);      //clear obf
		outp(ecgi_pmc.cmd, MBX_OFFSET_CMD + ecgi_hwoffset);
		u8temp = inp(ecgi_pmc.data);

		if(u8temp == 00)
			return 0;
		sys_delay(1);
	}
	return -1;
}

/*==============================================================*/
int ecgi_mbox_port_write(uint8_t offset, uint8_t data)
{
	inp(ecgi_pmc.data);      //clear obf
	outp(ecgi_pmc.cmd, ecgi_hwoffset + offset);
    outp(ecgi_pmc.data, data);
	
	return 0;
}

/*==============================================================*/
int ecgi_mbox_port_read(uint8_t offset, uint8_t *data)
{
	inp(ecgi_pmc.data);      //clear obf
	outp(ecgi_pmc.cmd, ecgi_hwoffset + offset);
	*data = inp(ecgi_pmc.data);
	
	return 0;
}

/*==============================================================*/
// ECG-EC ISP Function
/*==============================================================*/
/*==============================================================*/
int ecgi_isp_entry(void)
{
	return ite_pmc_write_cmd(&ecgi_pmc, ECGE_PCMD_ISP);
}

/*==============================================================*/
int ecgi_isp_write_cmd(uint8_t cmd)
{
	if(ite_pmc_write_cmd(&ecgi_pmc, cmd) != 0)
	{
		return -1;
	}
    
	return 0;
}

/*==============================================================*/
int ecgi_isp_write_data(uint8_t data)
{
	if(ite_pmc_write_data(&ecgi_pmc, data) != 0)
	{
		return -1;
	}
    
	return 0;
}

/*==============================================================*/
int ecgi_isp_read_data(uint8_t *buf)
{
	if(ite_pmc_read_data(&ecgi_pmc, buf) != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_send_file_and_crypt_data(FILE *fi, uint32_t size)
{
	int icount;
	uint8_t fdata[ECGI_ISP_SECURITY_MD2_BLOCK];
	uint8_t dtemp = 0;
	uint32_t filepos = 0;
	
	//step 1
	if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_INIT_MD2) != 0)
	{
		return -1;
	}
	if(ecgi_isp_read_data(&dtemp) != 0)
	{
		return -1;
	}
	if(dtemp != ECGI_ISP_SECURITY_SUCCESS_ACK)
	{
		return -1;
	}
	
	fseek(fi, 0, SEEK_SET);
	
	while(1)
	{
		if(fread(fdata, 1, ECGI_ISP_SECURITY_MD2_BLOCK, fi) != ECGI_ISP_SECURITY_MD2_BLOCK)
		{
			return -1;
		}
		
		filepos += ECGI_ISP_SECURITY_MD2_BLOCK;
		if(filepos <= size)
		{
			//step 2
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_INPUT_DATA_FOR_MD2) != 0)
			{
				return -1;
			}
			for(icount = 0; icount < ECGI_ISP_SECURITY_MD2_BLOCK; icount++)
			{
				if(ecgi_isp_write_data(fdata[icount]) != 0)
				{
					return -1;
				}
			}
			
			//step 3
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_MD2_UPDATE) != 0)
			{
				return -1;
			}
			if(ecgi_isp_read_data(&dtemp) != 0)
			{
				return -1;
			}
			if(dtemp != ECGI_ISP_SECURITY_SUCCESS_ACK)
			{
				return -1;
			}
		}
		else
		{
			//step 4
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_MD2_FINISH) != 0)
			{
				return -1;
			}
			if(ecgi_isp_read_data(&dtemp) != 0)
			{
				return -1;
			}
			if(dtemp != ECGI_ISP_SECURITY_SUCCESS_ACK)
			{
				return -1;
			}
			
			//step 5
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_INPUT_ENCRYPT_DATA) != 0)
			{
				return -1;
			}
			for(icount = 0; icount < ECGI_ISP_SECURITY_MD2_BLOCK; icount++)
			{
				if(ecgi_isp_write_data(fdata[icount]) != 0)
				{
					return -1;
				}
			}
			
			//step 6
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_DECRYPT_DATA) != 0)
			{
				return -1;
			}
			if(ecgi_isp_read_data(&dtemp) != 0)
			{
				return -1;
			}
			if(dtemp != ECGI_ISP_SECURITY_SUCCESS_ACK)
			{
				return -1;
			}
			
			//step 7
			if(ecgi_isp_write_cmd(ECGI_ISP_SECURITY_CHECK_AND_START_FLASH) != 0)
			{
				return -1;
			}
			if(ecgi_isp_read_data(&dtemp) != 0)
			{
				return -1;
			}
			if(dtemp == ECGI_ISP_STANDARD_FLASH)
			{
				break;
			}
			else
			{
				return -1;
			}
		}
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_enter_follow_mode(void)
{
	if(ecgi_isp_write_cmd(0x01) != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_exit_follow_mode(void)
{
	if(ecgi_isp_write_cmd(0x05) != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_send_spi_command(uint8_t cmd)
{
	if(ecgi_isp_write_cmd(0x02) != 0)
	{
		return -1;
	}
	if(ecgi_isp_write_cmd(cmd) != 0)
	{
		return -1;
	}
    
	return 0;
}

/*==============================================================*/
int ecgi_isp_send_spi_byte(uint8_t index)
{
	if(ecgi_isp_write_cmd(0x03) != 0)
	{
		return -1;
	}
	if(ecgi_isp_write_cmd(index) != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_read_from_spi(uint8_t *buf)
{
	if(ecgi_isp_write_cmd(0x04) != 0)
	{
		return -1;
	}
	if(ecgi_isp_read_data(buf) != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_wait_spi_free(void)
{
	uint8_t data;
	
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_READ_STATUS) != 0)
	{
		return -1;
	}
	
	while(1)
	{
		if(ecgi_isp_read_from_spi(&data) != 0)
		{
			return -1;
		}
		if((data & 0x01) == 0x00)
		{
			break;
		}
	}
	
	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_read_device_id_cmd_ab(void)
{
	uint8_t index;
	uint8_t tmp;
	
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_RDID) != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_send_spi_byte(0x00) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(0x00) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(0x00) != 0)
	{
		return -1;
	}
	
	for(index=0x00; index<4; index++)
	{
		if(ecgi_isp_read_from_spi(&tmp) != 0)
		{
			return -1;
		}
		rom_id[index] = tmp;
	}
	
	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_read_device_id(void)
{
	uint8_t index;
	uint8_t tmp;

	rom_id[0] = 0x00;
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_DEVICE_ID) != 0)
	{
		return -1;
	}
	
	for(index=0x00; index<4; index++)
	{
		if(ecgi_isp_read_from_spi(&tmp) != 0)
		{
			return -1;
		}
		rom_id[index] = tmp;
	}
	if(rom_id[0] == 0x00)
	{
		if(ecgi_isp_read_device_id_cmd_ab() != 0)
		{
			return -1;
		}
	}

	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}

	return 0;
}

/*==============================================================*/
int ecgi_isp_spi_unlock_All(void)
{
	uint8_t tmp;
	
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_WREN) != 0)
	{
		return -1;
	}

	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_READ_STATUS) != 0)
	{
		return -1;
	}
	while(1)
	{
		if(ecgi_isp_read_from_spi(&tmp) != 0)
		{
			return -1;
		}
		if((tmp & 0x02) != 0x00)
		{
			break;
		}
	}

	if(rom_id[0] == ECGI_SSTID)
	{
		if(ecgi_isp_enter_follow_mode() != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_EWSR) != 0)
		{
			return -1;
		}
	}

	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_WRSR) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(0x00) != 0)
	{
		return -1;
	}

	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}

	return 0;
}

/*==============================================================*/
int ecgi_isp_spi_write_enable(void)
{
	uint8_t tmp;
	
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_WREN) != 0)
	{
		return -1;
	}

	if(rom_id[0] == ECGI_SSTID)
	{
		if(ecgi_isp_enter_follow_mode() != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_EWSR) != 0)
		{
			return -1;
		}
	}

	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_READ_STATUS) != 0)
	{
		return -1;
	}
	while(1)
	{
		if(ecgi_isp_read_from_spi(&tmp) != 0)
		{
			return -1;
		}
		if((tmp & 0x03) == 0x02)
		{
			break;
		}
	}
	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}

	return 0;
}

/*==============================================================*/
int ecgi_isp_spi_write_disable(void)
{
	uint8_t tmp;
	
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
    if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_WRDI) != 0)
	{
		return -1;
	}

    if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_READ_STATUS) != 0)
	{
		return -1;
	}
    while(1)
    {
		if(ecgi_isp_read_from_spi(&tmp) != 0)
		{
			return -1;
		}
        if((tmp & 0x02) == 0x00)
        {
            break;
        }
    }
	
    if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}

	return 0;
}

/*==============================================================*/
int ecgi_isp_spi_sector_erase(unsigned long address, uint8_t eraseCmd)
{
	uint8_t addr2;
	uint8_t addr1;
	uint8_t addr0;

	addr2 = (uint8_t)(address >> 16 & 0xFF);
	addr1 = (uint8_t)(address >> 8 & 0xFF);
	addr0 = (uint8_t)(address >> 0 & 0xFF);

	if(ecgi_isp_spi_write_enable() != 0)
	{
		return -1;
	}
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	//if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_04KByteBE) != 0)
	//{
	//	return -1;
	//}
	//if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_04KByteBE) != 0)
	//{
	//	return -1;
	//}
	if(ecgi_isp_send_spi_command(eraseCmd) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr2) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr1) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr0) != 0)
	{
		return -1;
	}
	
	if(ecgi_isp_exit_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
int ecgi_isp_byte_program_4K_from_file(unsigned long address, FILE *fi)
{
	unsigned long icount;
	uint8_t addr2;
	uint8_t addr1;
	uint8_t addr0;
	unsigned long adrtemp;

	fseek(fi, (long int)address, 0);

	for(icount=0; icount<ECGI_BlockSize21_4K; icount++)
	{
		ecgi_filebuffer[icount] = 0xff;
	}

	fread(ecgi_filebuffer, 1, ECGI_BlockSize21_4K, fi);
	for(icount=0; icount<ECGI_BlockSize21_4K; icount++)
	{
		adrtemp = address + icount;
		addr2 = (uint8_t)(adrtemp >> 16 & 0xFF);
		addr1 = (uint8_t)(adrtemp >> 8 & 0xFF);
		addr0 = (uint8_t)(adrtemp >> 0 & 0xFF);

		if(ecgi_isp_spi_write_enable() != 0)
		{
			return -1;
		}
		//if(ecgi_isp_wait_spi_free() != 0)
		//{
		//	return -1;
		//}
		if(ecgi_isp_enter_follow_mode() != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_BYTE_PROGRAM) != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_byte(addr2) != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_byte(addr1) != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_byte(addr0) != 0)
		{
			return -1;
		}
		if(ecgi_isp_send_spi_byte(ecgi_filebuffer[icount]) != 0)
		{
			return -1;
		}

		if(ecgi_isp_wait_spi_free() != 0)
		{
			return -1;
		}
	}

	if(ecgi_isp_spi_write_disable() != 0)
	{
		return -1;
	}

	return 0;
}

/*==============================================================*/
int ecgi_isp_spi_verify_4K(unsigned long address, FILE *fi)
{
	unsigned long icount;
	uint8_t addr2;
	uint8_t addr1;
	uint8_t addr0;
	unsigned long adrtemp;
	uint8_t dtemp;

	for(icount=0; icount<ECGI_BlockSize21_4K; icount++)
	{
		ecgi_filebuffer[icount] = 0xff;
	}

	//read from file
	fseek(fi, (long int)address, 0);
	fread(ecgi_filebuffer, 1, ECGI_BlockSize21_4K, fi);

	adrtemp = address;
	addr2 = (uint8_t)(adrtemp >> 16 & 0xFF);
	addr1 = (uint8_t)(adrtemp >> 8 & 0xFF);
	addr0 = (uint8_t)(adrtemp >> 0 & 0xFF);

	if(ecgi_isp_spi_write_disable() != 0)
	{
		return -1;
	}

	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	if(ecgi_isp_enter_follow_mode() != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_command(ECGI_ISP_SPI_HIGH_SPEED_READ) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr2) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr1) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(addr0) != 0)
	{
		return -1;
	}
	if(ecgi_isp_send_spi_byte(0x00) != 0)	// fast read dummy byte
	{
		return -1;
	}

	for(icount=0; icount<ECGI_BlockSize21_4K; icount++)
	{
		if(ecgi_isp_read_from_spi(&dtemp) != 0)
		{
			return -1;
		}
		if(ecgi_filebuffer[icount] != dtemp)
		{
			return -1;
		}
	}

	if(ecgi_isp_wait_spi_free() != 0)
	{
		return -1;
	}
	
	return 0;
}

/*==============================================================*/
void ecgi_isp_wdt_reset(void)
{
    ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_WDT_RESET);
}

/*==============================================================*/
// ECG-EC General Function
/*==============================================================*/
embeded_t ecgi_ite_pjt_decode(ecgi_pjt_t *pjt)
{
	embeded_t infotemp;
	
	sprintf(infotemp.project_name, "%8s", pjt->name);
	
	//sprintf(infotemp.ver, "%c%02X%02X%04X",pjt->ver[3], pjt->ver[1], pjt->ver[2], (uint16_t)pjt->kver[0] << 8 | pjt->kver[1]);
	sprintf(infotemp.ver, "%c%02X%02X%c%02X%02X", pjt->chip[0], pjt->chip[1], pjt->id, pjt->ver[3], pjt->ver[1], pjt->ver[2]);
	infotemp.id = pjt->id;

	switch(pjt->chip[0]){
		case 'I':
			if(pjt->chip[1] == 0x21){
				sprintf(infotemp.chip, "%10s", "ITE-IT5121");
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
int ecgi_mobx_get_embedic_info_method1(ecgi_pjt_t *pjt)
{
	uint8_t i;
	uint8_t r_idx  = 0;
	int ret = 0;
//	uint8_t		ver[8];
	
	ecgi_pmc.cmd = ECGI_PMC_CMD_PORT;
	ecgi_pmc.data = ECGI_PMC_DAT_PORT;
	
	ecgi_mbox_check_busy();
	if(ecgi_mbox_port_write(MBX_OFFSET_CMD, ECGE_MBOX_CMD_F0h) != 0)
	{
		goto fail;
	}
	ecgi_mbox_check_busy();
	
	// Project name
	for(i=0; i<PJT_NAME_SIZE; i++)
	{
		if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->name[i]) != 0)
		{
			goto fail;
		}
		r_idx++;
	}
	pjt->name[PJT_NAME_SIZE] = '\0';

	// Version table 
	r_idx = PJT_NAME_SIZE;
	if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->ver[0]) != 0)
	{
		goto fail;
	}
	r_idx++;

	// Kernel Version
	for(i=0; i<2; i++)	// skip kernel version
	{
		//if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->kver[i]) != 0)
		//{
		//	goto fail;
		//}
		r_idx++;
	}
	
	// Chip Code
	for(i=0; i<2; i++)
	{
		if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->chip[i]) != 0)
		{
			goto fail;
		}
		r_idx++;
	}

	// Project ID
	if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->id) != 0)
	{
		goto fail;
	}
	r_idx++;

	// Firmware Ver
	if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->ver[3]) != 0)
	{
		goto fail;
	}
	r_idx++;
	
	for(i=0; i<2; i++)
	{
		if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &pjt->ver[i + 1]) != 0)
		{
			goto fail;
		}
		r_idx++;
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
			
//	for(i=0, r_idx=PJT_NAME_SIZE+3; i<6; i++)
//	{
//		if(ecgi_mbox_port_read(MBX_OFFSET_DATA + r_idx, &ver[i]) != 0)
//		{
//			goto fail;
//		}
//		r_idx++;
//}
	
//	sprintf(pjt->ver, "%c%02X%02X%c%02X%02X", ver[0], ver[1], ver[2], ver[3], ver[4], ver[5]);
	

end:
	ecgi_pmc.cmd = ECGE_PMC_CMD_PORT;
	ecgi_pmc.data = ECGE_PMC_DAT_PORT;
	return ret;
	
fail:
	ret = -1;
	goto end;
}
/*==============================================================*/
int ecgi_mobx_get_embedic_info(ecgi_pjt_t *pjt)
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
//if(ecge_mbox_read_data(r_idx, ver, 6) != 0)
		//return -1;
//	sprintf(pjt->ver, "%c%02X%02X%c%02X%02X", ver[0], ver[1], ver[2], ver[3], ver[4], ver[5]);

	return 0;
}
/*==============================================================*/
int ecgi_acpi_get_embedic_info(ecgi_pjt_t *pjt)
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

//	sprintf(ver, "%c%02X%02X%c%02X%02X",
//			pjt->chip[0], pjt->chip[0], pjt->id,
//			pjt->ver[3], pjt->ver[1], pjt->ver[2]);

//	sprintf(pjt->ver, "%s", ver);

	return 0;
}

/*==============================================================*/
int ecgi_mbx_check_access_method(uint8_t method)
{
	uint8_t tmp;
	int ret = 0;
	
	if(method == 0)//29E/29F
	{
		ecgi_pmc.cmd = ECGI_PMC_CMD_PORT;
		ecgi_pmc.data = ECGI_PMC_DAT_PORT;
		
		if(pmc_open_port(&ecgi_pmc) != 0)
		{
			goto fail;
		}
		ecgi_hwoffset = 0x80;
		if(ecgi_mbox_port_write(MBX_OFFSET_CMD, ECGI_MBOX_CMD_C0h) != 0)
		{
			goto fail;
		}
		ecgi_mbox_check_busy();
		if(ecgi_mbox_port_read(MBX_OFFSET_STA, &tmp) != 0)
		{
			goto fail;
		}
		if(tmp != 1)
		{
			ecgi_hwoffset = 0x00;
			if(ecgi_mbox_port_write(MBX_OFFSET_CMD, ECGI_MBOX_CMD_C0h) != 0)
			{
				goto fail;
			}
			ecgi_mbox_check_busy();
			if(ecgi_mbox_port_read(MBX_OFFSET_STA, &tmp) != 0)
			{
				goto fail;
			}
			if(tmp != 1)
			{
				goto fail;
			}
		}
	}
	else if(method == 1)//299/29A
	{
		if(ecge_mbox_write_cmd(ECGI_MBOX_CMD_C0h) != 0)
		{
			goto fail;
		}
		if(ecge_mbox_read_stat(&tmp) != 0)
		{
			goto fail;
		}
		if(tmp != 1)
		{
			goto fail;
		}
	}
	
end:
	ecgi_pmc.cmd = ECGE_PMC_CMD_PORT;
	ecgi_pmc.data = ECGE_PMC_DAT_PORT;
	return ret;
fail:
	ret = -1;
	goto end;
}

/*==============================================================*/
int ecgi_get_embedic_info(embeded_t *fw_info)
{
	ecgi_pjt_t	pjt;
	uint16_t	kver;
	uint8_t		accesstype = 0;
	pjt.kver[0] = 0;
	pjt.kver[1] = 0;

	if(ite_read_ec_ram(ECGE_ACPI_ADDR_KVER1, &pjt.kver[0]) != 0)
		return -1;

	if(ite_read_ec_ram(ECGE_ACPI_ADDR_KVER2, &pjt.kver[1]) != 0)
		return -1;

	kver = pjt.kver[0] << 8 | pjt.kver[1];

	if (kver >= 0x1105)
	{
		//check access method
		if(ecgi_mbx_check_access_method(0) == 0)	//29E/29F
		{
			accesstype = 1;
		}
		else if(ecgi_mbx_check_access_method(1) == 0)	//299/29A
		{
			accesstype = 2;
		}
		
		if(accesstype == 1)		//29E/29F
		{
			if(ecgi_mobx_get_embedic_info_method1(&pjt) != 0)
				return -1;
		}
		else if(accesstype == 2)//299/29A
		{
			if (ecgi_mobx_get_embedic_info(&pjt) != 0)
				return -1;
		}
	}
	else
	{
		ecgi_flag = 1;									// unsupport mbox, must be use general pmc access
		if (ecgi_acpi_get_embedic_info(&pjt) != 0)
			return -1;
	}

	*fw_info = ecgi_ite_pjt_decode(&pjt);
	return 0;
}
/*==============================================================*/
int ecgi_get_bin_info(binary_t *bin)
{
	uint8_t		itestr[ECGI_SIGN_LEN];
	uint8_t		itestr_def[ECGI_SIGN_LEN] = ECGI_SIGN_STR;
	ecgi_pjt_t	pjt;
	
	uint32_t i;
	char str[30], buf[30];
	
	// Check ITE String
	fseek(bin->ctrl, ECGI_SIGN_ADDR, SEEK_SET);
	if(fread(itestr, 1, ECGI_SIGN_LEN, bin->ctrl) != ECGI_SIGN_LEN){
		fprintf(stderr, "ERROR: Read binary file fail.\n");
        return -1;
	}
	if(memcmp(itestr_def, itestr, ECGI_SIGN_LEN) != 0){
		fprintf(stderr, "ERROR: Invalid binary file.\n");
        return -1;
	}
	
	// Get ITE EC ROM Header
	rewind(bin->ctrl);
	memset(str, 0, sizeof(str));
	fseek(bin->ctrl, 0, SEEK_SET);
	for(i=1;i<=bin->fsize;i++)
	{
		fread(buf, 1, 1, bin->ctrl);
		if(buf[0] == 'K')
		{
			str[0] = buf[0];
			fread(buf, 1, 30, bin->ctrl);
			if((buf[0] == 'V') && (buf[1] == 'R') && \
				(buf[7] == 'O') && (buf[8] == 'V') && (buf[9] == 'R'))
			{
				//get kernel version
				pjt.kver[0] = buf[3];
				pjt.kver[1] = buf[4];
				
				//get firmware version
				pjt.id = buf[11];	//Project ID
				pjt.ver[0] = buf[12];//Version table
				pjt.ver[1] = buf[13];//Project Major version
				pjt.ver[2] = buf[14];//Project Minor version
				pjt.ver[3] = buf[15];//Project type code
				
				//get project name
				pjt.name[0] = buf[19];
				pjt.name[1] = buf[20];
				pjt.name[2] = buf[21];
				pjt.name[3] = buf[22];
				pjt.name[4] = buf[23];
				pjt.name[5] = buf[24];
				pjt.name[6] = buf[25];
				pjt.name[7] = buf[26];
				pjt.name[8] = '\0';
				
				//get chip id
				pjt.chip[0] = buf[27];
				pjt.chip[1] = buf[28];
				
				break;
			}
			else
			{
				fseek(bin->ctrl, i, SEEK_SET);
			}
		}
	}

	bin->finfo = ecgi_ite_pjt_decode(&pjt);
    rewind(bin->ctrl);                                                  // init file cursor postion
    return 0;
}

/*==============================================================*/
int ecgi_ec_update(binary_t *bin, isp_flag_t upflag)
{
    int         rtn = -1;
	int			flag = 0;		// bit0 = 1: dis_kbc, bit1 = 1: enter ISP mode
	uint8_t		count = 50;
	uint8_t		type;
	unsigned long blk;
    unsigned long blocknum;
    unsigned long jcount;
	unsigned long addrtemp;

#ifdef _DOS_
	flag = ecge_disable_port6064();
	if(flag == -1){
		return -1;
	}
#endif //_DOS_

	if(ecgi_isp_entry() != 0){                                                      // enter ISP mode
        fprintf(stderr, "ERROR: Enter ISP mode fail.\n");
		goto eFlash;
    }
	if(ecgi_isp_read_data(&type) != 0)
	{
		fprintf(stderr, "ERROR: Failed to read flash type.\n");
		goto eFlash;
	}
	flag |= 0x02;
	
	switch(type)
	{
		case ECGI_ISP_SECURITY_FLASH:
		{
			if(ecgi_isp_send_file_and_crypt_data(bin->ctrl, ECGI_ISP_SECURITY_SIZE) != 0)
			{
				ecgi_isp_write_cmd(ECGI_ISP_SECURITY_EXIT_WITHOUT_FLASH);
				fprintf(stderr, "ERROR: Check security fail.\n");
				goto eFlash;
			}
			else
			{
				fprintf(stderr, "Check security Success.\n");
			}
		}
		case ECGI_ISP_STANDARD_FLASH:
		{
			//get spi rom id
			if(ecgi_isp_read_device_id() != 0)
			{
				fprintf(stderr, "ERROR: ISP get ROM ID fail\n");
				goto eFlash;
			}

			//unlock
			if(ecgi_isp_spi_unlock_All() != 0)
			{
				fprintf(stderr, "ERROR: Unlock fail\n");
				goto eFlash;
			}
			
			//erase
			blocknum = (uint16_t)(bin->fsize / ECGI_BlockSize21_4K);
			printf("Application:\n");
			for(blk=0; blk<blocknum; blk++)
			{
				fprintf(stderr,"  Erase ...\t\t\t%.1f%%\r", (float)blk / blocknum * 100);

				if(rom_id[0] == ECGI_EmbedRomID)
				{
					for(jcount=0; jcount<4; jcount++)
					{
						addrtemp = (unsigned long)blk * ECGI_BlockSize21_4K + (unsigned long)jcount * 0x400;
						if(ecgi_isp_spi_sector_erase(addrtemp, ECGI_ISP_SPI_SectorErase1K) != 0)
						{
							fprintf(stderr, "ERROR: Erase sector fail\n");
							goto eFlash;
						}
					}
				}
				else
				{
					addrtemp = (unsigned long)blk * ECGI_BlockSize21_4K;
					if(ecgi_isp_spi_sector_erase(addrtemp, ECGI_ISP_SPI_04KByteBE) != 0)
					{
						fprintf(stderr, "ERROR: Erase sector fail\n");
						goto eFlash;
					}
				}
			}
			fprintf(stderr,"  Erase ...\t\t\tDone.                \n");
			
			//program
			ecgi_filebuffer = (uint8_t *)malloc(ECGI_BlockSize21_4K);
			rewind(bin->ctrl);
			for(blk=0; blk<blocknum; blk++)
			{
				fprintf(stderr,"  Program & Verify...\t\t%.1f%%\r", ((float)blk / blocknum * 100) / 2);
				
				addrtemp = (unsigned long)blk * ECGI_BlockSize21_4K;
				if(ecgi_isp_byte_program_4K_from_file(addrtemp, bin->ctrl) != 0)
				{
					fprintf(stderr, "ERROR: Program fail\n");
					goto eFlash;
				}
			}
			
			//verify
			rewind(bin->ctrl);
			for(blk=0; blk<blocknum; blk++)
			{
				fprintf(stderr,"  Program & Verify...\t\t%.1f%%\r", 50 + (((float)blk / blocknum * 100) / 2));
				
				addrtemp = (unsigned long)blk * ECGI_BlockSize21_4K;
				if(ecgi_isp_spi_verify_4K(addrtemp, bin->ctrl) != 0)
				{
					fprintf(stderr, "ERROR: Verify fail\n");
					goto eFlash;
				}
			}
			
			break;
		}
		default:
		{
			fprintf(stderr, "Cmd 0xDC return unknow value.\n");
			goto eFlash;
		}
	}
	
	sys_delay(100);
	
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
				ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_GO_EC_MAIN);
				OSPowerOff();
				break;
			}
			case 2:		// exit ISP mode and goto ec_main
			{
				if(ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_GO_EC_MAIN) != 0)
				{
					fprintf(stderr, "ERROR: Exit ISP mode fail.\n");
				}
				sys_delay(200);
				break;
			}
			case 3:		//platform reset
			{
				ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_GO_EC_MAIN);
				sys_reset();
				break;
			}
			case 4:		//kbc reset
			{
				ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_GO_EC_MAIN);
				KBCReset();
				break;
			}
			case 5:		//ec reset
			{
				ecgi_isp_wdt_reset();
				break;
			}
		}
	}
#else
	if(flag & 0x02){
		sys_reset_notify();
		if(ite_pmc_write_cmd(&ecgi_pmc, ECGI_ISP_GO_EC_MAIN) != 0){                                          // exit ISP mode and goto ec_main
			fprintf(stderr, "ERROR: Exit ISP mode fail.\n");
		}
		sys_delay(200);
	}

#ifdef _DOS_
	if(flag & 0x01)
		ecgi_enable_port6064();
#endif // _DOS_	
	
	if(flag & 0x02)
		sys_reset();
#endif
                                                                                                       // flash error
	if(ecgi_filebuffer != NULL)
		free(ecgi_filebuffer);
    return rtn;
}
/*==============================================================*/
// Check Model Name
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int ecgi_ec_check_model_name(binary_t *bin, embeded_t *fw_info)
{
	if(memcmp((const char *)bin->finfo.project_name, (const char *)fw_info->project_name, PJT_NAME_SIZE) != 0){
		fprintf(stderr, "ERROR: The firmware image file is broken.\n");
		return -1;
	}
	return 0;
}
/*==============================================================*/
int ecgi_ec_init(void)
{
	if(pmc_open_port(&ecgi_pmc) != 0){
        return -1;
    }
	
	return 0;
}

/*==============================================================*/
void ecgi_ec_exit(void)
{
	pmc_close_port(&ecgi_pmc);
}

