
#include <config.h>
#include <string.h>
#include <utility/trace.h>

#include <drivers/sdcard.h>
#include <drivers/ethernet.h>

#include <usb/adm8511.h>
#include "usbdevices.h"


/// Size in bytes of the buffer used for reading data from the USB & USART
#define DATABUFFERSIZE \
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

#define USB_PIN_VBUS	(1 << 6)
#define USB_PIN_PULLUP	(1 << 7)

/* ===== Variables ===== */

static int devtype = 0;
static int configured = 0;

// Card Reader
static MSDLun cardlun;
static unsigned char cardbuffer[CARD_BLOCK_SIZE];

// Debug Port
static unsigned char usbBuffer[DATABUFFERSIZE];
static char termBuffer[DATABUFFERSIZE];

/* ===== Debug port ===== */

static void dport_recv(unsigned int unused,
                            unsigned char status,
                            unsigned int received,
                            unsigned int remaining)
{
	unsigned char *buf;
	sdcard * sd;

	// Check that data has been received successfully
	if (status == USBD_STATUS_SUCCESS) {

		termBuffer[0] = 0;

		// Terminal command
		switch (usbBuffer[0]) {
			case 't':
				sprintf(termBuffer, "Terminal Test OK\r\n");
				break;

			case 'e':
				sprintf(termBuffer, "0 = %X, 16 = %X, 17 = %X, 28 = %X\r\n",
					EthPHYRead(0), EthPHYRead(16), EthPHYRead(17), EthPHYRead(28));
				break;

			case 's':
				sprintf(termBuffer, "FrOK %i, FCSE %i, ALE %i, RRE %i, ROVR %i, RSE %i\r\n",
					AT91C_BASE_EMAC->EMAC_FRO,
					AT91C_BASE_EMAC->EMAC_FCSE,
					AT91C_BASE_EMAC->EMAC_ALE,
					AT91C_BASE_EMAC->EMAC_RRE,
					AT91C_BASE_EMAC->EMAC_ROV,
					AT91C_BASE_EMAC->EMAC_RSE);
				break;

			case 'c':
				EthPHYWrite(26, 0x8000);
				sprintf(termBuffer, "VCT Started\r\n");
				break;
			case 'r':
				sprintf(termBuffer, "VCT Results: %X %X\r\n", EthPHYRead(26), EthPHYRead(27));
				break;
			case 'R':
				EthInit();
				break;

			case 'm':
				if (sdcCardActivate()) {
					sd = sdcCardInfo();
					sprintf(termBuffer, "NAME = '%c%c%c%c%c' SIZE = %uK SDHC = %i\r\n",
						sd->cid[3], sd->cid[4], sd->cid[5], sd->cid[6], sd->cid[7],
						sd->size, sd->hc);
				} else {
					sprintf(termBuffer, "No SD Card detected\r\n");
				}
				break;
		}

		// Write terminal data
		CDCDSerialDriver_Write(termBuffer, strlen(termBuffer), 0, 0);

		// Start receiving next data
		CDCDSerialDriver_Read(usbBuffer, DATABUFFERSIZE, (TransferCallback) dport_recv, 0);
	}
}

/* ===== Private functions ===== */

static int usbInitDevice(int type)
{
	sdcard * card;
	Media *cardmedia;

	devtype = type;
	configured = 0;

	switch (devtype) {
		case USB_DEVICE_CARDREADER:
			/* Initialize SD card */
			sdcCardActivate();

			/* Check for sd card */
			card = sdcCardInfo();
			if (card->state != CARD_STATE_READY) return 0;

			/* Initialize media */
			cardmedia = sdcGetMedia();
			/* Initialize LUN */
			LUN_Init(&cardlun, cardmedia, cardbuffer, 0, cardmedia->size, CARD_BLOCK_SIZE);
			// Driver initialization
			MSDDriver_Initialize(&cardlun, 1);
			return 1;

		case USB_DEVICE_DEBUGPORT:
			// Driver initialization
			CDCDSerialDriver_Initialize();
			return 1;
			
		case USB_DEVICE_ADM8511:
			ADM8511_Initialize();
			return 1;
	}

	return 0;
}

/* ===== Callbacks ===== */

void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
	switch (devtype) {
		case USB_DEVICE_CARDREADER:
			MSDDriver_RequestHandler(request);
			return;

		case USB_DEVICE_DEBUGPORT:
			CDCDSerialDriver_RequestHandler(request);
			return;
			
		case USB_DEVICE_ADM8511:
			ADM8511_RequestHandler(request);
			return;
	}
}

void USBDDriverCallbacks_ConfigurationChanged(unsigned char cfgnum)
{
	trace_LOG(trace_WARNING, "Configuration %i", cfgnum);

	switch (devtype) {
		case USB_DEVICE_CARDREADER:
			//if (cfgnum > 0) MSDDriver_Reset();
			return;
	}
}

/* ===== Exported functions ===== */

void usbInit()
{
	/* Configure pins */
	AT91C_BASE_PIOA->PIO_ODR = USB_PIN_VBUS;
	AT91C_BASE_PIOA->PIO_OER = USB_PIN_PULLUP;
	AT91C_BASE_PIOA->PIO_PER = USB_PIN_VBUS | USB_PIN_PULLUP;

	/* Disable pullup */
	AT91C_BASE_PIOA->PIO_SODR = USB_PIN_PULLUP;

	usbStart();
}

void usbStart()
{
	if (usbInitDevice(USB_DEVICE_CARDREADER)) {
		/* Enable pullup */
		AT91C_BASE_PIOA->PIO_CODR = USB_PIN_PULLUP;
	}
}

void usbStop()
{
	devtype = 0;

	/* Disable pullup */
	AT91C_BASE_PIOA->PIO_SODR = USB_PIN_PULLUP;
}

void usbShutdown()
{
	usbStop();

	/* Disable USB transceiver */
	AT91C_BASE_UDP->UDP_TXVC = AT91C_UDP_TXVDIS;

	/* Disable USB clock */
	AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_UDP;
}

void usbPoll()
{
	if (USBD_GetState() < USBD_STATE_CONFIGURED) return;

	switch (devtype) {
		case USB_DEVICE_CARDREADER:
			MSDDriver_StateMachine();
			break;

		case USB_DEVICE_DEBUGPORT:
			if (!configured) {
				// Start receiving data on the USB
				CDCDSerialDriver_Read(usbBuffer, DATABUFFERSIZE, (TransferCallback) dport_recv, 0);
			}
			break;

		case USB_DEVICE_ADM8511:
			if (!configured) ADM8511_Configured();
			break;
	}

	configured = 1;
}
