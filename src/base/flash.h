#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_ID_LEN				3
#define FLASH_TYPE_SST				0xBF
#define FLASH_BLOCK_SIZE          	0x10000
#define FLASH_SECTOR_SIZE			0x1000
#define FLASH_PAGE_SIZE				0x100
/*==============================================================*/
// SPI Flash
/*==============================================================*/
// Commands list
#define FLASH_CMD_WRSR     			0x01			// Write Status Register
#define FLASH_CMD_PP   				0x02			// Page Program
#define FLASH_CMD_READ  			0x03			// Read Data
#define FLASH_CMD_WRDI    			0x04			// Disable Write
#define FLASH_CMD_RDSR    			0x05			// Read Status Register
#define FLASH_CMD_WREN     			0x06			// Enable Write
#define FLASH_CMD_SE   				0x20			// Sector Erase
#define FLASH_CMD_REMS     			0x90			// Read Electronic Manufacturer & Device ID
#define FLASH_CMD_RDID     			0x9F			// Read Identification
#define FLASH_CMD_CE	 			0xC7			// Chip Erase
#define FLASH_CMD_BE   				0xD8			// Block Erase
// Below For SST ROM Only
#define FLASH_CMD_EWSR 				0x50			// Enable Write Status Register
#define FLASH_CMD_AAIP   			0xAD			// Auto Address Increment Programming

// Flash Read Status Register
#define FLASH_RDSR_BIT_BUSY			0x01
#define FLASH_RDSR_BIT_WREN			0x02


#endif // __FLASH_H__

