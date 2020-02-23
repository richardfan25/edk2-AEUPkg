#ifndef __IO_H__
#define __IO_H__

#ifdef _LINUX_

#define inp(addr)               inb(addr)
#define inpw(addr)              inw(addr)
#define inpd(addr)              ((uint32_t)inw(addr) | (uint32_t)inw(addr + 2) << 16)
#define outp(addr,data)			outb(data,addr)
#define outpw(addr,data)        outw(data,addr)
#define outpd(addr,data)        do{outw(addr, (uint16_t)data);outw(addr + 2, (uint16_t)(data >> 16));}while(0)

#endif // _LINUX_

#endif // __IO_H__

