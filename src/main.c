#include <signal.h>
#include <conio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "usb_interface.h"
#include "fx2_interface.h"
#include "isc.h"

#define _BV(bit)		(1 << bit)

enum {
	CLOSE,
	OPEN,
};

enum {
	CMD_SET_LOG_MODE = 0xA0,
	CMD_SET_LOG_TYPE = 0xA1,
};

enum {
	LT_INT = _BV(0),
	LT_REF = _BV(1),
	LT_RAW = _BV(2),
	LT_GRP = _BV(3),
	LT_PRT = _BV(4),
	LT_DLY = _BV(5),
	LT_SS = _BV(6),
};

enum {
	U8 = 1,
	S8,
	U16,
	S16,
	U32,
	S32,
};

FILE *fp;
static bool loop = true;
static char *trigger = NULL;
static bool pause = false;
static bool log_mode = false;
static uint8_t log_flag = 0;
static bool clr_scr = false;

unsigned short uSlaveId;
char* BinFileName;
static bool bFwDown = false;
static bool bFwDump = false;

static bool trim_flag = false;

static void signal_handler(int sig)
{
	if (sig == SIGINT) {
		printf("\n\n[Ctrl+C] User terminated running of program.\n\n");
		loop = false;
	}
}

static void log_init(void)
{
	char *file_name = "next_count";
	char cnt = '0';
	char log_file[20];
	int fd;

	fd = open(file_name, O_RDWR);

	if (fd < 0) {
		printf("failed to open %s, creating one instead\n", file_name);
		fd = open(file_name, O_CREAT | O_RDWR, 0664);
	} else {
		read(fd, &cnt, 1);
	}

	sprintf(log_file, "output%c.log", cnt);
	printf("log file : %s\n", log_file);

	fp = fopen(log_file, "w+");

	if (cnt == '9')
		cnt = '0';
	else
		cnt++;

	lseek(fd, 0, SEEK_SET);
	write(fd, &cnt, 1);

	close(fd);
}

static void log_close(void)
{
	int n;
	char buf[512];
	FILE *fp2;

	fp2 = fopen("output.log", "w+");

	fseek(fp, 0, SEEK_SET);

	while ((n = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
		fwrite(buf, sizeof(char), n, fp2);
	}

	fclose(fp);
	fclose(fp2);
}

static inline void print_log(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);
	fflush(fp);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static void log_mask_set(int val)
{
	uint8_t buf[2];

	log_flag ^= val;

	buf[0] = CMD_SET_LOG_TYPE;
	buf[1] = log_flag;

	i2c_write(buf, 2);
}

void gotoxy( int x, int y)
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

static void master_packet_sender(void)
{
	uint8_t buf[10];
	char cin;

	if (kbhit()) {
		cin = _getch();
		switch (cin) {
			case 'l':
				buf[0] = CMD_SET_LOG_MODE;
				buf[1] = log_mode = !log_mode;
				i2c_write(buf, 2);
				break;

			// isc eixt
			case 'e':
				printf("enter / exit\n");
				iscEnter();
				Sleep(100);
				iscExit();
				break;

			case 'i':
				log_mask_set(LT_INT);
				break;
			case 'r':
				log_mask_set(LT_RAW);
				break;
			case 'g':
				log_mask_set(LT_GRP);
				break;
			case 'I':
				if (trigger == NULL) {
					trigger = "intensity\n";
				} else {
					trigger = NULL;
				}
				clr_scr = true;
				break;
			case 'R':
				if (trigger == NULL) {
					trigger = "raw\n";
				} else {
					trigger = NULL;
				}
				clr_scr = true;
				break;
			case 'G':
				if (trigger == NULL) {
					trigger = "group\n";
				} else {
					trigger = NULL;
				}
				clr_scr = true;
				break;
			case 'S':
				if (trigger == NULL) {
					trigger = "self raw\n";
				} else {
					trigger = NULL;
				}
				clr_scr = true;
				break;
			case 's':
				buf[0] = 0xf0;
				buf[1] = 0x00;
				buf[2] = 0x65;
				i2c_write(buf, 3);
				break;
			case 'w':
				buf[0] = 0x0;
				i2c_write(buf, 1);
				break;
			case ' ':
				pause = !pause;
				break;
			case 'm':
				printf("set op mode\n");
				printf("\t1 : init\n");
				printf("\t2 : optimize delay\n");
				printf("\t3 : low power mode\n");
				printf("\t4 : normal mode\n");
				buf[0] = 0xB0;
				buf[1] = getchar() - '0';
				i2c_write(buf, 2);
				fflush(stdin);
				break;
			case 'v':
				buf[0] = 0xf0;
				i2c_write(buf, 1);
				i2c_read(buf, 4);
//				printf("irq flag : 0x%08x\n", *(uint32_t *)buf);
				break;
			default:
				if (cin >= '1' || cin <= '9') {
					buf[0] = 0x30 + cin - '1';
					i2c_write(buf, 1);
					printf("\nwrite 0x%08x\n", buf[0]);
				}
				break;
		}
		fflush(stdin);
	}
}

static void master_run(void)
{
	static int row_cnt = 0;
	uint8_t buf[1600];
	uint8_t cmd;
	int intr;
	int i;
	struct {
		uint16_t len;
		uint16_t type;
	} __attribute__ ((packed)) log_info;

	master_packet_sender();

	intr = get_intr_value();

	if (pause)
		return;

	if (intr < 0) {

	} else if (!intr) {
		cmd = 0xf;
		if (i2c_write(&cmd, 1) != 1)
			return;
		if (i2c_read((uint8_t *)&log_info, 4) != 4)
			return;

		cmd = 0x10;
		if (i2c_write(&cmd, 1) != 1)
			return;

#if	0
		if (i2c_read(buf, log_info.len) != log_info.len)
			return;
#else
		if (log_info.len < 248) {
			if (fx2_i2c_read(buf, log_info.len) != log_info.len)
				return;
		} else {
			if (fx2_i2c_read_long(buf, log_info.len) != log_info.len)
				return;
		}
#endif

		if (log_info.type == 0)
		{
			for (i = 0; i < log_info.len; i+= 6) {
				int x, y, str, width;
				int id, state;
				char type;

				id = buf[i + 0] & 0xf;
				state = !!(buf[i + 0] & 0x80);
				type = (buf[i + 0] & 0x40) ? 'H' : 'T';
				x = (buf[i + 1] & 0x0f) << 8 | buf[i + 2];
				y = (buf[i + 1] & 0xf0) << 4 | buf[i + 3];
				str = buf[i + 4];
				width = buf[i + 5];
				printf("[%d %c %d] : %4d, %4d (%3d, %3d)\n", id, type, state, x, y, str, width);
			}
			return;
		}

		switch (log_info.type) {
		case U8:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len; i++) {
				print_log("%5d, ", *((uint8_t *)buf + i));
			}
			print_log("\n");
			break;
		case S8:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len; i++) {
				print_log("%5d, ", *((int8_t *)buf + i));
			}
			print_log("\n");
			break;
		case U16:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len / 2; i++) {
				print_log("%5d, ", *((uint16_t *)buf + i));
			}
			print_log("\n");
			break;
		case S16:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len / 2; i++) {
				print_log("%5d, ", *((int16_t *)buf + i));
			}
			print_log("\n");
			break;
		case U32:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len / 4; i++) {
				print_log("%5d, ", *((uint32_t *)buf + i));
			}
			print_log("\n");
			break;
		case S32:
			print_log("[%2d] : ", row_cnt++);
			for (i = 0; i <log_info.len / 4; i++) {
				print_log("%5d, ", *((int32_t *)buf + i));
			}
			print_log("\n");
			break;
		default:
			if (trigger && !strcmp(trigger, (char *)buf)) {
				gotoxy(0, 0);
			}
			row_cnt = 0;
			if (clr_scr) {
				system("cls");
				clr_scr = false;
			}
			print_log("%s", buf);
			break;
		}
	}

//	usleep(1000);
	Sleep(1);
}

void print_app_message(bool open)
{
	char *msg = open ? "Terminated" : "Started";

	printf(
		"===========================================================================\n"
		"MELFAS Console App %s\n"
		"===========================================================================\n",msg);
}

int StrHexToInt(char *strData)
{
	int i;
	int nHexData, strLen, nValue;

	strLen = (int)strlen(strData);
	nHexData = 0;
	for(i=0; i<strLen; i++) {			
		if(('0' <= strData[i]) && (strData[i] <= '9')) {
			nValue = strData[i] - '0';
		} else if(('A' <= strData[i]) && (strData[i] <= 'F')) {
			nValue = (strData[i] - 'A') + 10;
		} else if(('a' <= strData[i]) && (strData[i] <= 'f')) {
			nValue = (strData[i] - 'a') + 10;
		} else {
			return 0;
		}
		nHexData |= (nValue << (4 * (strLen - 1 - i)));
	}
	return nHexData;
}

int ArgHandler(int argc, char* argv[])
{
	int i;
	char* SlaveId;

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			SlaveId = argv[++i];
			if (strlen(SlaveId) > 2) {
				if (SlaveId[0] == '0' && SlaveId[1] == 'x')
					uSlaveId = (uint8_t)StrHexToInt(&SlaveId[2]);
				else
					uSlaveId = (uint8_t)atoi(SlaveId);
			} else {
				uSlaveId = atoi(SlaveId);
			}
			fx2_set_slave_addr((uint8_t)(uSlaveId & 0xff));
		} else if(!strcmp(argv[i], "-d")) {
			BinFileName = argv[++i];
			bFwDown = true;
		} else if (!strcmp(argv[i], "-dump")) {
			bFwDump = true;
		} else if (!strcmp(argv[i], "-trim")) {
			trim_flag = true;
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (signal(SIGINT, signal_handler) == SIG_ERR ) {
		fprintf(stderr, "failed to register signal handler\n");
		exit(-1);
	}
	log_init();
	print_app_message(OPEN);

	ArgHandler(argc - 1, &argv[1]);
	printf(" >> I2C slave id = 0x%02X\n", uSlaveId);

	if (!(usb_init_fx2() & usb_download_fx2())) {
		fprintf(stderr, "failed to init fx2 interface\n");
		goto out;
	}
	printf("- FX2 Firmware download finished successfully.\n\n");
	fx2_set_vdd(true);

	if (trim_flag)
		Trimming();

	if (bFwDown)
		fwDownload(BinFileName, MAIN);

	if (bFwDump)
		fwDump();

//	fx2_set_vdd(false);
//	fx2_set_vdd(true);

	while (loop)
		master_run();

	log_close();

out:
	fx2_set_vdd(false);
	print_app_message(CLOSE);
	
	return 0;
}

