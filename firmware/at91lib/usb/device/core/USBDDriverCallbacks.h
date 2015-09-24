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
    Title: USBDDriverCallbacks

    About: Purpose
        Definition of several callbacks which are triggered by the USB software
        driver after receiving specific requests.

    About: Usage
        1 - Re-implement the <USBDDriverCallbacks_ConfigurationChanged>
            callback to know when the hosts changes the active configuration of
            the device.
        2 - Re-implement the <USBDDriverCallbacks_InterfaceSettingChanged>
            callback to get notified whenever the active setting of an interface
            is changed by the host.
*/

#ifndef USBDDRIVERCALLBACKS_H
#define USBDDRIVERCALLBACKS_H

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Functions: USB driver callbacks
        USBDDriverCallbacks_ConfigurationChanged - Indicates that the current
            configuration of the device has changed.
        USBDDriverCallbacks_InterfaceSettingChanged - Notifies of a change in
            the currently active setting of an interface.
*/
extern void USBDDriverCallbacks_ConfigurationChanged(unsigned char cfgnum);
extern void USBDDriverCallbacks_InterfaceSettingChanged(unsigned char interface,
                                                        unsigned char setting);

#endif //#ifndef USBDDRIVERCALLBACKS_H

