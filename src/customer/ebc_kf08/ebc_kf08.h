#ifndef _EXCEPTION_H__
#define _EXCEPTION_H__

#include <stdint.h>

#define CUSTOMER_USAGE() DisplayUsageOfEBCKF08()

void DisplayUsageOfEBCKF08(void);
int CheckProjectEBCKF08BoardName(char *name);
int CheckProjectEBCKF08BinName(char *name);

#endif
