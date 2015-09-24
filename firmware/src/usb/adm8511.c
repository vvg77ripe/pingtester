
#include <config.h>
#include <string.h>
#include <utility/trace.h>

#include <usb/device/core/USBD.h>
#include <usb/device/core/USBDDriver.h>
#include <drivers/ethernet.h>
#include <net/bridge.h>
#include "adm8511.h"


#define ADM8511_MTU					1520

#define ADM_REG_ETHCOLTROL0			0x00
#define ADM_REG_ETHCOLTROL1			0x01
#define ADM_REG_ETHCOLTROL2			0x02
#define ADM_REG_ADDRESS0			0x10
#define ADM_REG_ADDRESS1			0x11
#define ADM_REG_ADDRESS2			0x12
#define ADM_REG_ADDRESS3			0x13
#define ADM_REG_ADDRESS4			0x14
#define ADM_REG_ADDRESS5			0x15
#define ADM_REG_EP1CONTROL			0x1C
#define ADM_REG_PHYAD				0x25
#define ADM_REG_PHYDATA0			0x26
#define ADM_REG_PHYDATA1			0x27
#define ADM_REG_PHYCONTROL			0x28


/* ===== Internal variables ===== */

/* ADM8511 Registers */
static unsigned char regs[256];

/* ADM8511 PHY Registers */
static unsigned short phyregs[32];

/* Driver descriptors */
extern const USBDDriverDescriptors ADM8511Descriptors;

/* Standard device driver instance */
static USBDDriver usbdDriver;

/* Receive data buffer */
static unsigned char ethRecvData[ADM8511_MTU];
static unsigned int ethRecvDataSize;
static pktbuf ethRecvBuffer;

/* Send data buffer + 4byte CRC + 4byte status */
static unsigned char ethSendData[ADM8511_MTU + 8];

/* Interrupt endpoint buffer */
static unsigned char intData[ADM8511_INT_SIZE];


/* ===== Internal functions ===== */

static void admDataHandler(unsigned int unused, unsigned char status,
						   unsigned int received, unsigned int remaining)
{
	ethRecvDataSize += received;

	if (received < 64) {
		do {
			/* Packet size (2) + 2 mac addr (12) */
			if (ethRecvDataSize < 14) break;

			/* Send received packet to bridge */
			ethRecvBuffer.next = NULL;
			ethRecvBuffer.data = &ethRecvData[2];
			ethRecvBuffer.len = ethRecvDataSize - 2;
			briPacketRecv(BRI_IF_USB, &ethRecvBuffer);
		} while(0);

		ethRecvDataSize = 0;
	}

	/* Check for input buffer overflow */
	if ( (ethRecvDataSize + ADM8511_DATAOUT_SIZE) > ADM8511_MTU) {
		ethRecvDataSize = 0;
	}

	/* Start reading next data packet */
	USBD_Read(ADM8511_DATAOUT, &ethRecvData[ethRecvDataSize], ADM8511_DATAOUT_SIZE, (TransferCallback) admDataHandler, 0);
}

static void admIntHandler(unsigned int unused, unsigned char status,
						   unsigned int received, unsigned int remaining)
{
	/* Write next status data */
	USBD_Write(ADM8511_INT, intData, ADM8511_INT_SIZE, (TransferCallback) admIntHandler, 0);
}

static void admRegChanged(unsigned char idx)
{
	unsigned char reg;
	unsigned short phydata;

	reg = regs[idx];

	switch (idx) {
		// PHY access control
		case 0x28:
			// Write to PHY
			if (reg & 0x20) {
			}
			// Read from PHY
			if (reg & 0x40) {
				phydata = phyregs[reg & 0x1F];
				regs[0x26] = phydata;
				regs[0x27] = phydata >> 8;
			}
			// DONE
			regs[idx] |= 0x80;
			break;
	}
}

static void admRegWriteHandler(unsigned int wIndex, unsigned char status,
						   unsigned int received, unsigned int remaining)
{
	int i;

	for (i = 0; i < received; i++) {
		admRegChanged(wIndex + i);
	}

	/* Ack */
	USBD_Write(0, 0, 0, 0, 0);
}

static void admSendPacket(pktbuf *packet)
{
	unsigned char *data;
	unsigned short size;
	pktbuf *buf;

	data = ethSendData;
	size = 0;

	/* Copy packet to send buffer */
	for (buf = packet; buf; buf = buf->next) {
		/* Check buffer overflow */
		if ( (size + buf->len) > ADM8511_MTU ) return;

		/* Copy data */
		memcpy(data, buf->data, buf->len);
		
		data += buf->len;
		size += buf->len;
	}

	/* Packet CRC */
	*data++ = 0;
	*data++ = 0;
	*data++ = 0;
	*data++ = 0;
	size += 4;

	/* Packet trailer */
	*data++ = size;
	*data++ = size >> 8;
	*data++ = 0;
	*data++ = 0;
	size += 4;

	/* Send packet */
	USBD_Write(ADM8511_DATAIN, ethSendData, size, 0, 0);
}

/* ===== Exported functions ===== */

void ADM8511_Initialize()
{
	/* Initialize variables */
	memset(regs, 0, sizeof(regs));
	memset(phyregs, 0, sizeof(phyregs));

	/* Default PHY register values */
	phyregs[0] = 0x1100;
	phyregs[1] = 0x782C;

	/* Assign MAC address */
	regs[ADM_REG_ADDRESS0] = 0x00;
	regs[ADM_REG_ADDRESS1] = 0x52;
	regs[ADM_REG_ADDRESS2] = 0x42;
	regs[ADM_REG_ADDRESS3] = 0x03;
	regs[ADM_REG_ADDRESS4] = 0x00;
	regs[ADM_REG_ADDRESS5] = 0x01;

	/* Register interface */
	briIfRegister(BRI_IF_USB, "USB Interface", admSendPacket, &regs[ADM_REG_ADDRESS0]);

	/* Init the USB driver */
	USBDDriver_Initialize(&usbdDriver, &ADM8511Descriptors, 0);
	USBD_Init();
}

void ADM8511_RequestHandler(const USBGenericRequest *request)
{
	switch (USBGenericRequest_GetRequest(request)) {
		case ADM8511_REQUEST_READ:
			USBD_Write(0, &regs[request->wIndex & 0xFF], request->wLength, NULL, 0);
			break;

		case ADM8511_REQUEST_WRITE:
		case 0x0FE:
			if (request->wLength > 1) {
				USBD_Read(0, &regs[request->wIndex & 0xFF], request->wLength,
					(TransferCallback) admRegWriteHandler, (void *) request->wIndex);
			} else {
				/* Ack */
				USBD_Write(0, 0, 0, 0, 0);
			}
			break;

		default:
			USBDDriver_RequestHandler(&usbdDriver, request);
	}
}

void ADM8511_Configured()
{
	/* Start reading data packets */
	ethRecvDataSize = 0;
	USBD_Read(ADM8511_DATAOUT, ethRecvData, ADM8511_DATAOUT_SIZE, (TransferCallback) admDataHandler, 0);

	/* Write status data */
	USBD_Write(ADM8511_INT, intData, ADM8511_INT_SIZE, (TransferCallback) admIntHandler, 0);
}
