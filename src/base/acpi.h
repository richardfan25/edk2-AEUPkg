#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>

#define ACPI_CMD_PORT		0x66
#define ACPI_DATA_PORT		0x62

// ACPI RAM offset define
#define ADV_ACPI_RAM_IC_VID	0xFA
#define ADV_ACPI_RAM_IC_PID	0xFB
#define ADV_ACPI_RAM_FWCB	0xFC


/*==============================================================*/
int acpi_init_port(void);
int acpi_close_port(void);
int acpi_read_byte(uint8_t offset, uint8_t *data);
int acpi_write_byte(uint8_t offset, uint8_t data);

#endif //__ACPI_H__

