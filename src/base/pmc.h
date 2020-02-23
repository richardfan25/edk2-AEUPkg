#ifndef __PMC_H__
#define __PMC_H__

#include <stdint.h>

#define dPMCSTA_IBF_BIT  0x2                            // cmd/status port bit1 mask
#define dPMCSTA_OBF_BIT  0x1                            // cmd/status port bit0 mask

#define dPMCSTA_CHK_DLY  20000              // uint16 delay time that check io port status(IBF/OBF)
/*==============================================================*/
typedef	struct _pmc_port_t{
	uint16_t	cmd;
	uint16_t	data;
} pmc_port_t;

/*==============================================================*/
// EXTERN fucntion, variable
/*==============================================================*/
extern int pmc_open_port(pmc_port_t *port);
extern int pmc_close_port(pmc_port_t *port);
extern int pmc_check_ibf(pmc_port_t *port);
extern int pmc_check_obf(pmc_port_t *port);
extern int pmc_write_cmd(pmc_port_t *port, uint8_t val);
extern int pmc_write_data(pmc_port_t *port, uint8_t val);
extern int pmc_read_data(pmc_port_t *port, uint8_t *val);
extern int ite_pmc_write_cmd(pmc_port_t *port, uint8_t val);
extern int ite_pmc_write_data(pmc_port_t *port, uint8_t val);
extern int ite_pmc_read_data(pmc_port_t *port, uint8_t *val);

#endif  // __PMC_H__

