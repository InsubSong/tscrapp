/*
 * isc.h
 *
 *  Created on: 2016. 1. 27.
 *      Author: SIS
 */

#ifndef __ISC_H__
#define __ISC_H__

#include <stdio.h>
#include <stdint.h>

#include "fx2_interface.h"

#define MMS427		1
#define MMS587N		2
#define MMS649		3

//#define IC			MMS587N
#define IC			MMS427

#if IC == MMS427
#define ADDR_SZ		2
#define PAGE_SZ		128
#define FLASH_SZ	(48 * 1024)

#elif IC == MMS587N
#define ADDR_SZ		4
#define PAGE_SZ		64
#define FLASH_SZ	(64 * 1024)

#elif IC == MMS649
#define ADDR_SZ		4
#define PAGE_SZ		128
#define FLASH_SZ	(64 * 1024)

#endif

typedef enum {
	MAIN = 0x00,
	STATUS = 0x36,
	INFO = 0xC0,
	SFR = 0xDD,
	SPP = 0xD2,
	IC_INFO = 0x50,
	STONE = 0x0C,
	BIST = 0xA3,
} target_t;

typedef enum {
	MASS_ERASE = 0x15,
	PAGE_ERASE = 0x8F,
	PAGE_WRITE = 0xA5,
	FLASH_WRITE = 0x5F,
	PAGE_PROGRAM = 0x54,
	READ = 0xC2,
	EXIT = 0x66,
	ENTER = 0x65,
} cmd_t;

typedef enum {
	BUSY = 0x96,
	DONE = 0xAD,
} status_t;

typedef struct {
	uint8_t code1;
	uint8_t code2;
	uint8_t target;
	uint8_t cmd;
	uint8_t addr[ADDR_SZ];
	uint8_t data[PAGE_SZ];
} packet_t;

void iscMassErase(target_t target);
void iscRead(target_t target, uint32_t addr, uint32_t length);
void iscPageErase(target_t target, uint32_t addr);
void iscEnter(void);
void iscExit(void);

int statusRead(void);
int fwDownload(char *filename, int target);
int fwDump(void);

void WriteSFR(uint8_t *trim_val, uint32_t size);
void WriteSPP(uint8_t *trim_val, uint32_t size);
void ReadSFR(void);
void ReadSPP(void);

void Trimming(void);

void make_testbin(uint8_t val);

#endif
