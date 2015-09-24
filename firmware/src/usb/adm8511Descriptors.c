
#include <config.h>
#include <board.h>

#include <usb/common/core/USBGenericDescriptor.h>
#include <usb/common/core/USBConfigurationDescriptor.h>
#include <usb/common/core/USBEndpointDescriptor.h>
#include <usb/common/core/USBStringDescriptor.h>
#include <usb/common/core/USBGenericRequest.h>

#include "adm8511.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

/// Device vendor ID
#define ADM8511_VENDORID			0x07A6

/// Device product ID.
#define ADM8511_PRODUCTID			0x8511

/// Device release number.
#define ADM8511_RELEASE				0x0100

/// Device class
#define ADM8511_CLASS				0

/// Device subclass
#define ADM8511_SUBCLASS			0

/// Device protocol
#define ADM8511_PROTOCOL			0

//------------------------------------------------------------------------------
//         Macros
//------------------------------------------------------------------------------

/// Returns the minimum between two values.
#define MIN(a, b)       ((a < b) ? a : b)

//------------------------------------------------------------------------------
//         Internal structures
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Configuration descriptor list
//------------------------------------------------------------------------------
typedef struct {

    /// Standard configuration descriptor.
    USBConfigurationDescriptor configuration;
    /// Data interface descriptor.
    USBInterfaceDescriptor intrface;
    /// Data IN endpoint descriptor.
    USBEndpointDescriptor dataIn;
    /// Data OUT endpoint descriptor.
    USBEndpointDescriptor dataOut;
    /// Interrupt IN endpoint descriptor.
    USBEndpointDescriptor intIn;

} PACKED ADM8511ConfigurationDescriptors;

//------------------------------------------------------------------------------
//         Exported variables
//------------------------------------------------------------------------------

/// Standard USB device descriptor
static const USBDeviceDescriptor deviceDescriptor = {

    sizeof(USBDeviceDescriptor),
    USBGenericDescriptor_DEVICE,
    USBDeviceDescriptor_USB2_00,
    ADM8511_CLASS,
    ADM8511_SUBCLASS,
    ADM8511_PROTOCOL,
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(0),
    ADM8511_VENDORID,
    ADM8511_PRODUCTID,
    ADM8511_RELEASE,
    0, // No string descriptor for manufacturer
    1, // Index of product string descriptor is #1
    0, // No string descriptor for serial number
    1 // Device has 1 possible configuration
};

/// Standard USB configuration descriptor
static const ADM8511ConfigurationDescriptors configurationDescriptors = {

    // Standard configuration descriptor
    {
        sizeof(USBConfigurationDescriptor),
        USBGenericDescriptor_CONFIGURATION,
        sizeof(ADM8511ConfigurationDescriptors),
        1, // One interfaces in this configuration
        1, // This is configuration #1
        0, // No string descriptor for this configuration
        BOARD_USB_BMATTRIBUTES,
        USBConfigurationDescriptor_POWER(100)
    },
    // Interface 0 descriptor
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0, // This is interface #0
        0, // This is alternate setting #0 for this interface
        3, // This interface uses 4 endpoints
        0x00,
        0xE0,
        0x00,
        0  // No string descriptor for this interface
    },
    // Bulk-IN endpoint descriptor
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN, ADM8511_DATAIN),
        USBEndpointDescriptor_BULK,
        64,
        0 // Must be 0 for full-speed bulk endpoints
    },
    // Bulk-OUT endpoint standard descriptor
    {
        sizeof(USBEndpointDescriptor), 
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT, ADM8511_DATAOUT),
        USBEndpointDescriptor_BULK,
        64,
        0 // Must be 0 for full-speed bulk endpoints
    },
    // Interrupt-IN endpoint descriptor
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN, ADM8511_INT),
        USBEndpointDescriptor_INTERRUPT,
        8,
        100 // 100ms polling interval
    }
};

// Language ID string descriptor
static const unsigned char languageIdStringDescriptor[] = {

    USBStringDescriptor_LENGTH(1),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_ENGLISH_US
};

// Product string descriptor
static const unsigned char productStringDescriptor[] = {

    USBStringDescriptor_LENGTH(24),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('P'),
    USBStringDescriptor_UNICODE('i'),
    USBStringDescriptor_UNICODE('n'),
    USBStringDescriptor_UNICODE('g'),
    USBStringDescriptor_UNICODE('T'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('s'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE(' '),
    USBStringDescriptor_UNICODE('E'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('h'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('n'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE(' '),
    USBStringDescriptor_UNICODE('C'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('d')
};

/// List of string descriptors used by the device
static const unsigned char *stringDescriptors[] = {
    languageIdStringDescriptor,
    productStringDescriptor,
};

/// List of standard descriptors
USBDDriverDescriptors ADM8511Descriptors = {

    &deviceDescriptor,
    (USBConfigurationDescriptor *) &(configurationDescriptors),
    0, // No full-speed device qualifier descriptor
    0, // No full-speed other speed configuration
    0, // No high-speed device descriptor
    0, // No high-speed configuration descriptor
    0, // No high-speed device qualifier descriptor
    0, // No high-speed other speed configuration descriptor
    stringDescriptors,
    2 // 2 string descriptors in list
};
