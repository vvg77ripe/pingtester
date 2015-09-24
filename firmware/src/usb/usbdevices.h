
#ifndef _USBDEVICES_H
#define _USBDEVICES_H

#include <usb/device/core/USBD.h>
#include <usb/device/core/USBDDriverCallbacks.h>
#include <usb/CDCDSerialDriver.h>
#include <usb/CDCDSerialDriverDescriptors.h>
#include <usb/MSDDriver.h>
#include <usb/MSDLun.h>


#define USB_DEVICE_NONE				0
#define USB_DEVICE_CARDREADER		1
#define USB_DEVICE_DEBUGPORT		2
#define USB_DEVICE_ADM8511			3


void usbInit(void);
void usbShutdown(void);
void usbStart(void);
void usbStop(void);
void usbPoll(void);

#endif
