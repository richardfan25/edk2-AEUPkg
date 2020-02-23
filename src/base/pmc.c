#include <stdio.h>
#include <stdint.h>

#include "../platform/adaptate.h"
#include "pmc.h"

#define MSG_IO_INFO				0

#if (MSG_IO_INFO > 0)
#define mMSG_IO_GET_DATA()		fprintf(stderr, "I/O: get data 0x%02X from 0x%X\n",*val,port->data)
#else
#define mMSG_IO_GET_DATA()
#endif

#if (MSG_IO_INFO > 0)
#define mMSG_IO_SET_DATA()		fprintf(stderr, "I/O: send 0x%02X data to 0x%X\n",val,port->data)
#else
#define mMSG_IO_SET_DATA()
#endif

#if (MSG_IO_INFO > 0)
#define mMSG_IO_GET_CMD()		fprintf(stderr, "I/O: get cmd 0x%02X from 0x%X\n",*val,port->cmd)
#else
#define mMSG_IO_GET_CMD()
#endif

#if (MSG_IO_INFO > 0)
#define mMSG_IO_SET_CMD()		fprintf(stderr, "I/O: send 0x%02X cmd to 0x%X\n",cmd,port->cmd)
#else
#define mMSG_IO_SET_CMD()
#endif

#if (MSG_IO_INFO > 0)
#define mMSG_IO_OPEN_FAIL()		fprintf(stderr, "I/O port(0x%X,0x%X) open fail.\n", port->cmd, port->data);
#else
#define mMSG_IO_OPEN_FAIL()		fprintf(stderr, "ERROR: Access EC Fail.\n");
#endif

/*==============================================================*/
// Open I/O port of EC
// Input:	cmd	 - ioport num that is a uint16_t data.
//			data - ioport num that is a uint16_t data.
//
// Output:	-1	 - fail
//			 0	 - success
/*==============================================================*/
int pmc_open_port(pmc_port_t *port)
{
	// Get Access IO port permission
	sys_open_ioport(port->cmd);
	
	// Open IO port
	sys_open_ioport(port->data);
	
	// Check IO port
	if(inp(port->cmd) == 0xff){
		mMSG_IO_OPEN_FAIL();
		return -1;
	}
	// Read data port to clear OBF
	inp(port->data);
	return 0;
}
/*==============================================================*/
// Close I/O port of EC
// Input:	cmd	 - ioport num that is a uint16_t data.
//			data - ioport num that is a uint16_t data.
//
// Output:	-1	 - fail
//			 0	 - success
/*==============================================================*/
int pmc_close_port(pmc_port_t *port)
{
	if(port->cmd != 0){
		sys_close_ioport(port->cmd);
	}	
	
	if(port->data != 0){
		sys_close_ioport(port->data);
	}
	return 0;
}
/*==============================================================*/
// check status port IBF
// Input:	none
//
// Output:	-1	 - busy, timeout
//			 0	 - success, IBF clear
/*==============================================================*/
int pmc_check_ibf(pmc_port_t *port)
{
	uint16_t retry = 0;
	
	while(inp(port->cmd) & dPMCSTA_IBF_BIT){					// 2sec
		if(++retry > dPMCSTA_CHK_DLY){
			fprintf(stderr, "ERROR: 0x%x IBF isn't clear.\n",port->cmd);
			return -1;
		}
		//sys_delay(1);									// loop tick 10ms
	}
	return 0;
}

/*==============================================================*/
// check status port OBF
// Input:	none
//
// Output:	-1	 - fail, no received
//			 0	 - success, OBF set
/*==============================================================*/
int pmc_check_obf(pmc_port_t *port)
{
	uint16_t retry = 0;
	
	while((inp(port->cmd) & dPMCSTA_OBF_BIT) == 0){					// 2sec
		if(++retry > dPMCSTA_CHK_DLY){
#ifndef _WINDOW_
			fprintf(stderr, "ERROR: 0x%x OBF isn't set.\n",port->cmd);
#endif
			return -1;
		}
		//sys_delay(1);									// loop tick 10ms
	}
	return 0;
}

/*==============================================================*/
// write val to cmd port
// Input:	cmd	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int pmc_write_cmd(pmc_port_t *port, uint8_t cmd)
{
	// clear data port
	if((inp(port->cmd) & dPMCSTA_OBF_BIT) != 0){
		inp(port->data);								// dummy read for data port
	}
	
	// send new command
	if(pmc_check_ibf(port) == 0){
		mMSG_IO_SET_CMD();
		outp(port->cmd, cmd);							// write val to cmd port
			return 0;
	}
	else 
		return -1;
}

/*==============================================================*/
// write val to data port
// Input:	val	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int pmc_write_data(pmc_port_t *port, uint8_t val)
{
	if(pmc_check_ibf(port) == 0){
		mMSG_IO_SET_DATA();
		outp(port->data, val);							// write val to cmd port
		return 0;
	}
	else 
		return -1;
}

/*==============================================================*/
// read val from data port
// Input:	val	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int pmc_read_data(pmc_port_t *port, uint8_t *val)
{
	if(pmc_check_obf(port) == 0){
		*val = inp(port->data);							// read val from data port
		mMSG_IO_GET_DATA();
		return 0;
	}
	else 
		return -1;
}


/*==============================================================*/
// ite check status port IBF
// Input:	none
//
// Output:	-1	 - busy, timeout
//			 0	 - success, IBF clear
/*==============================================================*/
int ite_pmc_check_ibf(pmc_port_t *port)
{
	uint8_t flag = 0;
	uint32_t retry = 0;
	
	while(inp(port->cmd) & dPMCSTA_IBF_BIT){
		if(++retry > 0x7FFFFFFF){
			fprintf(stderr, "ERROR: 0x%x IBF isn't clear.\n",port->cmd);
			return -1;
		}
		if((retry % 10000) == 0)
		{
			flag = 1;
			fprintf(stderr, "\r0x%x IBF retry %d.", port->cmd, retry);
		}
		//sys_delay(1);									// loop tick 10ms
	}
	
	if(flag)
	{
		fprintf(stderr, "\r0x%x IBF retry %d.\n", port->cmd, retry);
	}
	
	return 0;
}

/*==============================================================*/
// ite check status port OBF
// Input:	none
//
// Output:	-1	 - fail, no received
//			 0	 - success, OBF set
/*==============================================================*/
int ite_pmc_check_obf(pmc_port_t *port)
{
	uint8_t flag = 0;
	uint32_t retry = 0;
	
	while((inp(port->cmd) & dPMCSTA_OBF_BIT) == 0){
		if(++retry > 0x7FFFFFFF){
#ifndef _WINDOW_
			fprintf(stderr, "ERROR: 0x%x OBF isn't set.\n",port->cmd);
#endif
			return -1;
		}
		if((retry % 10000) == 0)
		{
			flag = 1;
			fprintf(stderr, "\r0x%x OBF retry %d.", port->cmd, retry);
		}
		//sys_delay(1);									// loop tick 10ms
	}
	
	if(flag)
	{
		fprintf(stderr, "\r0x%x OBF retry %d.\n", port->cmd, retry);
	}
	
	return 0;
}

/*==============================================================*/
// ite write val to cmd port
// Input:	cmd	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int ite_pmc_write_cmd(pmc_port_t *port, uint8_t cmd)
{
	// clear data port
	if((inp(port->cmd) & dPMCSTA_OBF_BIT) != 0){
		inp(port->data);								// dummy read for data port
	}
	
	// send new command
	if(ite_pmc_check_ibf(port) == 0){
		mMSG_IO_SET_CMD();
		outp(port->cmd, cmd);							// write val to cmd port
			return 0;
	}
	else 
		return -1;
}

/*==============================================================*/
// ite write val to data port
// Input:	val	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int ite_pmc_write_data(pmc_port_t *port, uint8_t val)
{
	if(ite_pmc_check_ibf(port) == 0){
		mMSG_IO_SET_DATA();
		outp(port->data, val);							// write val to cmd port
		return 0;
	}
	else 
		return -1;
}

/*==============================================================*/
// ite read val from data port
// Input:	val	-	a byte data that want to write to cmd port
//
// Output:	-1	- busy, timeout
//			0	- success
/*==============================================================*/
int ite_pmc_read_data(pmc_port_t *port, uint8_t *val)
{
	if(ite_pmc_check_obf(port) == 0){
		*val = inp(port->data);							// read val from data port
		mMSG_IO_GET_DATA();
		return 0;
	}
	else 
		return -1;
}
