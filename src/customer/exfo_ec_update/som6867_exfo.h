#ifndef _EXCEPTION_H__
#define _EXCEPTION_H__


#include <stdint.h>



#define CUSTOMER_USAGE() DisplayUsageOfSOM6867EXFO()



void DisplayUsageOfSOM6867EXFO(void);
int CheckProjectSOM6867(uint8_t vid, uint8_t pid);



#endif
