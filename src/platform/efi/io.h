#ifndef __IO_H__
#define __IO_H__

#ifdef _UEFI_

#define inp(addr)               IoRead8(addr)
#define inpw(addr)              IoRead16(addr)
#define inpd(addr)              IoRead32(addr)
#define outp(addr,data)			IoWrite8(addr,data)
#define outpw(addr,data)        IoWrite16(addr,data)
#define outpd(addr,data)        IoWrite32(addr,data)

#endif // _UEFI_

#endif // __IO_H__

