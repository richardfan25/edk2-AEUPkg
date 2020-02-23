#ifdef _LINUX_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/io.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/reboot.h>
//#include <sys/syslog.h>
#include <linux/reboot.h>
//#include "init_shared.h"
#include "io.h"

//const char * const init_sending_format = "Sending SIG%s to all processes.";
//const char * const shutdown_format = "\r%s\n";
int gResetTime = 0;
int gResetMode = 0;

//==========================================================================
int linux_check_permission(void)
{
	if(getuid() != 0){
		fprintf(stderr, "ERROR: root permission needed.\n\n");
		return -1;
	}
	
	return 0;
}

//==========================================================================
int linux_open_ioport(uint16_t addr)
{
	if(ioperm(addr, 1, 1)){
		perror("ioperm");
		return -1;
	}

	return 0;
}

//==========================================================================
int linux_close_ioport(uint16_t addr)
{
	if(ioperm(addr, 1, 0)){
		perror("ioperm");
		return -1;
	}

	return 0;
}

//==========================================================================
void linux_reset_notify(void)
{
	/* Don't kill ourself */
	signal(SIGTERM,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	setpgrp();

	/* Allow Ctrl-Alt-Del to reboot system. */
	reboot(RB_ENABLE_CAD);

	//openlog("/var/log/log, 0, pri);

	//message = "\nThe system is going down NOW !!";
	//syslog(pri, "%s", message);
	//printf(shutdown_format, message);

	sync();

	/* Send signals to every process _except_ pid 1 */
	//message = "Sending SIGTERM to all processes.";
	//syslog(pri, "%s", message);
	//printf(shutdown_format, message);

	kill(-1, SIGTERM);
	sleep(1);
	sync();

	//message = "Sending SIGKILL to all processes.";
	//syslog(pri, "%s", message);
	//printf(shutdown_format, message);

	kill(-1, SIGKILL);
	sleep(1);

	sync();
}
//==========================================================================
void linux_coldreset(void)
{
/*	linux_sys_reset();
#if __GNU_LIBRARY__ > 5
    reboot(LINUX_REBOOT_CMD_RESTART);
#else
    reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART);
#endif*/
	sync();
	linux_open_ioport(0xCF9);
	outp(0xCF9, 0x0E);
}

void LinuxKBCReset(void)
{
	sync();
	linux_open_ioport(0x64);
	outp(0x64, 0xFE);
}

void LinuxPowerOff(void)
{
	sync();
	system("shutdown now");//reboot(RB_POWER_OFF);
}

int LinuxCheckKey(void)
{
	int ret = 0;
	fd_set rfds;
	struct timeval tv;
	
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	
	system("stty cbreak -echo");
	
	if(select(1, &rfds, NULL, NULL, &tv) > 0)
	{
		ret = 1;
	}
	
	system("stty cooked echo");
	
	return ret;
}
#endif // _LINUX_

