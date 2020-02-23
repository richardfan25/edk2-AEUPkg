#ifndef __MISC_H__
#define __MISC_H__

#ifdef _WINDOW_


#include "stdafx.h"

#ifdef RUN_TIME_DYNAMIC_LINKING
	#include "OlsApiInitExt.h"
	#include "OlsApiInit.h"
#else // for Load-Time Dynamic Linking
	#include "OlsApi.h"

#ifdef _M_X64
	#pragma comment(lib, "WinRing0x64.lib")
#else
	#pragma comment(lib, "WinRing0.lib")
#endif

#endif

#define sys_check_permission()  	if(windows_check_dll() != 0) {return -1;}

#define sys_open_ioport(addr)

#define sys_close_ioport(addr)

#define sys_clearscreen()       	system("cls")
#define sys_reset()          		windows_coldreset()
#define sys_reset_notify()

#define sys_delay(a)                Sleep(a)		// unit: ms

#define sys_dll_exit()				windows_dll_exit()

#define KBCReset()					WindowsKBCReset()
#define OSPowerOff()				WindowsPowerOff()
#define ReadACPIByte(a, b)			Read6662Port(a, b)
#define CheckKey()					WindowsCheckKey()

extern int windows_check_dll(void);
extern void windows_coldreset(void);
extern void windows_dll_exit(void);
extern int Read6662Port(BYTE offset, BYTE *value);

extern void WindowsKBCReset(void);
extern void WindowsPowerOff(void);
extern int WindowsCheckKey(void);

extern int gResetTime;
extern int gResetMode;

#endif // _WINDOW_

#endif // __MISC_H__

