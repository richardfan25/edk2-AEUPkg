#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../platform/adaptate.h"
#include "../../base/pmc.h"
#include "../../base/ite.h"



static pmc_port_t som6867_pmc = {MBX_PORT_PMC_CMD, MBX_PORT_PMC_DATA};


void DisplayUsageOfSOM6867EXFO(void)
{
	fprintf(stderr, "SOM6867 EC update tool for EXFO\n");
	fprintf(stderr, "Version:1.1\n");
	
    fprintf(stderr, "\nUsage: EXFO_EC_UPDATE - [OPTION] - [OPTION2 ...] - [BIN FILE]\n");
    fprintf(stderr, "Example: EXFO_EC_UPDATE [-q] -u EC_FW_V10.bin\n\n");
    fprintf(stderr, "Option:\n");
    fprintf(stderr, "\t-u\tUpdate EC firmware.\n");
    fprintf(stderr, "\t-e\tErase reserve block when update firmware.\n");
    fprintf(stderr, "\t-i\tRead EC information.\n");
    //fprintf(stderr, "\t-f\tSystem power reset.\n");
    fprintf(stderr, "\t-q\tNo prompt.\n");
	fprintf(stderr, "\t-h\tHelp.\n");
	
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-r\t[PARAMETER]\n");
	fprintf(stderr, "\t\tReboot OS after programming.\n");
	fprintf(stderr, "\t\tPARAMETER:\n");
	fprintf(stderr, "\t\t\toff  : Power off (default)\n");
	fprintf(stderr, "\t\t\tnone : Keeping alive\n");
	fprintf(stderr, "\t\t\tfull : Full reset\n");
	fprintf(stderr, "\t\t\tkbc  : KBC reset\n");
	fprintf(stderr, "\t\t\tec   : EC reset\n\n");
}

int mbox_check_busy(void)
{
	uint8_t  dumy 	= 0xFF;
	uint16_t retry	= 0;
	
	while(retry < 20000)
	{
		if(pmc_write_cmd(&som6867_pmc, 0xA0) != 0)
			return -1;
		if(pmc_read_data(&som6867_pmc, &dumy) != 0)
			return -1;
		if(dumy == 0)
			return 0;
		
		sys_delay(1);
		retry++;
	}
	fprintf(stderr, "ERROR: ITE Mailbox timeout.\n");
	return -1;
}

int mbox_write_cmd(uint8_t mb_cmd)
{
	if(mbox_check_busy() != 0)
		return -1;
	
	if(pmc_write_cmd(&som6867_pmc, 0x50) != 0)
		return -1;
	
	if(pmc_write_data(&som6867_pmc, mb_cmd) != 0)
		return -1;
	
	return 0;
}

int mbox_read_data(uint8_t offset, uint8_t *buf, uint8_t len)
{
	uint8_t idx;
	
	if(mbox_check_busy() != 0)
		return -1;
	
	offset = 0xA0 + 0x3 + offset;			// Read mbox + data byte offset + user read offset
	for(idx = 0; idx < len; idx++)
	{
		if(pmc_write_cmd(&som6867_pmc, offset + idx) != 0)
			return -1;
		if(pmc_read_data(&som6867_pmc, buf + idx) != 0)
			return -1;
	}
	return 0;
	
}

int CheckProjectSOM6867(uint8_t vid, uint8_t pid)
{
	char pname[10];
	
	if(vid != 0x49)
	{
		return -1;
	}
	
	if(pid != 0x28)
	{
		return -1;
	}
	
	if(mbox_write_cmd(0xF0) != 0)
	{
		return -1;
	}
	
	// Project name
	if(mbox_read_data(0, (uint8_t*)pname, 8) != 0)
	{
		return -1;
	}
	pname[8] = '\0';
	
	if(strcmp(pname, "SOM6867_"))
	{
		fprintf(stderr, "ERROR: project is %s.\n", pname);
		return -1;
	}
	
	return 0;
}
