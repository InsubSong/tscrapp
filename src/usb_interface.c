#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "usb_interface.h"
#include "cyioctl.h"

struct intel_hex {
	uint8_t Length;
	uint16_t Address;
	uint8_t Type;
	uint8_t Data[16];
};

#include "fx2_firmware.c"

#define HEX_RECORD_LEN_MAX	16

#define _DEFINE_GUID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

_DEFINE_GUID(GUID_CLASS_MELFAS, 0x9DD5531D, 0x8D85, 0x4412, 0x93, 0x8B, 0xFD,
			 0x44, 0x8E, 0xAA, 0x25, 0xE3);

static char szCompleteDeviceName[256];
HANDLE hDevice = NULL;

static HANDLE OpenOneDevice(IN HDEVINFO HardwareDeviceInfo,
					 IN PSP_INTERFACE_DEVICE_DATA DeviceInfoData, IN PCHAR pszDevName)
{
	PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
	ULONG predictedLength = 0;
	ULONG requiredLength = 0;
	HANDLE hOut = INVALID_HANDLE_VALUE;

	SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo, DeviceInfoData, NULL, 0, &requiredLength, NULL);

	predictedLength = requiredLength;
	functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc( predictedLength);
	functionClassDeviceData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

	if (!SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo, DeviceInfoData, functionClassDeviceData, predictedLength, &requiredLength, NULL))
	{
		free(functionClassDeviceData);
		return INVALID_HANDLE_VALUE;
	}

	strcpy(pszDevName, functionClassDeviceData->DevicePath);

	hOut = CreateFile(functionClassDeviceData->DevicePath,
					  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
					  NULL,
					  OPEN_EXISTING, 
					  0,
					  NULL);

	free(functionClassDeviceData);
	return hOut;
}

static HANDLE OpenUsbDevice(LPGUID pGuid, PCHAR pszName)
{
	ULONG NumberDevices;
	HANDLE hOut = INVALID_HANDLE_VALUE;
	HDEVINFO hardwareDeviceInfo;
	SP_INTERFACE_DEVICE_DATA deviceInfoData;
	ULONG i;
	BOOLEAN done;
	PUSB_DEVICE_DESCRIPTOR usbDeviceInst;
	PUSB_DEVICE_DESCRIPTOR *UsbDevices = &usbDeviceInst;

	*UsbDevices = NULL;
	NumberDevices = 0;

	hardwareDeviceInfo = SetupDiGetClassDevs(pGuid, NULL,
						 NULL,
						 (DIGCF_PRESENT |
						  DIGCF_INTERFACEDEVICE));

	NumberDevices = 4;
	done = false;
	deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	i = 0;
	while (!done) {
		NumberDevices *= 2;

		if (*UsbDevices) {
			*UsbDevices = (PUSB_DEVICE_DESCRIPTOR) realloc(*UsbDevices,
						  (NumberDevices * sizeof(USB_DEVICE_DESCRIPTOR)));
		} else {
			*UsbDevices = (PUSB_DEVICE_DESCRIPTOR) calloc(NumberDevices,
						  sizeof(USB_DEVICE_DESCRIPTOR));
		}

		if (NULL == *UsbDevices) {
			SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
			return INVALID_HANDLE_VALUE;
		}

		usbDeviceInst = *UsbDevices + i;

		for (; i < NumberDevices; i++) {
			if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0, pGuid, i, &deviceInfoData)) {

				hOut = OpenOneDevice(hardwareDeviceInfo, &deviceInfoData,
									 pszName);
				if (hOut != INVALID_HANDLE_VALUE) {
					done = true;
					break;
				}
			} else {
				if (ERROR_NO_MORE_ITEMS == GetLastError()) {
					done = true;
					break;
				}
			}
		}
	}

	NumberDevices = i;

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	free(*UsbDevices);

	return hOut;
}

static bool GetUsbDeviceFileName(LPGUID pGuid, PCHAR pszName)
{
	HANDLE hDev = OpenUsbDevice(pGuid, pszName);

	if (hDev != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDev);
		return true;
	}

	return false;
}

static bool usb_open_fx2(void)
{
	IN PCHAR pcPipeName = NULL;

	if (!GetUsbDeviceFileName((LPGUID) &GUID_CLASS_MELFAS,
							  szCompleteDeviceName))
	{
		printf("- No proper device is connected\n"); //INVALID_HANDLE_VALUE;
		return false;
	}

	if (pcPipeName)
	{
		strcat(szCompleteDeviceName, "\\");
		strcat(szCompleteDeviceName, pcPipeName);
		//printf(" szCompleteDeviceName : %s\n", szCompleteDeviceName );
	}
	//printf(" szCompleteDeviceName : %s\n", szCompleteDeviceName );

	hDevice = CreateFile(szCompleteDeviceName, GENERIC_WRITE | GENERIC_READ,
						 FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (GetLastError() == 0)
	{
		//bUsbIsOpened = true;
	}
	else
	{
		//bUsbIsOpened = false;
		return false;
	}

	return true;
}

bool usb_select_interface(void)
{
	DWORD dwBytes;
	UCHAR alt = 1;

	DWORD result;

	if (DeviceIoControl(hDevice, IOCTL_ADAPT_SELECT_INTERFACE, &alt,
						sizeof(alt), &alt, sizeof(alt), &dwBytes, NULL)
	   )
	{
		//printf("IOCTL_ADAPT_SELECT_INTERFACE OK\n");
	}
	else
	{
		result = GetLastError();
		printf("USB_ERROR       : IOCTL_ADAPT_SELECT_INTERFACE [%ld]\n",
			   result);
		return false;

	}

	return true;
}

BOOL usb_reset_pipe(void)
{

	DWORD dwBytes;
	UCHAR Address = 0x02;

	DWORD result;

	if (DeviceIoControl(hDevice, IOCTL_ADAPT_RESET_PIPE, &Address,
						sizeof(Address), NULL, 0, &dwBytes, NULL)

	   )
	{

		//printf("IOCTL_ADAPT_RESET_PIPE OK\n");

	}
	else
	{

		result = GetLastError();
		printf("USB_ERROR       : IOCTL_ADAPT_RESET_PIPE [%ld]\n", result);
		return FALSE;

	}

	return TRUE;
}

static bool usb_reset_fx2(uint8_t ucValue)
{
	DWORD result;
	DWORD dwReturnBytes;

	int nTotalSize;

	UCHAR *pucData;
	BMREQUEST sRquest;
	PSINGLE_TRANSFER pTransfer;
	UCHAR *pEntireBuf = NULL;

	nTotalSize = sizeof(SINGLE_TRANSFER) + 1;

	pEntireBuf = (UCHAR *) malloc(nTotalSize);
	memset(pEntireBuf, 0x00, nTotalSize);

	pTransfer = (PSINGLE_TRANSFER) pEntireBuf; 
	pucData = pEntireBuf + sizeof(SINGLE_TRANSFER);

	//--------------------------------
	// Input Packet Information
	//--------------------------------
	sRquest.bmRequest.Recipient = 0;
	sRquest.bmRequest.Type = 2;
	sRquest.bmRequest.Direction = 0;

	nTotalSize = sizeof(SINGLE_TRANSFER) + 1;

	pTransfer->SetupPacket.bmRequest = sRquest.bmReq;
	pTransfer->SetupPacket.bRequest = 0xA0;
	pTransfer->SetupPacket.wValue = 0xE600;
	pTransfer->SetupPacket.wIndex = 0x00;
	pTransfer->SetupPacket.wLength = 0x01;

	pTransfer->ucEndpointAddress = 0x00;
	pTransfer->IsoPacketLength = 0;
	pTransfer->BufferOffset = sizeof(SINGLE_TRANSFER);
	pTransfer->BufferLength = 0x01;

	pucData[0] = (0x01 & ucValue);

	if (DeviceIoControl(hDevice, IOCTL_ADAPT_SEND_EP0_CONTROL_TRANSFER,
						pEntireBuf, nTotalSize, pEntireBuf, nTotalSize, &dwReturnBytes,
						NULL)

	   )
	{
		//printf( " dwReturnBytes : %ld\n",dwReturnBytes );
		//printf(" FX2 Reset(%d) OK\n", ucValue);
	}
	else
	{
		free(pEntireBuf);
		result = GetLastError();
		printf(" USB_ERROR       : FX2 Reset(%d) [%ld]\n", ucValue, result);
		return FALSE;
	}

	free(pEntireBuf);

	return TRUE;
}

bool usb_init_fx2(void)
{
	bool ret;

	ret = usb_open_fx2();
	if (!ret)
		goto out;

	ret = usb_select_interface();
	if (!ret)
		goto out;

	ret = usb_reset_pipe();
	if (!ret)
		goto out;
out:

	return ret;
}

bool usb_download_fx2(void)
{
	int i;
	DWORD result;
	DWORD dwReturnBytes;
	int nTotalSize;
	struct intel_hex *_pHexRecord = firmware_fx2;

	UCHAR *pucData;
	BMREQUEST sRquest;
	PSINGLE_TRANSFER pTransfer;
	UCHAR *pEntireBuf = NULL;

	nTotalSize = sizeof(SINGLE_TRANSFER) + HEX_RECORD_LEN_MAX;

	pEntireBuf = (UCHAR *) malloc(nTotalSize);
	memset(pEntireBuf, 0x00, nTotalSize);

	pTransfer = (PSINGLE_TRANSFER) pEntireBuf;
	pucData = pEntireBuf + sizeof(SINGLE_TRANSFER);

	if (!usb_reset_fx2(1))
	{
		free(pEntireBuf);
		result = GetLastError();
		printf("USB_ERROR       : usb_reset_fx2 [%ld]\n", result);
		return FALSE;
	}

	//-------------------------------
	//	Start Download
	//-------------------------------
	if (_pHexRecord->Type == 1)
	{
		free(pEntireBuf);
		printf(" Error : No Hex data.\n");
		return FALSE;
	}

	do
	{
		nTotalSize = sizeof(SINGLE_TRANSFER) + _pHexRecord->Length;
		//--------------------------------
		// Input Packet Information
		//--------------------------------
		sRquest.bmRequest.Recipient = 0; // Device
		sRquest.bmRequest.Type = 2; // Vendor
		sRquest.bmRequest.Direction = 0; // IN command (from Device to Host)

		pTransfer->SetupPacket.bmRequest = sRquest.bmReq;
		pTransfer->SetupPacket.bRequest = 0xA0; // ReqCode;
		pTransfer->SetupPacket.wValue = _pHexRecord->Address; // Starting Address
		pTransfer->SetupPacket.wIndex = 0x00; // 0x00
		pTransfer->SetupPacket.wLength = _pHexRecord->Length; // Length
		pTransfer->ucEndpointAddress = 0x00; // Control pipe
		pTransfer->IsoPacketLength = 0;
		pTransfer->BufferOffset = sizeof(SINGLE_TRANSFER);
		pTransfer->BufferLength = _pHexRecord->Length; //nTotalSize;

		for (i = 0; i < _pHexRecord->Length; i++)
		{
			pucData[i] = _pHexRecord->Data[i];
		}

		if (DeviceIoControl(hDevice, IOCTL_ADAPT_SEND_EP0_CONTROL_TRANSFER,
							pEntireBuf, nTotalSize, pEntireBuf, nTotalSize, &dwReturnBytes,
							NULL)

		   )
		{
			//printf( "dwReturnBytes : %ld\n",dwReturnBytes );
			//apputil_print_binary( pucData, _pHexRecord->Length );
			//printf("FX2_DOWNLOAD_HEX_RECORD OK\n");
		}
		else
		{

			free(pEntireBuf);
			result = GetLastError();
			printf("USB_ERROR       : FX2_DOWNLOAD_HEX_RECORD [%ld]\n", result);
			return FALSE;

		}

		_pHexRecord++;

	}
	while (_pHexRecord->Type != 1);

	free(pEntireBuf);

	// unreset fx2
	if (usb_reset_fx2(0) == false)
	{
		result = GetLastError();
		printf("USB_ERROR       : usb_reset_fx2 [%ld]\n", result);
		return FALSE;
	}

	return TRUE;
}

bool usb_bulkWrite(uint8_t* buf, int len)
{
	int iXmitBufSize = sizeof(SINGLE_TRANSFER);
	PUCHAR pXmitBuf = (PUCHAR) malloc(iXmitBufSize);
	PSINGLE_TRANSFER pTransfer;

	DWORD dwReturnBytes;
	DWORD result;

	memset(pXmitBuf, 0x00, iXmitBufSize);

	pTransfer = (PSINGLE_TRANSFER) pXmitBuf;
	pTransfer->ucEndpointAddress = 2;
	pTransfer->IsoPacketLength = 0;
	pTransfer->BufferOffset = 0;
	pTransfer->BufferLength = 0;

	if (DeviceIoControl(hDevice, IOCTL_ADAPT_SEND_NON_EP0_DIRECT, pXmitBuf,
						iXmitBufSize, buf, len, &dwReturnBytes, NULL))
	{
		//printf( "USB BULK Wite OK.  returnBytes [%ld]\n", dwReturnBytes);
	}
	else
	{
		result = GetLastError();

		printf(" dwReturnBytes : %ld\n", dwReturnBytes);
		printf(" USB BULK Wite Fail [%ld]\n", result);
		return true;
	}

	return false;
}

//------------------------------------
//
//------------------------------------
bool usb_bulkRead(uint8_t* buf, int len)
{
	int iXmitBufSize = sizeof(SINGLE_TRANSFER);
	PUCHAR pXmitBuf = (PUCHAR) malloc(iXmitBufSize);
	PSINGLE_TRANSFER pTransfer;

	DWORD dwReturnBytes;
	DWORD result;

	memset(pXmitBuf, 0x00, iXmitBufSize); 

	pTransfer = (PSINGLE_TRANSFER) pXmitBuf;
	pTransfer->ucEndpointAddress = 0x86; 
	pTransfer->IsoPacketLength = 0;
	pTransfer->BufferOffset = 0;
	pTransfer->BufferLength = 0;

	if (DeviceIoControl(hDevice, IOCTL_ADAPT_SEND_NON_EP0_DIRECT, pXmitBuf,
						iXmitBufSize, buf, len, &dwReturnBytes, NULL))
	{
		//printf( "USB BULK Wite OK.  returnBytes [%ld]\n", dwReturnBytes);
	}
	else
	{
		result = GetLastError();
		printf(" dwReturnBytes : %ld\n", dwReturnBytes);
		printf(" USB BULK Wite Fail [%ld]\n", result);
		return true;
	}

	return false;
}

