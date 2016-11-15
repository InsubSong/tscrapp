#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>

typedef struct {
	union {
		struct {
			UCHAR Recipient :5;
			UCHAR Type :2;
			UCHAR Direction :1;
		} bmRequest;
		UCHAR bmReq;
	};
} BMREQUEST;

bool usb_bulkRead(uint8_t* buf, int len);
bool usb_bulkWrite(uint8_t* buf, int len);
bool usb_download_fx2(void);
bool usb_init_fx2(void);

#endif

