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
    Title: CDCDeviceDescriptor

    About: Purpose
        Definition of several constants used when declaring USB CDC device
        descriptors.
*/

#ifndef CDCDEVICEDESCRIPTOR_H
#define CDCDEVICEDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: CDC device descriptor values
        CDCDeviceDescriptor_CLASS - Device class code when using the CDC class.
        CDCDeviceDescriptor_SUBCLASS - Device subclass code when using the CDC
            class.
        CDCDeviceDescriptor_PROTOCOL - Device protocol code when using the CDC
            clas.
*/
#define CDCDeviceDescriptor_CLASS               0x02
#define CDCDeviceDescriptor_SUBCLASS            0x00
#define CDCDeviceDescriptor_PROTOCOL            0x00

#endif //#ifndef CDCDEVICEDESCRIPTOR_H

