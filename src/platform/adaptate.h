#ifndef __ADAPTATE_H__
#define __ADAPTATE_H__

#ifdef _LINUX_

#include <sys/io.h>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <signal.h>

#include "./linux/type.h"
#include "./linux/constant.h"
#include "./linux/io.h"
#include "./linux/misc.h"
 
#else

#ifdef _UEFI_

#include <Include/unistd.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "./efi/type.h"
#include "./efi/constant.h"
#include "./efi/io.h"
#include "./efi/misc.h"

#else

#ifdef _DOS_

#include <conio.h>
#include <i86.h>
#include <graph.h>

#include "./dos/type.h"
#include "./dos/constant.h"
#include "./dos/io.h"
#include "./dos/misc.h"

#else

#ifdef _WINDOW_
#include <stdlib.h>
#include "./windows/type.h"
#include "./windows/constant.h"
#include "./windows/io.h"
#include "./windows/misc.h"

#endif // _WINDOW_

#endif // _DOS_

#endif // _UEFI_

#endif // _LINUX_

#endif // __ADAPTATE_H__
