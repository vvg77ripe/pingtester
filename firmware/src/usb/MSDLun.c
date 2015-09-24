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


#include "MSDLun.h"
#include <utility/trace.h>
#include <usb/device/core/USBD.h>

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------

/// Inquiry data to return to the host for the Lun.
static SBCInquiryData inquiryData = {

    SBC_DIRECT_ACCESS_BLOCK_DEVICE,  // Direct-access block device
    SBC_PERIPHERAL_DEVICE_CONNECTED, // Peripheral device is connected
    0x00,                            // Reserved bits
    0x01,                            // Media is removable
    SBC_SPC_VERSION_4,               // SPC-4 supported
    0x2,                             // Response data format, must be 0x2
    0,                           // Hierarchical addressing not supported
    0,                           // ACA not supported
    0x0,                             // Obsolete bits
    sizeof(SBCInquiryData) - 5,  // Additional length
    0,                           // No embedded SCC
    0,                           // No access control coordinator
    SBC_TPGS_NONE,                   // No target port support group
    0,                           // Third-party copy not supported
    0x0,                             // Reserved bits
    0,                           // Protection information not supported
    0x0,                             // Obsolete bit
    0,                           // No embedded enclosure service component
    0x0,                             // ???
    0,                           // Device is not multi-port
    0x0,                             // Obsolete bits
    0x0,                             // Unused feature
    0x0,                             // Unused features
    0,                           // Task management model not supported
    0x0,                             // ???
    {'A','T','M','E','L',' ',' ',' '},
    {'M','a','s','s',' ','S','t','o','r','a','g','e',' ','M','S','D'},
    {'0','.','0','1'},
    {'M','a','s','s',' ','S','t','o','r','a','g','e',' ','E','x','a','m','p','l','e'},
    0x00,                            // Unused features
    0x00,                            // Reserved bits
    {SBC_VERSION_DESCRIPTOR_SBC_3},    // SBC-3 compliant device
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} // Reserved
};

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! \brief  Initializes a LUN instance.
//! \param  lun         Pointer to the MSDLun instance to initialize
//! \param  media       Media on which the LUN is constructed
//! \param  buffer      Pointer to a buffer used for read/write operation and
//!                      which must be blockSize bytes long.
//! \param  baseAddress Base address of the LUN on the media
//! \param  size        Total size of the LUN in bytes
//! \param  blockSize   Length of one block of the LUN
//------------------------------------------------------------------------------
void LUN_Init(MSDLun         *lun,
              Media       *media,
              unsigned char *buffer,
              unsigned int  baseAddress,
              unsigned int  size,
              unsigned int  blockSize)
{
    unsigned int logicalBlockAddress = (size / blockSize) - 1;
    trace_LOG(trace_INFO, "I: LUN init\n\r");

    // Initialize LUN
    lun->media = media;
    lun->baseAddress = baseAddress;
    lun->size = size;
    lun->blockSize = blockSize;
    lun->readWriteBuffer = buffer;

    // Initialize request sense data
    lun->requestSenseData.bResponseCode = SBC_SENSE_DATA_FIXED_CURRENT;
    lun->requestSenseData.isValid = 1;
    lun->requestSenseData.bObsolete1 = 0;
    lun->requestSenseData.bSenseKey = SBC_SENSE_KEY_NO_SENSE;
    lun->requestSenseData.bReserved1 = 0;
    lun->requestSenseData.isILI = 0;
    lun->requestSenseData.isEOM = 0;
    lun->requestSenseData.isFilemark = 0;
    lun->requestSenseData.pInformation[0] = 0;
    lun->requestSenseData.pInformation[1] = 0;
    lun->requestSenseData.pInformation[2] = 0;
    lun->requestSenseData.pInformation[3] = 0;
    lun->requestSenseData.bAdditionalSenseLength
        = sizeof(SBCRequestSenseData) - 8;
    lun->requestSenseData.bAdditionalSenseCode = 0;
    lun->requestSenseData.bAdditionalSenseCodeQualifier = 0;
    lun->requestSenseData.bFieldReplaceableUnitCode = 0;
    lun->requestSenseData.bSenseKeySpecific = 0;
    lun->requestSenseData.pSenseKeySpecific[0] = 0;
    lun->requestSenseData.pSenseKeySpecific[0] = 0;
    lun->requestSenseData.isSKSV = 0;

    // Initialize inquiry data
    lun->inquiryData = &inquiryData;

    // Initialize read capacity data
    STORE_DWORDB(logicalBlockAddress,
                 lun->readCapacityData.pLogicalBlockAddress);
    STORE_DWORDB(blockSize, lun->readCapacityData.pLogicalBlockLength);
}

//------------------------------------------------------------------------------
//! \brief  Writes data on the a LUN starting at the specified block address.
//! \param  pLUN          Pointer to a MSDLun instance
//! \param  blockAddress First block address to write
//! \param  data         Pointer to the data to write
//! \param  length       Number of blocks to write
//! \param  callback     Optional callback to invoke when the write finishes
//! \return Operation result code
//------------------------------------------------------------------------------
unsigned char LUN_Write(MSDLun        *lun,
                        unsigned int blockAddress,
                        void         *data,
                        unsigned int length,
                        TransferCallback   callback,
                        void         *argument)
{
	return MED_Write(lun->media, blockAddress, data, length , (MediaCallback) callback, argument);
}

//------------------------------------------------------------------------------
//! \brief  Reads data from a LUN, starting at the specified block address.
//! \param  pLUN          Pointer to a MSDLun instance
//! \param  blockAddress First block address to read
//! \param  data         Pointer to a data buffer in which to store the data
//! \param  length       Number of blocks to read
//! \param  callback     Optional callback to invoke when the read finishes
//! \return Operation result code
//------------------------------------------------------------------------------
unsigned char LUN_Read(MSDLun        *lun,
                       unsigned int blockAddress,
                       void         *data,
                       unsigned int length,
                       TransferCallback   callback,
                       void         *argument)
{
	return MED_Read(lun->media, blockAddress, data, length , (MediaCallback) callback, argument);
}
