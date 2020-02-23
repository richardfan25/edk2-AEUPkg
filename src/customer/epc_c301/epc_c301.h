#ifndef _EXCEPTION_H__
#define _EXCEPTION_H__


#include <stdint.h>



#define CUSTOMER_USAGE() DisplayUsageOfEPCC301()



void DisplayUsageOfEPCC301(void);
int CheckProjectEPCC301BoardName(char *name);
int CheckProjectEPCC301BinName(char *name);



#endif
