/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution. 
 * 
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission. 
 * 
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/*
    Title: USBDCallbacks

    About: Purpose
        Definitions of callbacks used by the USB API to notify the user
        application of incoming events. These functions are declared as 'weak',
        so they can be re-implemented elsewhere in the application in a
        transparent way.
*/

#ifndef USBDCALLBACKS_H
#define USBDCALLBACKS_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <usb/common/core/USBGenericRequest.h>

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Functions: USB callbacks
        USBDCallbacks_Initialized - Invoked after the USB driver has been
            initialized. By default, configures the UDP interrupt.
        USBDCallbacks_Reset - Invoked when the USB driver is reset. Does nothing
            by default.
        USBDCallbacks_Suspend - Invoked when the USB device gets suspended. By
            default, turns off all LEDs.
        USBDCallbacks_Resumed - Invoked when the USB device leaves the Suspended
            state. By default, configures the LEDs.
        USBDCallbacks_RequestReceived - Invoked when a new SETUP request is
            received. Does nothing by default.
*/
extern void USBDCallbacks_Initialized(void);
extern void USBDCallbacks_Reset(void);
extern void USBDCallbacks_Suspended(void);
extern void USBDCallbacks_Resumed(void);
extern void USBDCallbacks_RequestReceived(const USBGenericRequest *request);

#endif //#ifndef USBDCALLBACKS_H

