#ifndef __MISC_H__
#define __MISC_H__

#ifdef _UEFI_

#define sys_check_permission()
#define sys_open_ioport(addr)
#define sys_close_ioport(addr)
#define sys_clearscreen()
#define sys_reset()              	gRT->ResetSystem(EfiResetShutdown,EFI_SUCCESS,0,NULL)
#define sys_reset_notify()       
#define sys_delay(a)                usleep((useconds_t)(a * 1000))		// unit: ms

#define sys_dll_exit()

#define ReadACPIByte(a, b)			acpi_read_byte(a, b)

extern EFI_HANDLE				gImageHandle;
extern EFI_SYSTEM_TABLE			*gST;
extern EFI_BOOT_SERVICES        *gBS;
__inline int CheckKey(void)
{
	EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *ConInEx;
	EFI_STATUS 		status;
	EFI_KEY_DATA 	keydata;

	// open extension console input
    status = gBS->OpenProtocol(
                 gST->ConsoleInHandle,
                 &gEfiSimpleTextInputExProtocolGuid,
                 (VOID **) &ConInEx,
                 gImageHandle,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if(EFI_ERROR(status))
		return 0;
	
	status = ConInEx->ReadKeyStrokeEx(ConInEx, &keydata);
	if(status == EFI_SUCCESS)
	{
		return 1;
	}
	
	return 0;
}

#endif // _UEFI_

#endif // __MISC_H__

