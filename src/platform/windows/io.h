#ifndef __IO_H__
#define __IO_H__

#ifdef _WINDOW_

#define inp(addr)               ReadIoPortByte(addr)
#define inpw(addr)              ReadIoPortWord(addr)
#define inpd(addr)              ReadIoPortDword(addr)
#define outp(addr,data)			WriteIoPortByte(addr, data)
#define outpw(addr,data)        WriteIoPortWord(addr, data)
#define outpd(addr,data)        WriteIoPortDword(addr, data)

#endif // _WINDOW_

#endif // __IO_H__

