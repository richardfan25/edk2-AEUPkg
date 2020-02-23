#ifndef __MISC_H__
#define __MISC_H__

#ifdef _DOS_

#define sys_check_permission()
#define sys_open_ioport(addr)
#define sys_close_ioport(addr)
#define sys_clearscreen()			_clearscreen(_GCLEARSCREEN)
#define sys_reset()              	outp(0xCF9, 0x0E)
#define sys_reset_notify()       
#define sys_delay(a)                delay(a)		// unit: ms

#define sys_dll_exit()

#define ReadACPIByte(a, b)			acpi_read_byte(a, b)
#define CheckKey()					(kbhit()?1:0)

#endif // _DOS_

#endif // __MISC_H__

