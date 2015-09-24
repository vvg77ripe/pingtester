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

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <config.h>
#include <string.h>

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Copies data from a source buffer into a destination buffer. The two buffers
/// must NOT overlap. Returns the destination buffer.
/// \param pDestination  Destination buffer.
/// \param pSource  Source buffer.
/// \param num  Number of bytes to copy.
//------------------------------------------------------------------------------
FASTCODE void * memcpy(void *pDestination, const void *pSource, size_t num)
{
    unsigned char *pByteDestination;
    unsigned char *pByteSource;
    unsigned int *pAlignedSource = (unsigned int *) pSource;
    unsigned int *pAlignedDestination = (unsigned int *) pDestination;

    // If num is more than 4 bytes, and both dest. and source are aligned,
    // then copy dwords
    if ((((unsigned int) pAlignedDestination & 0x3) == 0)
        && (((unsigned int) pAlignedSource & 0x3) == 0)
        && (num > 4)) {

        while (num > 4) {

            *pAlignedDestination++ = *pAlignedSource++;
            num -= 4;
        }
    }

    // Copy remaining bytes
    pByteDestination = (unsigned char *) pAlignedDestination;
    pByteSource = (unsigned char *) pAlignedSource;
    while (num--) {

        *pByteDestination++ = *pByteSource++;
    }

    return pDestination;
}

//------------------------------------------------------------------------------
/// Fills a memory region with the given value. Returns a pointer to the
/// memory region.
/// \param ptr  Pointer to the start of the memory region to fill.
/// \param value  Value to fill the region with.
/// \param num  Size to fill in bytes.
//------------------------------------------------------------------------------
FASTCODE void * memset(void *ptr, int value, size_t num)
{
    unsigned char *pByteDestination;
    unsigned int *pAlignedDestination = (unsigned int *) ptr;
    unsigned int alignedValue = (value << 24) | (value << 16) | (value << 8) | value;

    // Set words if possible
    if ((((unsigned int) pAlignedDestination & 0x3) == 0) && (num > 4)) {

        while (num > 4) {

            *pAlignedDestination++ = alignedValue;
            num -= 4;
        }
    }

    // Set remaining bytes
    pByteDestination = (unsigned char *) pAlignedDestination;
    while (num--) {

        *pByteDestination++ = value;
    }

    return ptr;
}

