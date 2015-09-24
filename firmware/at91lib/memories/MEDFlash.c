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
//      Includes
//------------------------------------------------------------------------------

#include "MEDFlash.h"
#include <utility/trace.h>

#if defined(AT91C_BASE_EFC)

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Returns a pointer to the physical flash interface
//! \param  media Pointer to a Media instance
//! \return Pointer to the physical flash interface used by the media
//! \see    Media
//------------------------------------------------------------------------------
static AT91S_EFC * FLA_GetInterface(Media *media)
{
    return (AT91S_EFC *) (media->interface);
}

//------------------------------------------------------------------------------
//! \brief  Writes one page of data on a flash memory
//! \param  media Pointer to a Media instance
//! \see    Media
//------------------------------------------------------------------------------
static void FLA_WritePage(Media *media)
{
    unsigned int i;
    unsigned int offset;
    unsigned int page;
    unsigned int length;
    unsigned int remaining;
    unsigned int *source;
    unsigned int *dest;
    AT91S_EFC *flash = FLA_GetInterface(media);

    // Compute function parameters
    // Source address
    source = (unsigned int *) media->transfer.data;

    // Destination address
    dest = (unsigned int *) media->transfer.address;

    // Page number and offset
    offset = media->transfer.address - (unsigned int) AT91C_IFLASH;
    page = offset / AT91C_IFLASH_PAGE_SIZE;
    offset = offset%AT91C_IFLASH_PAGE_SIZE;

    // Length of the data to write
    length = AT91C_IFLASH_PAGE_SIZE - offset;
    if (length > media->transfer.length) {

        length = media->transfer.length;
    }

    // Remaining bytes
    remaining = AT91C_IFLASH_PAGE_SIZE - offset - length;

    // Recopy the data which is not rewritten
    i = 0;
    while (i < offset) {

        *dest = *dest;

        dest++;
        i += 4;
    }

    // Copy the data to write
    i = 0;
    while (i < length) {

      *dest = *source;

      dest++;
      source++;
      i += 4;
    }

    // Recopy the data which is not rewritten
    i = 0;
    while (i < remaining) {

        *dest = *dest;

        dest++;
        i += 4;
    }

    // Update the transfer descriptor
    media->transfer.data = source;
    media->transfer.address = (unsigned int) dest;
    media->transfer.length -= length;

    // Perform the write operation
    flash->EFC_FCR |= (0x5A << 24) //AT91C_MC_CORRECT_KEY
                     | AT91C_MC_FCMD_START_PROG
                     | (page << 8);

    // Enable interrupt on MC_FRDY (end of operation)
    flash->EFC_FMR |= AT91C_MC_FRDY;
}

//------------------------------------------------------------------------------
//! \brief  Indicates if the interrupt for the specified flash media is pending
//! \param  media Pointer to the Media instance used
//! \return 1 if interrupt is pending, 0 otherwise
//------------------------------------------------------------------------------
static unsigned char FLA_Pending(Media *media)
{
    if ((AT91C_BASE_AIC->AIC_IPR & (1 << AT91C_ID_SYS)) != 0) {

        return 1;
    }
    else {

        return 0;
    }
}

//------------------------------------------------------------------------------
//! \brief  Interrupt handler for the internal flash
//!
//!         Completes or continues a pending transfer
//! \param  media Pointer to a Media instance
//! \return 1 if an interrupt was pending and has been treated, 0 otherwise
//! \see    Media
//------------------------------------------------------------------------------
static void FLA_Handler(Media *media)
{
    AT91S_EFC *flash = FLA_GetInterface(media);
    
    // Check if interrupt is pending
    if (!FLA_Pending(media)) {

        return;
    }

    // Disable the FRDY interrupt
    flash->EFC_FMR &= ~AT91C_MC_FRDY;

    // Check if the transfer is finished or not
    if (media->transfer.length == 0) {

        // End of transfer
        // Put the media in Ready state
        media->state = MED_STATE_READY;

        // Invoke the callback if it exists
        if (media->transfer.callback != 0) {

            media->transfer.callback(media->transfer.argument, 0, 0, 0);
        }
    }
    else {

        // Continue the transfer
        FLA_WritePage(media);
    }
}

//------------------------------------------------------------------------------
//! \brief  Reads a specified amount of data from a flash memory
//! \param  media    Pointer to a Media instance
//! \param  address  Address of the data to read
//! \param  data     Pointer to the buffer in which to store the retrieved
//!                   data
//! \param  length   Length of the buffer
//! \param  callback Optional pointer to a callback function to invoke when
//!                   the operation is finished
//! \param  argument Optional pointer to an argument for the callback
//! \return Operation result code
//------------------------------------------------------------------------------
static unsigned char FLA_Read(Media      *media,
                               unsigned int address,
                               void         *data,
                               unsigned int length,
                               MediaCallback   callback,
                               void         *argument)
{
    unsigned char *source = (unsigned char *) address;
    unsigned char *dest = (unsigned char *) data;

    // Check that the media is ready
    if (media->state != MED_STATE_READY) {

        trace_LOG(trace_INFO, "I: Media busy\n\r");
        return MED_STATUS_BUSY;
    }

    // Check that the data to read is not too big
    if ((length + address) > (media->baseAddress + media->size)) {

        trace_LOG(trace_WARNING, "W: FLA_Read: Data too big\n\r");
        return MED_STATUS_ERROR;
    }

    // Enter Busy state
    media->state = MED_STATE_BUSY;

    // Read data
    while (length > 0) {

        *dest = *source;

        dest++;
        source++;
        length--;
    }

    // Leave the Busy state
    media->state = MED_STATE_READY;

    // Invoke callback
    if (callback != 0) {

        callback(argument, MED_STATUS_SUCCESS, 0, 0);
    }

    return MED_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
//! \brief  Writes data on a flash media
//! \param  media    Pointer to a Media instance
//! \param  address  Address at which to write
//! \param  data     Pointer to the data to write
//! \param  length   Size of the data buffer
//! \param  callback Optional pointer to a callback function to invoke when
//!                   the write operation terminates
//! \param  argument Optional argument for the callback function
//! \return Operation result code
//! \see    Media
//! \see    Callback_f
//------------------------------------------------------------------------------
static unsigned char FLA_Write(Media      *media,
                                unsigned int address,
                                void         *data,
                                unsigned int length,
                                MediaCallback   callback,
                                void         *argument)
{
    // Check that the media if ready
    if (media->state != MED_STATE_READY) {

        trace_LOG(trace_WARNING, "W: FLA_Write: Media is busy\n\r");
        return MED_STATUS_BUSY;
    }

    // Check that address is dword-aligned
    if (address%4 != 0) {

        trace_LOG(trace_WARNING, "W: FLA_Write: Address must be dword-aligned\n\r");
        return MED_STATUS_ERROR;
    }

    // Check that length is a multiple of 4
    if (length%4 != 0) {

        trace_LOG(trace_WARNING, "W: FLA_Write: Data length must be a multiple of 4 bytes\n\r");
        return MED_STATUS_ERROR;
    }

    // Check that the data to write is not too big
    if ((length + address) > (media->baseAddress + media->size)) {

        trace_LOG(trace_WARNING, "W: FLA_Write: Data too big\n\r");
        return MED_STATUS_ERROR;
    }

    // Put the media in Busy state
    media->state = MED_STATE_BUSY;

    // Initialize the transfer descriptor
    media->transfer.data = data;
    media->transfer.address = address;
    media->transfer.length = length;
    media->transfer.callback = callback;
    media->transfer.argument = argument;

    // Start the write operation
    FLA_WritePage(media);

    return MED_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
//      Exported Functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Initializes a Media instance and the associated physical interface
//! \param  media Pointer to the Media instance to initialize
//! \see    Media
//------------------------------------------------------------------------------
void FLA_Initialize(Media *media, AT91S_EFC *efc)
{
    trace_LOG(trace_INFO, "I: Flash init\n\r");

    // Initialize media fields
    media->write = FLA_Write;
    media->read = FLA_Read;
    media->flush = 0;
    media->handler = FLA_Handler;
    media->baseAddress = (unsigned int) AT91C_IFLASH;
    media->size = AT91C_IFLASH_SIZE;
    media->interface = efc;
    media->state = MED_STATE_READY;

    media->transfer.data = 0;
    media->transfer.address = 0;
    media->transfer.length = 0;
    media->transfer.callback = 0;
    media->transfer.argument = 0;

    // Initialize low-level interface
    // Configure Flash Mode register
    efc->EFC_FMR |= (BOARD_MCK / 666666) << 16;
}

#endif //#if defined(AT91C_BASE_EFC)

