#ifndef __FX2_INTERFACE_H__
#define __FX2_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>

#define i2c_write 		fx2_i2c_write
#define i2c_read 		fx2_i2c_read
#define get_intr_value	fx2_get_intr_value

enum {
	USB_INSTR_NONE = 0,

	//--------------------------
	// Shutdown module
	//--------------------------
	USB_INSTR_SHUTDOWN,
	USB_INSTR_POWER_ON_FOR_EMULATION,

	//--------------------------
	// VDD
	//--------------------------
	USB_INSTR_VDD_SET_LOW,
	USB_INSTR_VDD_SET_HIGH,
	USB_INSTR_VDD_SET_VALUE,
	USB_INSTR_VDD_SET_LOW_AND_HIGH,

	//--------------------------
	//	RESETB
	//--------------------------
	USB_INSTR_RSTB_SET_DIRECTION,
	USB_INSTR_RSTB_SET_INPUT,
	USB_INSTR_RSTB_SET_OUTPUT,
	USB_INSTR_RSTB_SET_VALUE,
	USB_INSTR_RSTB_SET_HIGH,
	USB_INSTR_RSTB_SET_LOW,
	USB_INSTR_RSTB_GET_VALUE,

	//--------------------------
	//	CE
	//--------------------------
	USB_INSTR_CE_SET_DIRECTION,
	USB_INSTR_CE_SET_INPUT,
	USB_INSTR_CE_SET_OUTPUT,
	USB_INSTR_CE_SET_VALUE,
	USB_INSTR_CE_SET_HIGH,
	USB_INSTR_CE_SET_LOW,
	USB_INSTR_CE_GET_VALUE,

	//--------------------------
	//	I2C
	//--------------------------
	USB_INSTR_I2C_WRITE,
	USB_INSTR_I2C_READ,
	USB_INSTR_I2C_WRITE_AND_READ,
	USB_INSTR_I2C_WRITE_AND_READ_WITH_RESTART,
	USB_INSTR_I2C_WRITE_WITHOUT_STOP,
	USB_INSTR_I2C_READ_WITHOUT_STOP,

	USB_INSTR_I2C_START,
	USB_INSTR_I2C_REPEATED_START,
	USB_INSTR_I2C_SALVE_ADDRESS,
	USB_INSTR_I2C_WRITE_SALVE_ADDRESS,
	USB_INSTR_I2C_READ_SALVE_ADDRESS,
	USB_INSTR_I2C_WRITE_DATA,
	USB_INSTR_I2C_WRITE_DATA_WITH_STOP,
	USB_INSTR_I2C_READ_DATA,
	USB_INSTR_I2C_READ_DATA_WITH_STOP,
	USB_INSTR_I2C_STOP,

	USB_INSTR_I2C_SET_TYPE,
	USB_INSTR_I2C_GET_TYPE,

	//----------------------------------------
	//
	//	ISP Commands
	//	MCS8000, MMS100, MMS100s
	//
	//----------------------------------------
	USB_INSTR_ISP_POWER_ON_FOR_DOWNLOAD,

	USB_INSTR_ISP_REFERENCE_INIT,
	USB_INSTR_ISP_REFERENCE_ERASE,
	USB_INSTR_ISP_INFORMATION_MASS_ERASE,
	USB_INSTR_ISP_INFORMATION_PAGE_ERASE,
	USB_INSTR_ISP_INFORMATION_WRITE,
	USB_INSTR_ISP_INFORMATION_INIT,
	USB_INSTR_ISP_INFORMATION_READ,
	USB_INSTR_ISP_MAIN_ERASE,
	USB_INSTR_ISP_MAIN_WRITE,
	USB_INSTR_ISP_MAIN_READ,
	USB_INSTR_ISP_SET_ERASE_TIMING,
	USB_INSTR_ISP_SET_PROGRAM_TIMING,

	USB_INSTR_ISP_PREPARE_WRITING,

	USB_INSTR_ISP_BYPASS_NEXT_CHIP,
	USB_INSTR_ISP_BYPASS_NEXT_CHIP_FIX,

	USB_INSTR_ISP_ENTER_MODE,
	USB_INSTR_ISP_EXIT_MODE,

	USB_INSTR_ISP_FLASH_CLEAR,	// 2013.03.27 Downloader v2.0

	//--------------------------
	// Special Command
	//--------------------------
	USB_INSTR_SPECIAL_COMMAND1,

	USB_INSTR_ISP_PROGRAM_SLAVE_ADDR,

	//--------------------------
	// ADDED
	//--------------------------

	USB_INSTR_ISC_READ,
	USB_INSTR_SEND_SERIAL_DATA,
	USB_INSTR_GET_TEST_RESULT,
	USB_INSTR_POWER_ON_WITH_SLAVE_ADDRESS,
	USB_INSTR_ISP_PAGE_ERASE,	// 2013.07.18 by doo for AIT ISP

	//--------------------------
	// The end
	//--------------------------

	USB_INSTR_LIM
};

enum {
	FX2_BULK_WRITE_ERR = 1,
	FX2_BULK_READ_ERR,
};

void fx2_set_slave_addr(uint8_t addr);
void fx2_set_vdd(bool val);
void fx2_set_rstb(bool val);
int fx2_get_intr_value(void);
int fx2_i2c_write(uint8_t *buf, int len);
int fx2_i2c_read(uint8_t *buf, int len);
int fx2_i2c_read_long(uint8_t *buf, int len);
void fx2_reset_vdd(int udelay);

#endif

