
#ifndef _ADM8511_H
#define _ADM8511_H

#include <usb/device/core/USBDDriver.h>

/* USB endpoints */
#define ADM8511_DATAIN			1
#define ADM8511_DATAOUT			2
#define ADM8511_INT				3

#define ADM8511_DATAIN_SIZE		64
#define ADM8511_DATAOUT_SIZE	64
#define ADM8511_INT_SIZE		8

/* USB Request */
#define ADM8511_REQUEST_READ	0xF0
#define ADM8511_REQUEST_WRITE	0xF1


void ADM8511_Initialize(void);
void ADM8511_RequestHandler(const USBGenericRequest *request);
void ADM8511_Configured(void);

#endif
