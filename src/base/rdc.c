#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../platform/adaptate.h"
#include "pmc.h"
#include "flash.h"
#include "rdc.h"

static pmc_port_t rdc_pmc;
static pmc_port_t rdc_ecio;


static rdc_spi_t 	spi_sta;
static uint16_t		rdc_spi_div;

#define mRDC_SPI_CS_EN()		rdc_ecio_outp(RDC_REG_SPICS, 0x00)
#define mRDC_SPI_CS_DIS()		rdc_ecio_outp(RDC_REG_SPICS, 0x01)

/*==============================================================*/
// RDC PNP Function
//
/*==============================================================*/
void rdc_pnp_unlock(void)
{
	outp(RDC_PNP_INDEX, 0x87);
	outp(RDC_PNP_INDEX, 0x87);
}

/*==============================================================*/
void rdc_pnp_lock(void)
{
	outp(RDC_PNP_INDEX, 0xAA);
}

/*==============================================================*/
void rdc_pnp_write(uint8_t idx, uint8_t data)
{
	outp(RDC_PNP_INDEX, idx);
	outp(RDC_PNP_DATA, data);
}

/*==============================================================*/
uint8_t rdc_pnp_read(uint8_t idx)
{
	outp(RDC_PNP_INDEX, idx);
	return inp(RDC_PNP_DATA);
}

/*==============================================================*/
// RDC PMC Function
//
/*==============================================================*/
int rdc_pmc_write_cmd(uint8_t cmd)
{
	if(spi_sta.bits.init == 0)
		return -1;
	
	return pmc_write_cmd(&rdc_pmc, cmd);
}

/*==============================================================*/
int rdc_pmc_write_data(uint8_t data)
{
	if(spi_sta.bits.init == 0)
		return -1;
	
	return pmc_write_data(&rdc_pmc, data);
}

/*==============================================================*/
int rdc_pmc_read_data(uint8_t *data)
{
	if(spi_sta.bits.init == 0)
		return -1;
	
	return pmc_read_data(&rdc_pmc, data);
}

/*==============================================================*/
// RDC ECIO Function
//
/*==============================================================*/
uint8_t rdc_ecio_inp(uint16_t addr)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	return inp(rdc_ecio.data + (addr & 0x0003));
}

/*==============================================================*/
void rdc_ecio_outp(uint16_t addr, uint8_t data)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	outp(rdc_ecio.data + (addr & 0x0003), data);
}

/*==============================================================*/
uint16_t rdc_ecio_inpw(uint16_t addr)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	return inpw(rdc_ecio.data + (addr & 0x0002));
}

/*==============================================================*/
void rdc_ecio_outpw(uint16_t addr, uint16_t data)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	outpw(rdc_ecio.data + (addr & 0x0002), data);
}

/*==============================================================*/
uint32_t rdc_ecio_inpd(uint16_t addr)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	return inpd(rdc_ecio.data);
}

/*==============================================================*/
void rdc_ecio_outpd(uint16_t addr, uint32_t data)
{
	outpw(rdc_ecio.cmd, addr & 0xFFFC);
	outpd(rdc_ecio.data, data);
}
/*==============================================================*/
// RDC SPI Function
//
/*==============================================================*/
static int rdc_spi_wait_tx(void)
{
    uint32_t retry = 0;

	// RDC SPI STATUS bit4: Output complete/FIFO empty when set.
    while(!(rdc_ecio_inp(RDC_REG_SPIST) & (1 << 4)))
    {
        // While Output FIFO empty
        if(++retry > RDC_SPI_TX_TIMEOUT)
        {
        	fprintf(stderr, "ERROR: FSPI FIFO timeout! (0x%02X)\r\n", rdc_ecio_inp(RDC_REG_SPIST));
            return -1;
        }
    }
    return 0;
}

/*==============================================================*/
uint32_t rdc_spi_data_exchange(uint8_t* dest, uint8_t* src , uint32_t len)
{
    uint32_t i, remain, widx, pos = 0;

    if (dest != NULL && src != NULL){
        if (rdc_spi_wait_tx() != 0)
			return pos;

        while (pos < len){
			remain = len - pos;
			widx = (remain < RDC_SPI_FIFO_SIZE) ? remain : RDC_SPI_FIFO_SIZE;

			// Write Data to output FIFO
			for (i = 0 ; i < widx ; i++){
				rdc_ecio_outp(RDC_REG_SPIDO, src[pos + i]);
			}

			// Wait TRX Finish
			if (rdc_spi_wait_tx() != 0)
			    break;

			// Read Data from input FIFO
			for (i = 0 ; i < widx ; i++){
				dest[pos + i] = rdc_ecio_inp(RDC_REG_SPIDI);
			}
			pos += widx;
        }
    }

	return pos;
}

/*==============================================================*/
uint32_t rdc_spi_read_data(uint8_t* data, uint32_t len)
{
	uint32_t rtn = 0;
	uint8_t *wbuf = (uint8_t*)malloc(len);

	if (wbuf)
	{
		rtn = rdc_spi_data_exchange(data, wbuf, len);
		free(wbuf);
	}
	return rtn;
}

/*==============================================================*/
uint8_t rdc_spi_write_data(uint8_t* data, uint32_t len)
{
	uint32_t rtn = 0;
	uint8_t *rbuf = (uint8_t*)malloc(len);

	if (rbuf)
	{
		rtn = rdc_spi_data_exchange(rbuf, data, len);
		free(rbuf);
	}
	return (uint8_t)rtn;
}


/*==============================================================*/
// RDC FLASH Function
//
/*==============================================================*/
void rdc_fla_enable_write(void)
{
	uint8_t data = FLASH_CMD_WREN;
	
	mRDC_SPI_CS_EN();
	// Send Write Enable Command to Flash
	rdc_spi_write_data(&data, 1);
	mRDC_SPI_CS_DIS();
}

/*==============================================================*/
void rdc_fla_disable_write(void)
{
	uint8_t data = FLASH_CMD_WRDI;
	
	mRDC_SPI_CS_EN();
	// Send Write Enable Command to Flash
	rdc_spi_write_data(&data, 1);
	mRDC_SPI_CS_DIS();
}

/*==============================================================*/
uint8_t rdc_fla_read_status(void)
{
	uint8_t	cmd;
	uint8_t	sta;
	
	cmd = FLASH_CMD_RDSR;
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(&cmd, 1);
	rdc_spi_read_data(&sta, 1);
	
	mRDC_SPI_CS_DIS();
	
	return sta;
}

/*==============================================================*/
int rdc_fla_write_status(uint8_t data)
{
	uint8_t		wbuf[4];
	uint16_t	timeout = 0;

	rdc_fla_enable_write();

	wbuf[0] = FLASH_CMD_WRSR;
	wbuf[1] = data;
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(wbuf, 2);
	
	mRDC_SPI_CS_DIS();

	rdc_fla_disable_write();
	
	while(rdc_fla_read_status() & 0x01)
	{
		if (timeout++ >= 2000)
			return -1;
		sys_delay(1);
	};
	
	return 0;
}

/*==============================================================*/
int rdc_fla_read_id(uint8_t *id)
{
	uint8_t	cmd;
	
	if(id == NULL)
		return -1;
	
	cmd = FLASH_CMD_RDID;
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(&cmd, 1);
	rdc_spi_read_data(id, 3);
	
	mRDC_SPI_CS_DIS();
	
	return 0;
}

/*==============================================================*/
int rdc_fla_read_mid(uint8_t *mid)
{
	uint8_t	wbuf[4];
	
	if(mid == NULL)
		return -1;
	
	wbuf[0] = FLASH_CMD_REMS;
	wbuf[1] = 0;
	wbuf[2] = 0;
	wbuf[3] = 0; // MID offset
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(wbuf, 4);
	rdc_spi_read_data(mid, 1);
	
	mRDC_SPI_CS_DIS();
	
	return 0;
}

/*==============================================================*/
int rdc_fla_read_did(uint8_t *did)
{
	uint8_t	wbuf[4];
	
	if(did == NULL)
		return -1;
	
	wbuf[0] = FLASH_CMD_REMS;
	wbuf[1] = 0;
	wbuf[2] = 0;
	wbuf[3] = 1; // DID offset
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(wbuf, 4);
	rdc_spi_read_data(did, 1);
	
	mRDC_SPI_CS_DIS();
	
	return 0;
}

/*==============================================================*/
uint32_t rdc_fla_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
	uint32_t rtn = 0;
	uint8_t	wbuf[4];
	
	if((buf != NULL) && (len > 0)){
		wbuf[0] = FLASH_CMD_READ;
		wbuf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
		wbuf[2] = (addr & 0x0000FF00) >> 8;
		wbuf[3] = (addr & 0x000000FF);
		
		mRDC_SPI_CS_EN();
		
		rdc_spi_write_data(wbuf, 4);
		rtn = rdc_spi_read_data(buf, len);
		
		mRDC_SPI_CS_DIS();
	}
	
	return rtn;
}

/*==============================================================*/
void rdc_fla_erase_sector(uint32_t addr)
{
	uint8_t wbuf[4];
	
	rdc_fla_enable_write();
	
	wbuf[0] = FLASH_CMD_SE;
	wbuf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
	wbuf[2] = (addr & 0x0000FF00) >> 8;
	wbuf[3] = (addr & 0x000000FF);
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(wbuf, 4);
	
	mRDC_SPI_CS_DIS();
	
	// Wait Busy
	while(rdc_fla_read_status() & FLASH_RDSR_BIT_BUSY);
	
	rdc_fla_disable_write();
}

/*==============================================================*/
void rdc_fla_erase_block(uint32_t addr)
{
	uint8_t wbuf[4];
	
	rdc_fla_enable_write();
	
	wbuf[0] = FLASH_CMD_BE;
	wbuf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
	wbuf[2] = (addr & 0x0000FF00) >> 8;
	wbuf[3] = (addr & 0x000000FF);
	
	mRDC_SPI_CS_EN();
	
	rdc_spi_write_data(wbuf, 4);
	
	mRDC_SPI_CS_DIS();
	
	// Wait Busy
	while(rdc_fla_read_status() & FLASH_RDSR_BIT_BUSY);
	
	rdc_fla_disable_write();
}

/*==============================================================*/
uint32_t rdc_fla_program_page(uint32_t addr, uint8_t *buf, uint32_t len)
{
	uint8_t wbuf[4];
	uint32_t page_remain, part, i = 0;
	
	if((buf != NULL) && (len > 0)){
		while (i < len){
			while(rdc_fla_read_status() & FLASH_RDSR_BIT_BUSY);
			rdc_fla_enable_write();

			// Calculate Current Page Remain Size
			page_remain = 0x100 - (addr & 0xff);
			// Calculate Remain Data to Write
			part = len - i;
			// Calculate How many data write to current page
			if(part > page_remain)
				part = page_remain;
			
			// Send Page Program ommand
			wbuf[0] = FLASH_CMD_PP;
			wbuf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
			wbuf[2] = (addr & 0x0000FF00) >> 8;
			wbuf[3] = (addr & 0x000000FF);
			mRDC_SPI_CS_EN();
			
			rdc_spi_write_data(wbuf, 4);
			rdc_spi_write_data(buf, len);
			
			mRDC_SPI_CS_DIS();
			
			i += part;
			addr += part;
		}
		while(rdc_fla_read_status() & FLASH_RDSR_BIT_BUSY);
		rdc_fla_disable_write();
	}

	return i;
}

/*==============================================================*/
int rdc_ec_init(void)
{
	uint16_t	chip;
	
	// Open PNP port
	sys_open_ioport(RDC_PNP_INDEX);
	sys_open_ioport(RDC_PNP_DATA);
	rdc_pnp_unlock();
	
	// Check Chip
	chip = (uint16_t)rdc_pnp_read(0x20) << 8;
	chip |= (uint16_t)rdc_pnp_read(0x21);
	
	if(chip == RDC_CHIP_A9610_ID || chip == RDC_CHIP_A9620_ID){
		// Enable SIO device
		rdc_pnp_write(0x23, rdc_pnp_read(0x23) | 0x01);
	}
	else{
		fprintf(stderr, "ERROR: error chip id\n");
		return -1;
	}
	// PMC1
	rdc_pnp_write(0x07, RDC_LDN_PMC1);
	rdc_pnp_write(0x30, 0x01);				// enable PMC1
	rdc_pmc.data = (uint16_t)rdc_pnp_read(0x60) << 8;
	rdc_pmc.data |= (uint16_t)rdc_pnp_read(0x61);
	rdc_pmc.cmd = (uint16_t)rdc_pnp_read(0x62) << 8;
	rdc_pmc.cmd |= (uint16_t)rdc_pnp_read(0x63);
	if(pmc_open_port(&rdc_pmc) != 0){
		fprintf(stderr, "ERROR: Failed to open pmc1\n");
        return -1;
    }
	// ECIO
	rdc_pnp_write(0x07, RDC_LDN_ECIO);
	rdc_pnp_write(0x30, 0x01);				// enable ECIO
	rdc_ecio.cmd = (uint16_t)rdc_pnp_read(0x60) << 8;
	rdc_ecio.cmd |= (uint16_t)rdc_pnp_read(0x61);
	rdc_ecio.data = rdc_ecio.cmd + 4;
	for(chip = 0; chip < 8; chip++){
		sys_open_ioport(rdc_ecio.cmd + chip);
	}
	
	rdc_pnp_lock();
	sys_close_ioport(RDC_PNP_INDEX);
	sys_close_ioport(RDC_PNP_DATA);
	spi_sta.bits.init = 1;
	return 0;
}
/*==============================================================*/
void rdc_wdt_en(uint32_t ms, uint8_t event)
{
	// Disable Isolation cell
	//ecio_outd(0xFF40, ecio_ind(0xFF40) & ~(1L << 17)); 
	// Timeout: 1ms
	rdc_ecio_outp(0xF502, (uint8_t) ms);		
	rdc_ecio_outp(0xF503, (uint8_t)(ms >> 8));
	rdc_ecio_outp(0xF504, (uint8_t)(ms >> 16));
	// Reset event when timeout
	if(event != 0x10)
		event = 0x20;
	rdc_ecio_outp(0xF501, event);

	// start WDT
	rdc_ecio_outp(0xF500, rdc_ecio_inp(0xF500) | 0x40);
}
/*==============================================================*/
int rdc_spi_init(void)
{
	uint8_t temp;

	if(spi_sta.bits.spi_en){
		fprintf(stderr, "ERROR: SPI already enabled.\n");
		return -1;
	}
	
	spi_sta.bits.spi_en = 1;
	
	// Disable Auto Fetch
	rdc_ecio_outp(RDC_REG_SPIAF, 0x00);
	// Backup original setting
	rdc_spi_div = rdc_ecio_inpw(RDC_REG_SPIDIV);
	// Clear error bits
	rdc_ecio_outp(RDC_REG_SPIEST, 0xFF);
	// Disable Chip Select pin
	mRDC_SPI_CS_DIS();
	// Set clock divisor
	rdc_ecio_outp(RDC_REG_SPIDIV, 2);

	// Check Flash Write Protect
	temp = rdc_fla_read_status();
	if ((temp & 0x80) && ((temp & 0x1C) != 0))
	{
		// Try to disable Software Flash Protection
		fprintf(stderr, "Disable SRWD(0x%02X)\n", temp);
		if (rdc_fla_write_status(0x00) != 0)
			goto wp_err;
		temp = rdc_fla_read_status();
		if ((temp & 0x80) && ((temp & 0x1C) != 0))
			goto wp_err;
	}

	return 0;
wp_err:
	fprintf(stderr, "ERROR: Flash Write Protect Enable.\n");
	return -1;
}

/*==============================================================*/
void rdc_spi_uninit(void)
{
	if(spi_sta.bits.spi_en){
		rdc_ecio_outpw(RDC_REG_SPIDIV, rdc_spi_div);
		// Enable Auto Fetch
		rdc_ecio_outp(RDC_REG_SPIAF, 0x01);
		
		spi_sta.bits.spi_en = 0;
	}
}

///////////////////////////////////////////////////////////////////////////
int rdc_ft_pmc_status(uint8_t *buf)
{
	//clean data buf
	memset(buf, 0, 1);
	
	if(rdc_pmc_write_cmd(0x01) != 0)
	{
		return -1;
	}
	
	if(rdc_pmc_read_data(buf) != 0)
	{
		return -1;
	}
	
	return 0;
}

int rdc_ft_pmc_protocl(uint8_t cmd, uint8_t ctrl, uint8_t did, uint8_t len, uint8_t *buf)
{
	uint8_t idx;
	
	//clean data buf
	memset(buf, 0, len);
	
	if(rdc_pmc_write_cmd(cmd) != 0){
		return -1;
	}
	
	if(rdc_pmc_write_data(ctrl) != 0){
		return -1;
	}
	
	if((cmd & 0xFE) != 0x30)
	{
		if(rdc_pmc_write_data(did) != 0){
			return -1;
		}
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

int rdc_st_pmc_read(uint8_t cmd, uint8_t offset, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	
	//clean data buf
	memset(buf, 0, len);
	
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

int rdc_ec_get_ecram_info(uint8_t *vid, uint8_t *pid, uint8_t *cbid)
{
	uint8_t buf;
	int ret = -1;;
	
	//check firmware team protocol
	if(rdc_ft_pmc_status(&buf) == 0)
	{
		if(buf == 0)
		{
			//get vid
			if(rdc_ft_pmc_protocl(0x31, 0xFA, 0, 1, &buf) != 0)
			{
				goto end;
			}
			*vid = buf;
			
			//get pid
			if(rdc_ft_pmc_protocl(0x31, 0xFB, 0, 1, &buf) != 0)
			{
				goto end;
			}
			*pid = buf;
			
			//get cbid
			if(rdc_ft_pmc_protocl(0x31, 0xFC, 0, 1, &buf) != 0)
			{
				goto end;
			}
			*cbid = buf;
		}
	}
	else
	{
		//get vid
		if(rdc_st_pmc_read(0x31, 0xFA, 1, &buf) != 0)
		{
			goto end;
		}
		*vid = buf;
		
		//get pid
		if(rdc_st_pmc_read(0x31, 0xFB, 1, &buf) != 0)
		{
			goto end;
		}
		*pid = buf;
		
		//get cbid
		if(rdc_st_pmc_read(0x31, 0xFC, 1, &buf) != 0)
		{
			goto end;
		}
		*cbid = buf;
	}
	
	ret = 0;
	
end:
	return ret;
}

int rdc_flash_info(void)
{
	uint8_t		rdid[4];
	uint32_t	fid;		// flash id

	// Flash RDID
	if (rdc_fla_read_id(rdid) != 0)
		return -1;

	fprintf(stderr, "\nFlash RDID  : %02X %02X %02X %02X\n", rdid[0], rdid[1], rdid[2], rdid[3]);

	fid = *(uint32_t *)rdid;
	fid &= 0x00FFFFFF;

	// C2 : manufacturer id
	
	// 28 : memory type
	// 23 : memory type
	
	// 14 : memory density

	if (fid == 0x001428C2)
	{
		fprintf(stderr, "Flash Model : MX25R8035F (v1)\n");
		return 0;
	}
	else if (fid == 0x001423C2)
	{
		fprintf(stderr, "Flash Model : MX25V8035F (v2)\n");
		return 0;
	}
	else
	{
		fprintf(stderr, "Flash Model : unknown\n");
	}
	return -1;
}
