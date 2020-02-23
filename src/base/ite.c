#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../platform/adaptate.h"
#include "ite.h"
#include "acpi.h"




static pmc_port_t ite_pmc = {0, 0};



void ite_pnp_unlock(void)
{
	outp(ITE_PNP_INDEX, 0x87);
	outp(ITE_PNP_INDEX, 0x87);
}

void ite_pnp_lock(void)
{
	outp(ITE_PNP_INDEX, 0xAA);
}

void ite_pnp_write(uint8_t idx, uint8_t data)
{
	outp(ITE_PNP_INDEX, idx);
	outp(ITE_PNP_DATA, data);
}

uint8_t ite_pnp_read(uint8_t idx)
{
	outp(ITE_PNP_INDEX, idx);
	return inp(ITE_PNP_DATA);
}

uint8_t WriteITEConfigEC(uint32_t address, uint8_t data)
{
	uint8_t haddr, laddr;
	//uint8_t res = 0;
	
	//set address
	haddr = ((address >> 8) & 0xFF);
	laddr = (address & 0xFF);
	
	ite_pnp_write(ITE_EC_LPC_CMD, ITE_EC_HADDR_CMD);
	ite_pnp_write(ITE_EC_LPC_DATA, haddr);
	
	ite_pnp_write(ITE_EC_LPC_CMD, ITE_EC_LADDR_CMD);
	ite_pnp_write(ITE_EC_LPC_DATA, laddr);
	
	//set data
	ite_pnp_write(ITE_EC_LPC_CMD, ITE_EC_RAMDATA_CMD);
	ite_pnp_write(ITE_EC_LPC_DATA, data);
	
	return 0;
}

int ite_pmc_check_busy(void)
{
	uint8_t det1, det2;
	uint32_t retry = 0;
	
	while(retry < 100)
	{
		retry++;
		
		//check busy 1
		det1 = inp(ite_pmc.cmd);
		if((det1 & (dPMCSTA_IBF_BIT | dPMCSTA_OBF_BIT)))
		{
			continue;
		}
		sys_delay(1);	//1ms
		
		//check busy 2
		det2 = inp(ite_pmc.cmd);
		if((det2 & (dPMCSTA_IBF_BIT | dPMCSTA_OBF_BIT)))
		{
			continue;
		}
		
		//check port not in use
		if(det1 == det2)
		{
			return 0;
		}
		else
		{
			continue;
		}
	}
	
	return -1;
}

int ite_read_ec_ram(uint8_t offset, uint8_t *data)
{
	if(ite_pmc.cmd)
	{
		if(ite_pmc_check_busy() != 0)
		{
			fprintf(stderr, "I/O port(0x%X,0x%X) in use.\n", ite_pmc.cmd, ite_pmc.data);
			return -1;
		}
		if(pmc_write_cmd(&ite_pmc, 0x80) != 0)
		{
			return -1;
		}
		if(pmc_write_data(&ite_pmc, offset) != 0)
		{
			return -1;
		}
		if(pmc_read_data(&ite_pmc, data) != 0)
		{
			return -1;
		}
	}
	else
	{
		if(ReadACPIByte(offset, data) != 0)
		{
			return -1;
		}
	}
	
	return 0;
}

int ite_ec_get_ecram_info(uint8_t *vid, uint8_t *pid, uint8_t *cbid)
{
	ite_pmc.cmd = MBX_PORT_PMC_CMD;
	ite_pmc.data = MBX_PORT_PMC_DATA;
	if(pmc_open_port(&ite_pmc) != 0)
	{
		ite_pmc.cmd = 0;
		ite_pmc.data = 0;
		goto end;
	}
	
	//get vid
	if(ite_read_ec_ram(0xFA, vid) != 0)
	{
		goto end;
	}
	sys_delay(10);	// 10ms
	
	//get pid
	if(ite_read_ec_ram(0xFB, pid) != 0)
	{
		goto end;
	}
	sys_delay(10);	// 10ms
	
	//get cbid
	if(ite_read_ec_ram(0xFC, cbid) != 0)
	{
		goto end;
	}
	sys_delay(10);	// 10ms
	
#ifdef SOM6867_EXFO_VERSION
	if(CheckProjectSOM6867(*vid, *pid) != 0)
	{
		fprintf(stderr, "ERROR: This project is not SOM6867.\n");
		goto end;
	}
#endif
	
	return 0;
	
end:
	return -1;
}

int ite_ec_init(void)
{
	uint16_t chip;
	int ret = -1;
	
	// open PNP port
	sys_open_ioport(ITE_PNP_INDEX);
	sys_open_ioport(ITE_PNP_DATA);
	
	//check cfg port
	if((inp(ITE_PNP_INDEX) == 0xFF) && (inp(ITE_PNP_DATA) == 0xFF))
	{
		goto end;
	}
	
	// enter config
	ite_pnp_unlock();
	
#ifdef SOM6867_EXFO_VERSION
	//force to clear 62/66 burst
	WriteITEConfigEC(0x1500, 0x08);
#endif
	
	// get chip
	chip = (uint16_t)ite_pnp_read(0x20) << 8;
	chip |= (uint16_t)ite_pnp_read(0x21);
	
	// exit config
	ite_pnp_lock();
	
	if((chip == 0x8528) || (chip == 0x8525) || (chip == 0x8518) || (chip == 0x5121))
	{
		ret = 0;
	}
	
end:
	// close PNP port
	sys_close_ioport(ITE_PNP_INDEX);
	sys_close_ioport(ITE_PNP_DATA);
	
	return ret;
}
