/*
 * isc.c
 *
 *  Created on: 2016. 1. 27.
 *      Author: SIS
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "isc.h"

uint8_t rwBuffer[PAGE_SZ * 1024];

uint8_t getCode2(uint8_t target)
{
	switch (target) {
	case MAIN:
	case STATUS:
	case IC_INFO:
	case STONE:
		return 0x4A;
	case INFO:
	case SFR:
	case SPP:
	case BIST:
		return 0xB7;
	default:
		return 0;
	}
}

#define PROCESS_PACKET(TARGET, COMMAND)		\
		packet_t packet = {\
				.code1 = 0xFB, .code2 = getCode2(TARGET),\
				.target = TARGET,\
				.cmd = COMMAND,\
		}

#define PROCESS_ADDR(ADDR)	\
	int k;\
	for (k = 0; k < ADDR_SZ; k++) {\
		packet.addr[k] = (ADDR >> (8 * (ADDR_SZ - 1 - k))) & 0xff;\
	}

void iscMassErase(target_t target)
{
	PROCESS_PACKET(target, MASS_ERASE);
	i2c_write((uint8_t *)&packet, 4 + ADDR_SZ);
}

void iscRead(target_t target, uint32_t addr, uint32_t length)
{
	PROCESS_PACKET(target, READ);
	PROCESS_ADDR(addr);

	i2c_write((uint8_t *)&packet, 4 + ADDR_SZ);
	i2c_read(rwBuffer, length);
}

#define PROCESS_WRITE(FUNCNAME, COMMAND)\
		void isc##FUNCNAME (target_t target, uint32_t addr, uint32_t length) \
		{ \
			PROCESS_PACKET(target, COMMAND); \
			PROCESS_ADDR(addr); \
			memcpy(packet.data, rwBuffer, length); \
			i2c_write((uint8_t *)&packet, 4 + ADDR_SZ + length); \
		}

PROCESS_WRITE(PageWrite, PAGE_WRITE)			// iscPageWrite(target_t target, uint32_t addr, uint32_t length)
PROCESS_WRITE(FlashWrite, FLASH_WRITE)			// iscFlashWrite(target_t target, uint32_t addr, uint32_t length)
PROCESS_WRITE(PageProgram, PAGE_PROGRAM)		// iscPageProgram(target_t target, uint32_t addr, uint32_t length)

void iscPageErase(target_t target, uint32_t addr)
{
	PROCESS_PACKET(target, PAGE_ERASE);
	PROCESS_ADDR(addr);
	i2c_write((uint8_t *)&packet, 4 + ADDR_SZ);
}

void iscEnter(void)
{
	PROCESS_PACKET(MAIN, ENTER);
	i2c_write((uint8_t *)&packet, 4 + ADDR_SZ);
}

void iscExit(void)
{
	PROCESS_PACKET(MAIN, EXIT);
	i2c_write((uint8_t *)&packet, 4 + ADDR_SZ);
}

/*
 * 		Applied function
 */
int statusRead(void)
{
	int ret;
	uint8_t buf = rwBuffer[0];

#define LIMIT	100

	int i;
	for (i = 0; i < LIMIT; i++) {
		iscRead(STATUS, 0, 1);
		if (rwBuffer[0] == DONE) {
			ret = 1;
//			printf("done\n");
			goto ter;
		}
	}
	ret = 0;
//	printf("fail\n");

	ter:
		rwBuffer[0] = buf;
		return ret;
}

int fwDownload(char *filename, int target)
{
	FILE *fp;
	if ((fp = fopen(filename, "rb")) == NULL) {
		printf("File open error\n");
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	uint32_t FILE_SZ = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t buf[FILE_SZ];
	fread(buf, sizeof(uint8_t), FILE_SZ, fp);
	fclose(fp);

	iscEnter();
	if (target == MAIN) {
		printf("[Main download]\n");
		iscMassErase(MAIN);
		if (statusRead()) {
			printf("Mass erase done!\n");
		} else {
			printf("Mass erase fail\n");
			return 0;
		}
	} else if(target == STONE) {
		printf("[Stone download]\n");
	}

	div_t sz;
	sz = div(FILE_SZ, PAGE_SZ);

	int i;
	printf("[");
	for (i = 0; i < sz.quot + (sz.rem != 0); i++) {
		printf(" ");
	}
	printf("]\r");

/*
	COORD pos;
	CONSOLE_SCREEN_BUFFER_INFO screeninfo;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &screeninfo)) {
		pos.X = screeninfo.dwCursorPosition.X;
		pos.Y = screeninfo.dwCursorPosition.Y;
	}
	pos.X += 1;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
*/

	for (i = 0; i < sz.quot + (sz.rem != 0); i++) {
		memset(rwBuffer, 0, PAGE_SZ);
		memcpy(rwBuffer, &buf[PAGE_SZ * i], (i == sz.quot) ? sz.rem : PAGE_SZ);
		iscPageWrite(target, PAGE_SZ * i, PAGE_SZ);

		if (!statusRead())
			goto err;

		uint8_t verify_buf[PAGE_SZ];
		memcpy(verify_buf, rwBuffer, PAGE_SZ);
		iscRead(target, PAGE_SZ * i, PAGE_SZ);
		if (memcmp(verify_buf, rwBuffer, PAGE_SZ))
			printf("x");
		else
			printf("=");
	}
	iscExit();
	printf("\nDownload is done!\n\n");
	return 1;

	err:
		iscExit();
		printf("\nFailed to download\n\n");
		return 0;
}

int fwDump(void)
{
	FILE *fp;
	fp = fopen("C:\\Users\\is.song\\Desktop\\dump.bin", "wb");

	printf("fw dump start\n");

	iscEnter();

	int i;
	for (i = 0; i < FLASH_SZ / PAGE_SZ; i++) {
		memset(rwBuffer, 0, PAGE_SZ);
		iscRead(MAIN, PAGE_SZ * i, PAGE_SZ);
		fwrite(rwBuffer, sizeof(uint8_t), PAGE_SZ, fp);

#if 0
		int j;
		for (j = 0; j < PAGE_SZ; j++) {
			printf("%02x", rwBuffer[j]);
			if ((j % 2) == 1) printf(" ");
			if ((j % 16) == 15) printf("\n");
		}
#endif
	}

	iscExit();
	fclose(fp);

	printf("fw dump is done\n");
	return 0;
}

void analog_trimming(void)
{
	struct {
		uint8_t cp_div: 4;
		uint8_t cp_sel: 4;
		unsigned: 1;
		uint8_t cp_en: 1;
		unsigned: 1;
		uint8_t cp_disch: 1;
		uint8_t bias_cal: 4;
		uint8_t bias_en: 1;
		uint8_t ldoa_mode: 1;
		uint8_t ldoa_en: 1;
		uint8_t bg_ana_cal: 5;
		uint8_t bg_ldo_cal: 5;
		uint8_t bg_mux: 1;
		uint8_t bg_ldo_en: 1;
		uint8_t isc_ana: 1;
	} __attribute__ ((packed)) tmp = {
			.bg_ldo_cal = 10,
			.bg_ana_cal = 10,
			.ldoa_mode = 0,
	};

	iscEnter();

	memcpy(rwBuffer, &tmp, 4);

	iscExit();
}

void WriteCalData(void)
{
	printf("write calibration data\n");

	memset(rwBuffer, 0, PAGE_SZ);
	rwBuffer[4] = 0x48;
	rwBuffer[5] = 16;		// BG_LDO_CAL
	rwBuffer[6] = 16;		// BG_ANA_CAL

	iscEnter();
	iscPageProgram(INFO, 0x00020000, PAGE_SZ);
	statusRead();
	iscExit();

	printf("writing cal data done!\n");
}

void WriteSFR(uint8_t *trim_val, uint32_t size)
{
	memset(rwBuffer, 0, PAGE_SZ);
	memcpy(rwBuffer, trim_val, size);

	iscPageWrite(SFR, 0, PAGE_SZ);
	statusRead();

	printf("SFR write done\n");
}

void WriteSPP(uint8_t *trim_val, uint32_t size)
{
	memset(rwBuffer, 0, PAGE_SZ);
	memcpy(rwBuffer, trim_val, size);

	iscPageErase(SPP, 0);
	statusRead();
	iscPageWrite(SPP, 0, PAGE_SZ);
	statusRead();

	printf("SPP write done\n");
}


void ReadSFR(void)
{
	memset(rwBuffer, 0, sizeof(rwBuffer));
	iscRead(SFR, 0, PAGE_SZ);
	statusRead();

	int i;
	for (i = 0; i < PAGE_SZ; i++) {
		printf("%02x ", rwBuffer[i]);

		if (i % 4 == 3) {
			printf("\n");
		}
	}
}

void ReadSPP(void)
{
	memset(rwBuffer, 0, sizeof(rwBuffer));
	iscRead(SPP, 0, PAGE_SZ);
	statusRead();

	int i;
	for (i = 0; i < PAGE_SZ; i++) {
		printf("%02x ", rwBuffer[i]);

		if (i % 4 == 3) {
			printf("\n");
		}
	}
}

void Trimming(void)
{
	uint8_t trim_val[] = {
			0x18, 0x82, 0x27, 0x27, 0xFF, 0x03, 0x4A, 0x0E,
			0x10, 0x20, 0x7A, 0xFC, 0x00, 0x00, 0x31, 0x02
	};

	iscEnter();
	WriteSFR(trim_val, sizeof(trim_val) / sizeof(uint8_t));
	WriteSPP(trim_val, sizeof(trim_val) / sizeof(uint8_t));

	printf("SFR\n");
	ReadSFR();
	printf("\nSPP\n");
	ReadSPP();
	iscExit();
}

void make_testbin(uint8_t val)
{
	FILE *fp;
	fp = fopen("C:\\Users\\is.song\\Desktop\\testbin.bin", "wb");

	char buf[PAGE_SZ];
	memset((void *)buf, val, PAGE_SZ);

	int i;
	for (i = 0; i < FLASH_SZ / PAGE_SZ; i++) {
		fwrite(buf, 1, PAGE_SZ, fp);
	}

	fclose(fp);
	printf("Finish making testbin!!\n");
}

