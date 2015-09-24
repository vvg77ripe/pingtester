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
    Title: CDCCallManagementDescriptor

    About: Purpose
        Definition of a class for managing CDC call management descriptors.

    About: Usage
        Should be included in a list of configuration descriptors for a USB
        device.
*/

#ifndef CDCCALLMANAGEMENTDESCRIPTOR_H
#define CDCCALLMANAGEMENTDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Capabilities
        CDCCallManagementDescriptor_SELFCALLMANAGEMENT - Device handles call
            management itself.
        CDCCallManagementDescriptor_DATACALLMANAGEMENT - Device can exchange
            call management information over a Data class interface.
*/
#define CDCCallManagementDescriptor_SELFCALLMANAGEMENT      (1 << 0)
#define CDCCallManagementDescriptor_DATACALLMANAGEMENT      (1 << 1)

//------------------------------------------------------------------------------
//         Types
//------------------------------------------------------------------------------

#ifdef __ICCARM__          // IAR
#pragma pack(1)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR

/*
    Type: CDCCallManagementDescriptor
        Describes the processing of calls for the communication class interface.

    Variables:
        bFunctionLength - Size of this descriptor in bytes.
        bDescriptorType - Descriptor type (<CDCDescriptors_INTERFACE>).
        bDescriptorSubtype - Descriptor sub-type (<CDCDescriptors_CALLMANAGEMENT>).
        bmCapabilities - Configuration capabilities (see <Capabilities).
        bDataInterface - Interface number of the data class interface used for
            call management (optional).
*/
typedef struct {

    unsigned char bFunctionLength;
    unsigned char bDescriptorType;
    unsigned char bDescriptorSubtype;
    unsigned char bmCapabilities;
    unsigned char bDataInterface;

} __attribute__ ((packed)) CDCCallManagementDescriptor; // GCC

#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

#endif //#ifndef CDCCALLMANAGEMENTDESCRIPTOR_H

