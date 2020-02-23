#include <stdio.h>
#include <stdint.h>

#include "../platform/adaptate.h"
#include "pmc.h"
#include "acpi.h"

//==========================================================================
static pmc_port_t acpiport = { ACPI_CMD_PORT, ACPI_DATA_PORT};

//==========================================================================
/*==============================================================*/
int acpi_init_port(void)
{
	if(pmc_open_port(&acpiport) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int acpi_close_port(void)
{
	if(pmc_close_port(&acpiport) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int acpi_read_byte(uint8_t offset, uint8_t *data)
{
	if(pmc_write_cmd(&acpiport, 0x80) != 0)
		return -1;
	if(pmc_write_data(&acpiport, offset) != 0)
		return -1;
	if(pmc_read_data(&acpiport, data) != 0)
		return -1;
	
	return 0;
}

/*==============================================================*/
int acpi_write_byte(uint8_t offset, uint8_t data)
{
	if(pmc_write_cmd(&acpiport, 0x81) != 0)
		return -1;
	if(pmc_write_data(&acpiport, offset) != 0)
		return -1;
	if(pmc_write_data(&acpiport, data) != 0)
		return -1;
	
	return 0;
}
/*==============================================================*/

