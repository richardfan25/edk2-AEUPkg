#ifndef _ITE_H__
#define _ITE_H__

#include "pmc.h"
#include "../customer/customer.h"



/*==============================================================*/
// IO space
/*==============================================================*/
#define ITE_PNP_INDEX			0x29C
#define ITE_PNP_DATA			0x29D
#define MBX_PORT_PMC_CMD		0x29A
#define MBX_PORT_PMC_DATA		0x299

//ITE EC command
#define ITE_EC_LPC_CMD		0x2e
#define ITE_EC_LPC_DATA		0x2f

#define ITE_EC_HADDR_CMD	0x11
#define ITE_EC_LADDR_CMD	0x10
#define ITE_EC_RAMDATA_CMD	0x12



int ite_ec_init(void);
int ite_read_ec_ram(uint8_t offset, uint8_t *data);
int ite_ec_get_ecram_info(uint8_t *vid, uint8_t *pid, uint8_t *cbid);



#endif
