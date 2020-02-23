#ifndef _AEC_H_
#define _AEC_H_

#include <stdint.h>
#include "base/isp.h"

typedef union _aec_upflag_t {
	struct {
		uint8_t		fullerase	: 1;
		uint8_t		force		: 1;
		uint8_t		no_ack		: 1;
		uint8_t		rsvd		: 5;
	} bits;
	uint8_t			byte;
} aec_upflag_t;

typedef struct _aec_ec_t{
	char 	manufacturer[32];
	char 	module[32];
	char 	version[16];
	char 	chip[16];
} aec_ec_t;

extern int proc_aec_init(chip_t *chip);
extern int proc_aec_exit(void);
extern int proc_aec_update(char *file, aec_upflag_t upflag);
extern int proc_aec_get_info(aec_ec_t *project);
extern int proc_aec_print_info(void);
extern int proc_aec_print_bin(char *binfile);
extern int show_board_info(void);
extern int show_image_info(char *binfile);

#endif // _AEC_H_

