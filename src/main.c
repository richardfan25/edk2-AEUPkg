#include <string.h>
#include <ctype.h>

#include "customer/customer.h"
#include "platform/adaptate.h"
#include "aec.h"
#include "is200_test.h"
#include "main.h"

//#define DEBUG
/*==============================================================*/
// VARIABLE
/*==============================================================*/
aeu_app_t   user;
chip_t		chip;

const char   *env           = ENV_STRING;
const char   *build_data    = BUILD_DATE;
const char   *build_ver     = BUILD_VER;

/*==============================================================*/
// check_extension
// Input:       Argc, Argv
// Output:      HANDLE struct
/*==============================================================*/
int check_extension(char *str, char *extension)
{
	int cnt = 0, fst = 0, flen = 0, est = 0, elen = 0, ecnt;
	char word;
	if (str == NULL || extension == NULL)
		return 0;

	while(str[cnt] != '\0') {
		if (str[cnt] == '\\' || str[cnt] == '/') {			// sreach file name start
			fst = cnt + 1;
			flen = 0;
		}
		flen++;
		cnt++;
	}
	if (flen == 0)
		return 0;

	for(cnt = fst; cnt < flen; cnt++) {
		if (isalnum(str[cnt])) continue;
		else if (est != 0) return 0;
		else if (str[cnt] == '.') est = cnt + 1;
		else if (str[cnt] != '_' && str[cnt] != '-' && str[cnt] != '~') return 0;
	}
	elen = flen - est;

	cnt = 0;
	while(isalnum(extension[cnt])) cnt++;
	if (extension[cnt] != '\0')
		return 0;

	if (elen != cnt)return 0;

	for(cnt = est, ecnt = 0; ecnt < elen; cnt++, ecnt++) {
		if ((extension[ecnt] & 0xF0) == 0x40)
			word = extension[ecnt] + 0x20;							// to lowercase
		else
			word = extension[ecnt];
		if ((str[cnt] == word) || (str[cnt] == (word - 0x20))){
			continue;
		}
		else 
			return 0;
	}

	return 1;
}
/*==============================================================*/
// CheckResetTime
// Input:	str
// Output:	time
// Return:	0:fail
//			1:success
/*==============================================================*/
int CheckResetTime(char *str, int *time)
{
	int value = 0;
	
	while(1)
	{
		if(*str == '\0')
		{
			break;
		}
		
		if((*str < '0') || (*str > '9'))
		{
			return 0;
		}
		
		value *= 10;
		value += (int)(*str - '0');
		str++;
	}
	
	if((value < 5) || (value > 300))
	{
		return 0;
	}
	*time = value;
	
	return 1;
}
/*==============================================================*/
// Decode argument
// Input:       Argc, Argv
// Output:      Invalid para
/*==============================================================*/
char *decode_arg(int Argc, char **Argv)
{
    int     Argv_idx = 1;

	if(strcmp(Argv[1], "-su") == 0){
        user.opt.flags.admin = 1;
        Argv_idx++;
    }

	for(;Argv_idx < Argc; Argv_idx++){
		if (Argv[Argv_idx][0] != '-')
			continue;
        if (strcmp(Argv[Argv_idx], "-u") == 0){                                 // [-u] update firmware
			user.opt.flags.update = 1;
        }
        else if (strcmp(Argv[Argv_idx], "-i") == 0){                    		// [-i] display EC info
            user.opt.flags.pjt_info = 1;
        }
        else if (strcmp(Argv[Argv_idx], "-b") == 0){                    		// [-b] display Binary info
            user.opt.flags.bin_info = 1;
        }
        else if(strcmp(Argv[Argv_idx], "-q") == 0){                             // [-q] no prompt
            user.opt.flags.no_ack = 1;
        }
        /*else if(strcmp(Argv[Argv_idx], "-f") == 0){                             // [-n] power off
			user.opt.flags.poweroff = 1;
        }*/
        else if(strcmp(Argv[Argv_idx], "-force") == 0){                         // [-force] do not verify the project name that read from binary file and device.
			user.opt.flags.force = 1;
        }
        else if(strcmp(Argv[Argv_idx], "-e") == 0){                             // [-e] Erase reserve block for fw codebase
			user.opt.flags.fullerase = 1;
        }
		else if(strcmp(Argv[Argv_idx], "-h") == 0){                             // [-h] show help
			user.opt.flags.help = 1;
        }
#if defined(_WINDOW_) || defined(_LINUX_)
		else if(strcmp(Argv[Argv_idx], "-t") == 0){                             // [-t] Forced power down timer
        }
		else if(strcmp(Argv[Argv_idx], "-r") == 0){                             // [-r] Reboot OS after programming
        }
#endif
		else {
			user.opt.flags.err_para = 1;
			return Argv[Argv_idx];
		}
    }
#if defined(_WINDOW_) || defined(_LINUX_)
	for(Argv_idx = 1;Argv_idx < Argc; Argv_idx++)
	{
		if(chip.vid == 'R')
		{
			// forced power down timer
			if(strcmp(Argv[Argv_idx], "-t") == 0)
			{
				if((Argv_idx + 1) == Argc)
				{
					user.opt.flags.err_para = 1;
					return Argv[Argv_idx];
				}
				if(gResetTime == 0)
				{
					if(CheckResetTime(Argv[Argv_idx + 1], &gResetTime) == 0)
					{
						user.opt.flags.err_para = 1;
						return Argv[Argv_idx + 1];
					}
				}
				else
				{
					user.opt.flags.err_para = 1;
					return Argv[Argv_idx];
				}
			}
		}
		
		if(chip.vid == 'I')
		{
			// reboot OS after programming
			if(strcmp(Argv[Argv_idx], "-r") == 0)
			{
				if((Argv_idx + 1) == Argc)
				{
					user.opt.flags.err_para = 1;
					return Argv[Argv_idx];
				}
				if(gResetMode == 0)
				{
					if(strcmp(Argv[Argv_idx + 1], "off") == 0)
					{
						gResetMode = 1;
					}
					/*else if(strcmp(Argv[Argv_idx + 1], "none") == 0)
					{
						gResetMode = 2;
					}*/
					else if(strcmp(Argv[Argv_idx + 1], "full") == 0)
					{
						gResetMode = 3;
					}
					else if(strcmp(Argv[Argv_idx + 1], "kbc") == 0)
					{
						gResetMode = 4;
					}
					else if(strcmp(Argv[Argv_idx + 1], "ec") == 0)
					{
						gResetMode = 5;
					}
					else
					{
						user.opt.flags.err_para = 1;
						return Argv[Argv_idx];
					}
				}
				else
				{
					user.opt.flags.err_para = 1;
					return Argv[Argv_idx];
				}
			}
		}

		// get file path
		if(user.filename == NULL && check_extension(Argv[Argv_idx], "bin"))
		{
			if(user.opt.flags.bin_info || user.opt.flags.update)
			{
				user.filename = Argv[Argv_idx];
				continue;
			}
			else if(Argv_idx == 1 && Argc == 2)
			{
				user.filename = Argv[Argv_idx];
				user.opt.flags.update = 1;
				continue;
			}
		}
		else if(user.filename != NULL && check_extension(Argv[Argv_idx], "bin"))
		{
			user.opt.flags.err_para = 1;
			return Argv[Argv_idx];
		}
	}
#else
	for(Argv_idx = 1;Argv_idx < Argc; Argv_idx++){
		if (Argv[Argv_idx][0] == '-')
			continue;
		
		// get file path
 		if (user.filename == NULL && check_extension(Argv[Argv_idx], "bin")) {
			if (user.opt.flags.bin_info || user.opt.flags.update) {
				user.filename = Argv[Argv_idx];
				continue;
			}
			else if (Argv_idx == 1 && Argc == 2){
				user.filename = Argv[Argv_idx];
				user.opt.flags.update = 1;
				continue;
			}
		}

		user.opt.flags.err_para = 1;
		return Argv[Argv_idx];
    }
#endif
	return NULL;
}
/*==============================================================*/
// Display banner
// Input:       none
// Output:      none
/*==============================================================*/
void display_banner(void)
{
	fprintf(stderr, "+-----------------------------------------------------------------------------+\n");
    fprintf(stderr, "|  AEU %s - Advantech EC utility for %-7s                               |\n", build_ver, env);
	fprintf(stderr, "|                                                                             |\n");
	fprintf(stderr, "|              Copyright(C) 2017-2019 Advantech Co.,Ltd. All Rights Reserved  |\n");
	fprintf(stderr, "|                                             (Build : %s %s)  |\n", __DATE__, __TIME__);
	fprintf(stderr, "+-----------------------------------------------------------------------------+\n");
}

/*==============================================================*/
// Display usage page
// Input:       none
// Output:      none
/*==============================================================*/
void display_usage(void)
{
    fprintf(stderr, "\nUsage: aeu - [OPTION] - [OPTION2 ...] - [BIN FILE]\n");
    fprintf(stderr, "Example: aeu [-q] -u EC_FW_V10.bin\n\n");
    fprintf(stderr, "Option:\n");
    fprintf(stderr, "\t-u\tUpdate EC firmware.\n");
    fprintf(stderr, "\t-e\tErase the reserved block during firmware updating.\n");
    fprintf(stderr, "\t-i\tRead EC information.\n");
    //fprintf(stderr, "\t-f\tSystem power reset.\n");
    fprintf(stderr, "\t-q\tNo prompt.\n");
	fprintf(stderr, "\t-h\tHelp.\n");
	
#if defined(_WINDOW_) || defined(_LINUX_)
	if(chip.vid == 'R')
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "\t-t\t[PARAMETER]\n");
		fprintf(stderr, "\t\tForced shutdown timer.\n");
		fprintf(stderr, "\t\tIt must be longer enough to shutdown OS gracefully.\n");
		fprintf(stderr, "\t\tPARAMETER:\n");
		fprintf(stderr, "\t\t\t5 ~ 300 seconds, default 10 seconds\n");
	}
	
	if(chip.vid == 'I')
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "\t-r\t[PARAMETER]\n");
		fprintf(stderr, "\t\tReboot the OS after firmware updating.\n");
		fprintf(stderr, "\t\tPARAMETER:\n");
		fprintf(stderr, "\t\t\toff  : Power off (default)\n");
		//fprintf(stderr, "\t\t\tnone : Do nothing\n");
		fprintf(stderr, "\t\t\tfull : Full reset\n");
		fprintf(stderr, "\t\t\tkbc  : KBC reset\n");
		fprintf(stderr, "\t\t\tec   : EC reset\n\n");
	}
#endif
}

/*==============================================================*/
// MAIN
/*==============================================================*/
int main( IN int Argc, IN char **Argv)
{
	aec_upflag_t	upflag;
	int		rtn = 0;
	char	input[2];
	char	*para;
	//char 	key;

#ifndef CUSTOMER
	display_banner();
#endif
	
    //---------------------
    // App Initialize
    //---------------------
    if(proc_aec_init(&chip) != 0){
        goto eErr;
    }

    //---------------------
    // Decode argument
    //---------------------
    if(Argc <= 1){
        goto eErrArg;                                                                                   // no argument
    }
    else{
        para = decode_arg(Argc, Argv);
		if(user.opt.flags.help == 1)
		{
			goto eErrArg;
		}
        else if((user.opt.all & 0xFC) == 0){                                                 // skip -su/-q
            if(user.opt.flags.no_ack == 1)
                fprintf(stderr, "ERROR: Too few arguments.\n");
            else
                fprintf(stderr, "ERROR: No valid arguments.\n");
            goto eErrArg;                                                                           // no invalid argument
        }
		else if(user.opt.flags.err_para){
			fprintf(stderr, "ERROR: Get invalid argument(%s).\n", para);
			goto eErr;
		}
        else if(user.filename == NULL && (user.opt.flags.bin_info || user.opt.flags.update)){
            fprintf(stderr, "ERROR: No input file.\n");                              // it's no a name after the -b or -u
            goto eErr;
        }
    }

    //---------------------
    // Print Project info
    //---------------------
    if(user.opt.flags.pjt_info){
        if(proc_aec_print_info() != 0)
			fprintf(stderr, "ERROR: Getting the EC information failed!\n");
    }
	else{
		show_board_info();
	}

    //---------------------
    // Print Project info
    //---------------------
    if(user.opt.flags.bin_info){
        if(proc_aec_print_bin(user.filename) != 0)
			fprintf(stderr, "ERROR: Getting the image file information failed!\n");
    }
	else{
		show_image_info(user.filename);
	}

    //---------------------
    // Update EC firmware
    //---------------------
	upflag.byte = 0;
	if (user.opt.flags.fullerase)
		upflag.bits.fullerase = 1;
	if (user.opt.flags.no_ack)
		upflag.bits.no_ack = 1;
	if (user.opt.flags.force)
	{
		upflag.bits.force = 1;
		fprintf(stderr, "\n+--[ Warning ]--------------------------------------------------+ ");
		fprintf(stderr, "\n|                                                               |");
		fprintf(stderr, "\n| The verfication of firmware update was set to \"Disable\".      |");
		fprintf(stderr, "\n| Device may be broken in worst case.                           |");
		fprintf(stderr, "\n|                                                               |");
		fprintf(stderr, "\n+---------------------------------------------------------------+ ");
		fprintf(stderr, "\nDo you want to continue (y/n)? ");
        if(scanf("%1s",input))                                                                                                                    // fimware update prompt
		{;}
		if ((input[0] != 'y') && (input[0] != 'Y'))
		{
			fprintf(stderr, "Abort the firmware update.\n");
            goto exit;
		}
	}

	if(user.opt.flags.update){
        fprintf(stderr, "\nIt must shutdown the PC after firmware updating successfully.\n");
        if(user.opt.flags.no_ack == 0){
#if defined(_WINDOW_) || defined(_LINUX_)
			if(chip.vid == 'R')
			{
				if(gResetTime)
				{
					fprintf(stderr, "Forced shutdown timer = %d seconds.\n", gResetTime);
				}
				else
				{
					fprintf(stderr, "Forced shutdown timer = 10 seconds.\n");
				}
			}
#endif
            fprintf(stderr, "Do you want to continue(y/n)? ");
        
            if(scanf("%1s",input))                                                                                                  // fimware update prompt
			{;}
			if ((input[0] != 'y') && (input[0] != 'Y'))
			{
				fprintf(stderr, "Abort the firmware update.\n");
                goto exit;
			}
            fprintf(stderr, "\nNOTE:\tPlease do not shutdown PC during firmware updating.\n");
            fprintf(stderr, "Updating the EC firmware......\n");
        }

		// Execute Update
		if(proc_aec_update(user.filename, upflag) != 0){                                                                                                        // isp process start
			fprintf(stderr, "\nUpdating the EC firmware failed.");
			goto exit;
		}

    }
	
    //---------------------
    // Reset System
    //---------------------
   /* if(user.opt.flags.poweroff == 1){
		sys_reset_notify();
		sys_reset();                                                                                                               // auto shutdown system 
	}*/
    
exit:
    proc_aec_exit();

    fprintf(stderr, "\n");
    return rtn;

eErrArg:

#ifdef CUSTOMER
	CUSTOMER_USAGE();
#else
	display_usage();
#endif
    
eErr:
    rtn = ERR_EXIT;
    goto exit;
}

