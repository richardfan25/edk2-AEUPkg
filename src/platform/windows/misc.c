#ifdef _WINDOW_

#include <stdio.h>
#include <stdlib.h>
#include <conio.h> 

#include "io.h"
#include "misc.h"
#include "../../base/acpi.h"

HMODULE m_hOpenLibSys;
int gResetTime = 0;
int gResetMode = 0;

//const char * const init_sending_format = "Sending SIG%s to all processes.";
//const char * const shutdown_format = "\r%s\n";
//==========================================================================
int windows_check_dll(void)
{
#ifdef RUN_TIME_DYNAMIC_LINKING
	m_hOpenLibSys = NULL;
	if(!InitOpenLibSys(&m_hOpenLibSys))
	{
		fprintf(stderr, "DLL Load Error!!");
		return -1;
	}
#else
	if(!InitializeOls())
	{
		fprintf(stderr, "ERROR: initial DLL fail. sts=%x \n\n", GetDllStatus());
		return -1;
	}
#endif
	
	return 0;
}

//==========================================================================
void windows_dll_exit(void)
{
#ifdef RUN_TIME_DYNAMIC_LINKING
	DeinitOpenLibSys(&m_hOpenLibSys);
#else
	DeinitializeOls();
#endif
}

//==========================================================================
void windows_coldreset(void)
{
	WriteIoPortByte(0xCF9, 0x0E);
}

void WindowsKBCReset(void)
{
	WriteIoPortByte(0x64, 0xFE);
}

void WindowsPowerOff(void)
{
	//system("shutdown -s -t 5 -c \"System will shutdown after 5 second !!\"");
	system("shutdown -s -t 0");
}

int WindowsCheckKey(void)
{
	if(_kbhit())
	{
		return 1;
	}
	
	return 0;
}

int Read6662Port(BYTE offset, BYTE *value)
{
	BYTE retry = 0;
	
	while(1)
	{
		if(acpi_read_byte(offset, value) == 0)
		{
			break;
		}
		
		retry++;
		if(retry > 500)
		{
			return -1;
		}
		Sleep(10);	// 10ms
	}
	
	return 0;
}

#endif // _WINDOW_

