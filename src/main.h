#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdint.h>

#include "base/pmc.h"
#include "base/isp.h"
/*==============================================================*/
// CONFIG
/*==============================================================*/
#define	PJT_NAME			"DONTCARE"
#define BUILD_VER			"V1.20"
#define BUILD_DATE			"2020/02/06"


/*==============================================================*/
// DEFINE
/*==============================================================*/

/*==============================================================*/
// STRUCTION
/*==============================================================*/
typedef union _app_option_t{
    uint32_t     all;
    struct{
		uint32_t 	admin       : 1;
		uint32_t 	no_ack  	: 1;
		uint32_t 	update  	: 1;
		uint32_t 	pjt_info    : 1;
		uint32_t 	bin_info    : 1;
		uint32_t 	poweroff    : 1;
		uint32_t 	fullerase   : 1;
		uint32_t 	force       : 1;
		uint32_t 	err_para    : 1;
		uint32_t 	help		: 1;
		uint32_t 	reserved    : 22;
    }flags;
} app_option_t;

/*==============================================================*/
typedef struct _aeu_app_t{
    app_option_t	opt;
    char 			*filename;
} aeu_app_t;

/*==============================================================*/
// FUNCTIONS
/*==============================================================*/
void display_banner(void);

#endif	// _MAIN_H_
