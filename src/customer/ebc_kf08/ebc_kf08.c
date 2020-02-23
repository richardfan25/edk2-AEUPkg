#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void DisplayUsageOfEBCKF08(void)
{
	fprintf(stderr, "EBC-KF08 EC update tool\n");
	fprintf(stderr, "Version:1.1\n");
	
    fprintf(stderr, "\nUsage: EBC_KF08 - [OPTION] - [OPTION2 ...] - [BIN FILE]\n");
    fprintf(stderr, "Example: EBC_KF08 [-q] -u EC_FW_V10.bin\n\n");
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

int CheckProjectEBCKF08BoardName(char *name)
{
	if (strcmp(name, "SOM-5899") == 0)
		return 0;
	else if (strcmp(name, "EBC-KF08") == 0)
		return 0;

	return -1;
}

int CheckProjectEBCKF08BinName(char *name)
{
	if (strcmp(name, "SOM-5899") == 0)
		return 0;
	else if (strcmp(name, "EBC-KF08") == 0)
		return 0;

	return -1;
}
