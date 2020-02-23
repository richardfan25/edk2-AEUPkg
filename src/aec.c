#include <string.h>
#include <stdlib.h>

#include "platform/adaptate.h"
#include "base/pmc.h"
#include "base/acpi.h"
#include "base/util.h"
#include "base/rdc.h"
#include "base/ite.h"

#include "ecge.h"
#include "ecgi.h"
#include "ecgf.h"
#include "ecgs.h"
#include "ecgx.h"
#include "ecgz.h"
#include "ecg4.h"
#include "aec.h"

#include "customer/customer.h"

// code		   update     EC_RAM
// base   id   rule		FAh FBh FCh    chip
// ----------------------------------------------------
// ecge   e    e -> e 	49h 28h --	   e=ITE IT8528
//             e -> e   49h 18h --     e=ITE IT8518
// ecgi   i    i -> i   49h 21h --     i=ITE IT5121
// ecgs   s    s -> s	52h 10h 00h    s=RDC A9610-SW
// ecgf   f    f -> f   52h 10h 80h    f=RDC A9610-FW
// ecgx   x    s -> f   52h 10h 5Fh    x=s->f
// ecgz   z    f -> s   52h 10h 81h    z=f->s
// ecg4   4    4 -> 4   n/a n/a n/a    4=RDC A9620 (499h/49ah)
// ----------------------------------------------------

// increase code base number and add handle to below if you want to add new code base
#define CBID_NUM		7
#define CB_HANDLE		{	&ecge_cb,\
							&ecgi_cb,\
							&ecgs_cb,\
							&ecgf_cb,\
							&ecgx_cb,\
							&ecgz_cb,\
							&ecg4_cb,\
						}
						
static const codebase_t *ec_handle = NULL;
static embeded_t	ecinfo;
static binary_t		bininfo;

/*==============================================================*/
// Get EC function table
// Input:   none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
static int _get_ec_func(chip_t *chip_id)
{
	uint8_t 	vid = 0, pid = 0, cbid = 0;
	const chip_t 		*chip = NULL;
	const codebase_t	*ectemp[CBID_NUM] = CB_HANDLE;
	
#ifdef MIOE260_EC_VERSION
	// do nothing
#else
	uint8_t		i, j;
#endif

//---------------------------------------------------------------
//  Windows, Linux
//---------------------------------------------------------------
#if defined(_WINDOW_) || defined(_LINUX_)

#ifdef MIOE260_EC_VERSION
	if(rdc4_ec_init() == 0)	//rdc
	{
		if(rdc_ec_get_ecram_info(&vid, &pid, &cbid) != 0)
		{
			goto eErrInit;
		}
	}
	else
	{
		goto eErrInit;
	}
#else
	if(ite_ec_init() == 0)	//ite
	{
		if(ite_ec_get_ecram_info(&vid, &pid, &cbid) != 0)
		{
			goto eErrInit;
		}
	}
	else if(rdc_ec_init() == 0)	//rdc
	{
		if(rdc_ec_get_ecram_info(&vid, &pid, &cbid) != 0)
		{
			goto eErrInit;
		}
	}
	else
	{
		goto eErrInit;
	}
#endif

//---------------------------------------------------------------
//  UEFI, DOS
//---------------------------------------------------------------
#else

#ifdef MIOE260_EC_VERSION
	// do nothing
#else
	if(acpi_init_port() != 0)
		goto eErrInit;
	
	if(ReadACPIByte(ADV_ACPI_RAM_IC_VID, &vid) != 0)
		goto eErrInit;
	
	if(ReadACPIByte(ADV_ACPI_RAM_IC_PID, &pid) != 0)
		goto eErrInit;
	
	if(ReadACPIByte(ADV_ACPI_RAM_FWCB, &cbid) != 0)
		goto eErrInit;
#endif

#endif
	

	//---------------------------------------------------------------
	//  Code Base checking
	//---------------------------------------------------------------

#ifdef MIOE260_EC_VERSION

	chip = ectemp[6]->chip_list;
	ec_handle = ectemp[6];	// force to ecg4

#else

	for(i = 0; i < CBID_NUM-1; i++){
		for(j = 0; j < ectemp[i]->chip_num; j++){
			chip = ectemp[i]->chip_list + j;
			if(chip->vid != vid)
				continue;
			if(chip->pid != pid)
				continue;
			if((chip->vid != ITE_VID) && (ectemp[i]->cbid != cbid))
				continue;
			ec_handle = ectemp[i];
			break;
		}
		if(ec_handle != NULL)
			break;
	}

#endif

	if(ec_handle == NULL)
		goto eErric;

	chip_id->vid = chip->vid;
	chip_id->pid = chip->pid;
    return 0;
eErrInit:
	fprintf(stderr, "ERROR: Accessing the EC is inhibited.\n");
	return -1;
eErric:
    fprintf(stderr, "ERROR: Unknown chip (%02X-%02X-%02X)\n", vid, pid, cbid);
    return -1;
}
/*==============================================================*/
// Initialize the advantech EC
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_init(chip_t *chip)
{
	// Check OS permission
	sys_check_permission();
	
	// Get EC firmware vendor
	if(_get_ec_func(chip) != 0)
		return -1;
	
	// Initialize EC
	if(ec_handle->func.init() != 0)
		return -1;

	return 0;
}

/*==============================================================*/
// Initialize the advantech EC
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_exit(void)
{
	// Check firmware vendor
	if(ec_handle == NULL)
		return -1;
	ec_handle->func.exit();
	
	// DLL exit
	sys_dll_exit();
	
	return 0;
}


/*==============================================================*/
// Get EC information
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_get_info(aec_ec_t *project)
{
	// Check firmware vendor
	if(ec_handle == NULL)
		return -1;
	
	// Read project information
	if(ec_handle->func.get_embedic_info(&ecinfo) != 0)
		return -1;
	
	strcpy(project->manufacturer,	"Advantech");
	strcpy(project->module, 		ecinfo.project_name);
	strcpy(project->version, 		ecinfo.ver);
	strcpy(project->chip, 			ecinfo.chip);

	return 0;
}
/*==============================================================*/
// Print EC information
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_print_info(void)
{
	aec_ec_t aec_temp;
	
	memset(&aec_temp, 0, sizeof(aec_temp));
	
	if(proc_aec_get_info(&aec_temp) != 0)
		return -1;
	
	fprintf(stderr, " Manufacturer : %s\n", aec_temp.manufacturer);
	fprintf(stderr, "  Module Name : %s\n", aec_temp.module);
	fprintf(stderr, "      Version : %s\n", aec_temp.version);
	fprintf(stderr, "Embedded Chip : %s\n", aec_temp.chip);

	return 0;
}
/*==============================================================*/
// Show board info
/*==============================================================*/
int show_board_info(void)
{
	aec_ec_t aec_temp;
	
	memset(&aec_temp, 0, sizeof(aec_temp));
	
	if(proc_aec_get_info(&aec_temp) != 0)
		return -1;

	fprintf(stderr, "Board Info. : %s, %s, %s, %s\n",
			aec_temp.module,
			aec_temp.version,
			aec_temp.chip,
			aec_temp.manufacturer);

	return 0;
}

/*==============================================================*/
// Print EC information
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_print_bin(char *binfile)
{
	aec_ec_t aec_temp;
	
	memset(&aec_temp, 0, sizeof(aec_temp));
	
	if(proc_aec_get_info(&aec_temp) != 0)
		return -1;
	
	// Check firmware vendor
	if(ec_handle == NULL)
		return -1;
	
	if((bininfo.ctrl = fopen(binfile, "rb")) == NULL)
		return -1;
	
	// Get File Size
	fseek(bininfo.ctrl, 0, SEEK_END); 
	bininfo.fsize = ftell(bininfo.ctrl);
	if ((bininfo.fsize == -1L) || (bininfo.fsize == 0))
		return -1;
	
	// Get/check Binary Information
    if(ec_handle->func.get_binary_info(&bininfo) != 0)
		return -1;
	
	fprintf(stderr, " Manufacturer : %s\n", bininfo.finfo.mfu);
	fprintf(stderr, "  Module Name : %s\n", bininfo.finfo.project_name);
	fprintf(stderr, "      Version : %s\n", bininfo.finfo.ver);
	fprintf(stderr, "Embedded Chip : %s\n", bininfo.finfo.chip);

	return 0;
}

/*==============================================================*/
// Show image info
/*==============================================================*/
int show_image_info(char *binfile)
{
	aec_ec_t aec_temp;
	
	memset(&aec_temp, 0, sizeof(aec_temp));
	
	if(proc_aec_get_info(&aec_temp) != 0)
		return -1;
	
	// Check firmware vendor
	if(ec_handle == NULL)
		return -1;
	
	if((bininfo.ctrl = fopen(binfile, "rb")) == NULL)
		return -1;
	
	// Get File Size
	fseek(bininfo.ctrl, 0, SEEK_END); 
	bininfo.fsize = ftell(bininfo.ctrl);
	if ((bininfo.fsize == -1L) || (bininfo.fsize == 0))
		return -1;
	
	// Get/check Binary Information
    if(ec_handle->func.get_binary_info(&bininfo) != 0)
		return -1;
	
	fprintf(stderr, "Image Info. : %s, %s, %s, %s\n",
			bininfo.finfo.project_name,
			bininfo.finfo.ver,
			bininfo.finfo.chip,
			bininfo.finfo.mfu);

	return 0;
}

/*==============================================================*/
// Update EC Firmware
// Input:       none
// Output:  0: success
//		   -1: fail
/*==============================================================*/
int proc_aec_update(char *binfile, aec_upflag_t upflag)
{
	isp_flag_t	isp_f;
	uint16_t	tempkver;

	isp_f.all = 0;
	if (upflag.bits.fullerase)
		isp_f.bits.fullerase = 1;
	if (upflag.bits.no_ack)
		isp_f.bits.no_ack = 1;

	// Check firmware vendor
	if(ec_handle == NULL)
		return -1;

	// Get EC information
	if(ec_handle->func.get_embedic_info(&ecinfo) != 0)
		return -1;

	if((bininfo.ctrl = fopen(binfile, "rb")) == NULL)
	{
		fprintf(stderr, "ERROR: Open the file %s failed!\n", binfile);
		return -1;
	}
	
	// Get File Size
	fseek(bininfo.ctrl, 0, SEEK_END); 
	bininfo.fsize = ftell(bininfo.ctrl);
	if ((bininfo.fsize == -1L) || (bininfo.fsize == 0)){
		fprintf(stderr, "ERROR: The file %s is broken data integrity!\n", binfile);
		goto end;
	}

	// Get/check Binary Information
    if(ec_handle->func.get_binary_info(&bininfo) != 0){
        goto end;
    }

	// Check Binary/Target compatible
	if (upflag.bits.force == 0)
	{
		//   In the ECG EC codebse, the firmware that its kernel version smaller than 0x1105 doesn't support mailbox 
		//   and can't get project name
		sscanf(&ecinfo.ver[5], "%x", &tempkver);
		if ((ec_handle == &ecge_cb) && (tempkver < 0x1105))
		{
			if(ecinfo.id != bininfo.finfo.id){
				fprintf(stderr, "ERROR: The firmware image file is broken!\n");
				goto end;
			}
		}
		else
		{
			//---------------------------------------------------------
			//  standard version
			//---------------------------------------------------------
			#ifndef CUSTOMER

			if (ec_handle->func.check_model_name(&bininfo, &ecinfo) != 0)
			{
				fprintf(stderr, "ERROR: The firmware image file is broken.\n");
				goto end;
			}

			/*
			if(memcmp((const char *)bininfo.finfo.project_name, (const char *)ecinfo.project_name, PJT_NAME_SIZE) != 0){
				fprintf(stderr, "ERROR: The firmware image file is broken.\n");
				goto end;
			}
			*/
			//---------------------------------------------------------
			//  customer version
			//---------------------------------------------------------
			#else

				#ifdef EBCKF08_EC_VERSION
					// do not check the same as board name and bin name
				#endif
				
				#ifdef EPCC301_EC_VERSION
					// do not check the same as board name and bin name
				#endif

			#endif
		}
	}

	// Execute Update
    if(ec_handle->func.update_firmware(&bininfo, isp_f) != 0){                                                                                                        // isp process start
		goto end;
	}

	return 0;
end:
	if(bininfo.ctrl != NULL)
		fclose(bininfo.ctrl);
	return -1;
}

