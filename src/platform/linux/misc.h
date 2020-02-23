#ifndef __MISC_H__
#define __MISC_H__

#ifdef _LINUX_
#include <stdint.h>

#define sys_check_permission()  	if(linux_check_permission() != 0) {return -1;}

#define sys_open_ioport(addr)   	if(linux_open_ioport(addr) != 0) {return -1;}

#define sys_close_ioport(addr)  	if(linux_close_ioport(addr) != 0) {return -1;}

#define sys_clearscreen()       	if(system("tput clear"))
#define sys_reset()          		linux_coldreset()
#define sys_reset_notify()       	linux_reset_notify()

#define sys_delay(a)                usleep((useconds_t)(a * 1000))		// unit: ms

#define sys_dll_exit()

#define KBCReset()					LinuxKBCReset()
#define OSPowerOff()				LinuxPowerOff()
#define ReadACPIByte(a, b)			acpi_read_byte(a, b)
#define CheckKey()					LinuxCheckKey()

extern int linux_check_permission(void);
extern int linux_open_ioport(uint16_t addr);
extern int linux_close_ioport(uint16_t addr);
extern void linux_coldreset(void);
extern void linux_reset_notify(void);

extern void LinuxKBCReset(void);
extern void LinuxPowerOff(void);
extern int LinuxCheckKey(void);



extern int gResetTime;
extern int gResetMode;

#endif // _LINUX_

#endif // __MISC_H__

