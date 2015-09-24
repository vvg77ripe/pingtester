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
    Title: CDCCommunicationInterfaceDescriptor

    About: Purpose
        Definition of several constants used when declaring a CDC communication
        class interface descriptor.
*/

#ifndef CDCCOMMUNICATIONINTERFACEDESCRIPTOR_H
#define CDCCOMMUNICATIONINTERFACEDESCRIPTOR_H

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/*
    Constants: Communication interface values
        CDCCommunicationInterfaceDescriptor_CLASS - Interface class code for
            a CDC communication class interface.
        CDCCommunicationInterfaceDescriptor_ABSTRACTCONTROLMODEL - Interface
            subclass code for an Abstract Control Model interface descriptor.
        CDCCommunicationInterfaceDescriptor_NOPROTOCOL - Interface protocol code
            when a CDC communication interface does not implemenent any particular
            protocol.
*/
#define CDCCommunicationInterfaceDescriptor_CLASS                   0x02
#define CDCCommunicationInterfaceDescriptor_ABSTRACTCONTROLMODEL    0x02
#define CDCCommunicationInterfaceDescriptor_NOPROTOCOL              0x00

#endif //#ifndef CDCCOMMUNICATIONINTERFACEDESCRIPTOR_H

