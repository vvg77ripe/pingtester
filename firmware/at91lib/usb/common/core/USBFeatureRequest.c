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
    Title: USBFeatureRequest implementation

    About: Purpose
        Implementation of the USBFeatureRequest class.
*/

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "USBFeatureRequest.h"

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------
/*
    Function: USBFeatureRequest_GetFeatureSelector
        Returns the feature selector of a given CLEAR_FEATURE or SET_FEATURE
        request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Feature selector.
*/
unsigned char USBFeatureRequest_GetFeatureSelector(
    const USBGenericRequest *request)
{
    return USBGenericRequest_GetValue(request);
}

/*
    Function: USBFeatureRequest_GetTestSelector
        Indicates the test that the device must undertake following a
        SET_FEATURE request.

    Parameters:
        request - Pointer to a USBGenericRequest instance.

    Returns:
        Test selector.
*/
unsigned char USBFeatureRequest_GetTestSelector(
    const USBGenericRequest *request)
{
    return (USBGenericRequest_GetIndex(request) >> 8) & 0xFF;
}

