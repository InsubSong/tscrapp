#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "fx2_interface.h"
#include "usb_interface.h"

#define DEFAULT_SLAVE_ID		(0x48)
//#define DEFAULT_SLAVE_ID		(0x34)

#define FX2_BUF_SZ			1024

static uint8_t				gBuffer[FX2_BUF_SZ];
static uint8_t				slave_id = DEFAULT_SLAVE_ID;

void fx2_set_slave_addr(uint8_t addr)
{
	slave_id = addr;
}

void fx2_set_vdd(bool val)
{
	uint8_t cmd, result;
	int ret;

	if (val)
		cmd = USB_INSTR_POWER_ON_FOR_EMULATION;
	else
		cmd = USB_INSTR_SHUTDOWN;

	ret = usb_bulkWrite(&cmd, 1);
	if (ret) {
		//printf("Unable to perform bulk write @%s (%08x)\n", __func__, ret);
	}

	ret = usb_bulkRead(&result, 1);
	if (ret) {
		//printf("Unable to perform bulk read @%s (%08x)\n", __func__, kr);
	}

//	usleep(100000);
	Sleep(100);
}

void fx2_reset_vdd(int udelay)
{
	uint8_t cmd, result;
	int ret;

	cmd = USB_INSTR_SHUTDOWN;

	ret = usb_bulkWrite(&cmd, 1);
	if (ret) {
		//printf("Unable to perform bulk write @%s (%08x)\n", __func__, ret);
	}

	ret = usb_bulkRead(&result, 1);
	if (ret) {
		//printf("Unable to perform bulk read @%s (%08x)\n", __func__, ret);
	}

//	usleep(udelay);
	Sleep(udelay / 1000);

	cmd = USB_INSTR_POWER_ON_FOR_EMULATION;

	ret = usb_bulkWrite(&cmd, 1);
	if (ret) {
		//printf("Unable to perform bulk write @%s (%08x)\n", __func__, ret);
	}

	ret = usb_bulkRead(&result, 1);
	if (ret) {
		//printf("Unable to perform bulk read @%s (%08x)\n", __func__, kr);
	}

	printf("%s, result after power one : %d\n", __func__, result);
}

int fx2_get_intr_value(void)
{
	uint8_t result[2];
	int ret;

	gBuffer[0] = USB_INSTR_RSTB_GET_VALUE;
	ret = usb_bulkWrite(gBuffer, 1);
	if (ret) {
		//printf("Unable to perform bulk write @%s (%08x)\n", __func__, ret);
		ret = -FX2_BULK_WRITE_ERR;
		goto err;
	}

	ret = usb_bulkRead(result, 2);
	if (ret) {
		//printf("Unable to perform bulk read @%s (%08x)\n", __func__, ret);
		ret = -FX2_BULK_READ_ERR;
		goto err;
	}

	ret = !!result[1];

err:
	return ret;
}

int fx2_i2c_write(uint8_t *buf, int len)
{
	int ret = 0;
	uint8_t result;

	/* i2c write */
	gBuffer[0] = USB_INSTR_I2C_WRITE;
	gBuffer[1] = slave_id;
	gBuffer[2] = len;
	memcpy(gBuffer + 3, buf, len);
	ret = usb_bulkWrite(gBuffer,len + 3);
	if (ret) {
		//printf("Unable to perform bulk write @%s (%08x)\n", __func__, ret);
		ret = -FX2_BULK_WRITE_ERR;
		goto out;
	}

	ret = usb_bulkRead(&result, 1);
	if (ret || !result) {
		//printf("Unable to perform bulk read @%s ret : %08x, result : %08x\n", __func__, ret, result);
		ret = -FX2_BULK_READ_ERR;
		goto out;
	}

	ret = len;

out:
	return ret;
}

#define MODE	1

#if MODE == 0

int fx2_i2c_read(uint8_t *buf, int len)
{
	int ret;

#define BUF_SZ		248  //248

	div_t sz = div(len, BUF_SZ + 1);

	memset((void *)gBuffer, 0, sizeof(gBuffer));

	gBuffer[0] = USB_INSTR_I2C_READ;
	gBuffer[1] = slave_id;
	gBuffer[2] = len;
	usb_bulkWrite(gBuffer, 3);

	if (len > BUF_SZ) {
		usb_bulkRead(gBuffer, BUF_SZ + 1);
		memcpy(buf, gBuffer + 1, BUF_SZ);

		usb_bulkRead(gBuffer, sz.rem + 1);
		memcpy(buf + BUF_SZ, gBuffer + 1, sz.rem);
	} else {
		usb_bulkRead(gBuffer, len + 1);
		memcpy(buf, gBuffer + 1, len);
	}


#if 0

	int i, size;
	for (i = 0; i < sz.quot + (sz.rem != 0); i++) {
		size = (i == sz.quot)? sz.rem: BUF_SZ;

//		gBuffer[0] = (i == sz.quot)? USB_INSTR_I2C_READ: USB_INSTR_I2C_REPEATED_START;
		gBuffer[0] = (i == sz.quot)? USB_INSTR_I2C_READ: USB_INSTR_I2C_READ_WITHOUT_STOP;
//		gBuffer[0] = USB_INSTR_I2C_READ;
		gBuffer[1] = slave_id;
		gBuffer[2] = size;
		ret = usb_bulkWrite(gBuffer, 3);
		if (ret) {
			ret = -FX2_BULK_WRITE_ERR;
			goto err;
		}

		ret = usb_bulkRead(gBuffer, size + 1);
		if (ret) {
			ret = -FX2_BULK_READ_ERR;
			goto err;
		}

		memcpy(buf + BUF_SZ * i, gBuffer + 1, size);
	}
#endif

	if (0)
	{
		int k;
		for (k = 0; k < 500; k++) {
			printf("%02x, ", gBuffer[k + 1]);
			if (k % 20 == 19)
				puts("");
		}
		puts("");
	}

	ret = len;

err:
	return len;
}

#elif MODE == 1
int fx2_i2c_read(uint8_t *buf, int len)
{
	uint32_t numBytesRead = len + 1;
	int ret;

	memset((void *)gBuffer, 0, sizeof(gBuffer));

	gBuffer[0] = USB_INSTR_I2C_READ;
	gBuffer[1] = slave_id;
	gBuffer[2] = len;

	ret = usb_bulkWrite(gBuffer, 3);
	if (ret) {
		//printf("Unable to perform bulk write (%08x)\n", ret);
		ret = -FX2_BULK_WRITE_ERR;
		goto err;
	}

	ret = usb_bulkRead(gBuffer, numBytesRead);
	if (ret) {
		//printf("Unable to perform bulk read (%08x)\n", ret);
		ret = -FX2_BULK_READ_ERR;
		goto err;
	}

	memcpy(buf, gBuffer + 1, len);

	ret = len;

err:
	return len;
}
#endif



#define READ_LONG_UNIT	248
int fx2_i2c_read_long(uint8_t *buf, int len)
{
	int ret;
	int n = READ_LONG_UNIT;

	gBuffer[0] = USB_INSTR_I2C_READ_WITHOUT_STOP;
	gBuffer[1] = slave_id;
	gBuffer[2] = n;

	ret = usb_bulkWrite(gBuffer, 3);
	if (ret) {
		ret = -FX2_BULK_WRITE_ERR;
		goto err;
	}

	ret = usb_bulkRead(gBuffer, n + 1);
	if (ret) {
		ret = -FX2_BULK_READ_ERR;
		goto err;
	}

	memcpy(buf, gBuffer + 1, n);

	// process remainder
	while (n < len) {
		if (n + READ_LONG_UNIT >= len) {
			gBuffer[0] = USB_INSTR_I2C_READ_DATA_WITH_STOP;
			gBuffer[1] = len - n;
		} else {
			gBuffer[0] = USB_INSTR_I2C_READ_DATA;
			gBuffer[1] = READ_LONG_UNIT;
		}

		ret = usb_bulkWrite(gBuffer, 2);
		if (ret) {
			ret = -FX2_BULK_WRITE_ERR;
			goto err;
		}

		ret = usb_bulkRead(gBuffer, n + 1);
		if (ret) {
			ret = -FX2_BULK_READ_ERR;
			goto err;
		}

		memcpy(buf + n, gBuffer + 1, len);

		n += READ_LONG_UNIT;
	}

	ret = len;

err:
	return len;

}







