#include "SWM341.h"
#include "vcom_serial.h"
#include "hid_transfer.h"


uint8_t HID_ReportDescriptor[] =
{
    0x06, 0x00, 0xFF,       // Usage Page = 0xFF00 (Vendor Defined Page 1)
    0x09, 0x01,             // Usage (Vendor Usage 1)
    0xA1, 0x01,             // Collection (Application)
    0x19, 0x01,             // Usage Minimum
    0x29, 0x40,             // Usage Maximum //64 input usages total (0x01 to 0x40)
    0x15, 0x00,             // Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x26, 0xFF, 0x00,       // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x75, 0x08,             // Report Size: 8-bit field size
    0x95, 0x40,             // Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
    0x81, 0x00,             // Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
    0x19, 0x01,             // Usage Minimum
    0x29, 0x40,             // Usage Maximum //64 output usages total (0x01 to 0x40)
    0x91, 0x00,             // Output (Data, Array, Abs): Instantiates output packet fields. Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
    0xC0                    // End Collection
};


uint8_t USBD_DeviceDescriptor[] =
{
    18,                     // bLength
    USB_DESC_DEVICE,        // bDescriptorType
#ifdef DAP_FW_V1
	0x00, 0x02,             // bcdUSB
#else
    0x01, 0x02,             // bcdUSB
#endif
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    EP_CTRL_PKSZ,       	// bMaxPacketSize0
    USBD_VID & 0xFF, USBD_VID >> 8,  	// idVendor
    USBD_PID & 0xFF, USBD_PID >> 8,  	// idProduct
    0x10, 0x01,             // bcdDevice
    0x01,                   // iManufacture
    0x02,                   // iProduct
    0x03,                   // iSerialNumber - no serial
    0x01                    // bNumConfigurations
};


uint8_t USBD_ConfigDescriptor[] =
{
    9,                      // bLength
    USB_DESC_CONFIG,        // bDescriptorType
#ifdef DAP_FW_V1
#define TOTAL_LEN  (9 + (9 + 9 + 7 + 7) + (8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 7))
#else
#define TOTAL_LEN  (9 + (9     + 7 + 7) + (8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 7))
#endif
	TOTAL_LEN & 0xFF, TOTAL_LEN >> 8,	// wTotalLength
    0x03,                   // bNumInterfaces
    0x01,                   // bConfigurationValue
    0x00,                   // iConfiguration
    0x00,               	// bmAttributes, D6: self power  D5: remote wake-up
    0x64,               	// MaxPower, 100 * 2mA = 200mA

    /* I/F descr: HID */
    9,                      // bLength
    USB_DESC_INTERFACE,     // bDescriptorType
    0x00,                   // bInterfaceNumber
    0x00,                   // bAlternateSetting
    0x02,                   // bNumEndpoints
#ifdef DAP_FW_V1
	0x03,                   // bInterfaceClass
#else
    0xFF,                   // bInterfaceClass
#endif
    0x00,                   // bInterfaceSubClass
    0x00,                   // bInterfaceProtocol
    0x04,                   // iInterface

#ifdef DAP_FW_V1
    /* HID Descriptor */
    9,                      // bLength
    USB_DESC_HID,           // bDescriptorType
    0x10, 0x01,             // HID Class Spec. release number
    0x00,                   // H/W target country
    0x01,                   // Number of HID class descriptors to follow
    USB_DESC_HID_RPT,       // Descriptor type
    sizeof(HID_ReportDescriptor) & 0xFF, sizeof(HID_ReportDescriptor) >> 8,  // Total length of report descriptor
#endif

	// CMSIS-DAP v2 WinUSB Ҫ���һ���˵��� Bulk OUT���ڶ����˵��� Bulk IN
	/* EP Descriptor: interrupt out. */
    7,                              // bLength
    USB_DESC_ENDPOINT,              // bDescriptorType
    (USB_EP_OUT | HID_INT_OUT_EP),  // bEndpointAddress
#ifdef DAP_FW_V1
    USB_EP_INT,                     // bmAttributes
#else
	USB_EP_BULK,                     // bmAttributes
#endif
    HID_INT_OUT_PKSZ, 0x00,         // wMaxPacketSize
    1,     							// bInterval
	
    /* EP Descriptor: interrupt in. */
    7,                              // bLength
    USB_DESC_ENDPOINT,              // bDescriptorType
    (USB_EP_IN | HID_INT_IN_EP),  	// bEndpointAddress
#ifdef DAP_FW_V1
    USB_EP_INT,                     // bmAttributes
#else
	USB_EP_BULK,                     // bmAttributes
#endif
    HID_INT_IN_PKSZ, 0x00,         	// wMaxPacketSize
    1,    							// bInterval
	
	// Interface Association Descriptor (IAD)
    0x08,               	// bLength
    0x0B,               	// bDescriptorType: IAD
    0x01,               	// bFirstInterface
    0x02,               	// bInterfaceCount
    0x02,               	// bFunctionClass: CDC
    0x02,               	// bFunctionSubClass
    0x01,               	// bFunctionProtocol
    0x00,               	// iFunction, �����ַ�������
	
	// I/F descriptor: VCOM
    9,  					// bLength
    USB_DESC_INTERFACE, 	// bDescriptorType
    0x01,           		// bInterfaceNumber
    0x00,           		// bAlternateSetting
    0x01,           		// bNumEndpoints
    0x02,           		// bInterfaceClass
    0x02,           		// bInterfaceSubClass
    0x01,           		// bInterfaceProtocol
    0x00,           		// iInterface

    // Communication Class Specified INTERFACE descriptor
    0x05,           		// Size of the descriptor, in bytes
    0x24,           		// CS_INTERFACE descriptor type
    0x00,           		// Header functional descriptor subtype
    0x10, 0x01,     		// Communication device compliant to the communication spec. ver. 1.10

    // Communication Class Specified INTERFACE descriptor
    0x05,           		// Size of the descriptor, in bytes
    0x24,           		// CS_INTERFACE descriptor type
    0x01,           		// Call management functional descriptor
    0x00,           		// BIT0: Whether device handle call management itself.
							// BIT1: Whether device can send/receive call management information over a Data Class Interface 0
    0x02,           		// Interface number of data class interface optionally used for call management

    // Communication Class Specified INTERFACE descriptor
    0x04,           		// Size of the descriptor, in bytes
    0x24,           		// CS_INTERFACE descriptor type
    0x02,           		// Abstract control management functional descriptor subtype
    0x00,           		// bmCapabilities

    // Communication Class Specified INTERFACE descriptor
    0x05,           		// bLength
    0x24,           		// bDescriptorType: CS_INTERFACE descriptor type
    0x06,           		// bDescriptorSubType
    0x01,           		// bMasterInterface
    0x02,           		// bSlaveInterface0

    // ENDPOINT descriptor 
    7,                   			// bLength
    USB_DESC_ENDPOINT,              // bDescriptorType
    (USB_EP_IN | CDC_INT_IN_EP),   	 // bEndpointAddress
    USB_EP_INT,                     // bmAttributes
    CDC_INT_IN_PKSZ, 0x00,         	// wMaxPacketSize
    10,                           	// bInterval

    // INTERFACE descriptor 
    9,  					// bLength
    USB_DESC_INTERFACE, 	// bDescriptorType
    0x02,           		// bInterfaceNumber
    0x00,           		// bAlternateSetting
    0x02,           		// bNumEndpoints
    0x0A,           		// bInterfaceClass
    0x00,           		// bInterfaceSubClass
    0x00,           		// bInterfaceProtocol
    0x00,           		// iInterface

    // ENDPOINT descriptor 
    7,                   			// bLength
    USB_DESC_ENDPOINT,              // bDescriptorType
    (USB_EP_IN | CDC_BULK_IN_EP),   // bEndpointAddress
    USB_EP_BULK,                    // bmAttributes
    CDC_BULK_IN_PKSZ, 0x00,         // wMaxPacketSize
    0x00,                           // bInterval

    // ENDPOINT descriptor 
    7,                   			// bLength
    USB_DESC_ENDPOINT,              // bDescriptorType
    (USB_EP_OUT | CDC_BULK_OUT_EP), // bEndpointAddress
    USB_EP_BULK,                    // bmAttributes
    CDC_BULK_OUT_PKSZ, 0x00,        // wMaxPacketSize
    0x00,                           // bInterval
};


uint8_t LangStringDesc[] =
{
    4,                      // bLength
    USB_DESC_STRING,        // bDescriptorType
    0x09, 0x04
};

uint8_t VendorStringDesc[] =
{
    14,
    USB_DESC_STRING,
    'S', 0, 'y', 0, 'n', 0, 'w', 0, 'i', 0, 't', 0,
};

uint8_t ProductStringDesc[] =
{
    36,
    USB_DESC_STRING,
    'S', 0, 'W', 0, '-', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, ' ', 0, 'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0, 'P', 0
};

uint8_t SerialNumStringDesc[] =
{
    26,
    USB_DESC_STRING,
    '0', 0, '0', 0, '2', 0, '2', 0, '0', 0, '1', 0, '1', 0, '1', 0, '0', 0, '0', 0, '0', 0, '0', 0
};

uint8_t InterfaceStringDesc[] =
{
    36,
    USB_DESC_STRING,
    'S', 0, 'W', 0, '-', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, ' ', 0, 'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0, 'P', 0
};

uint8_t *USBD_StringDescriptor[] =
{
    LangStringDesc,
    VendorStringDesc,
    ProductStringDesc,
	SerialNumStringDesc,
	InterfaceStringDesc,
    NULL,
};


uint16_t USBD_HIDOffset[2] =
{
    9+9,
    0,
};

uint8_t *USBD_HIDReport[2] =
{
    HID_ReportDescriptor,
    NULL,
};



uint8_t USBD_BOSDescriptor[] =
{
	5,
	USB_DESC_BOS,
	5+20+8, 0,					// wTotalLength
	1,							// bNumDeviceCaps
	
	/*** MS OS 2.0 descriptor platform capability descriptor ***/
	28,
	USB_DESC_CAPABILITY,
	5,							// bDevCapabilityType: PLATFORM (05H)
	0x00,
	0xDF, 0x60, 0xDD, 0xD8,		// PlatformCapabilityUUID: MS_OS_20_Platform_Capability_ID (D8DD60DF-4589-4CC7-9CD2-659D9E648A9F)
	0x89, 0x45, 0xC7, 0x4C,
	0x9C, 0xD2, 0x65, 0x9D,
	0x9E, 0x64, 0x8A, 0x9F,
	
	0x00, 0x00, 0x03, 0x06,		// dwWindowsVersion: 0x06030000 for Windows 8.1
	10+8+20+128, 0x00,      	// wTotalLength: size of MS OS 2.0 descriptor set
    WINUSB_VENDOR_CODE,   		// bMS_VendorCode
    0x00,                 		// bAltEnumCmd
};


#define MS_OS_20_SET_HEADER_DESCRIPTOR        0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION  0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION       0x02
#define MS_OS_20_FEATURE_COMPATIBLE_ID        0x03
#define MS_OS_20_FEATURE_REG_PROPERTY         0x04
#define MS_OS_20_FEATURE_MIN_RESUME_TIME      0x05
#define MS_OS_20_FEATURE_MODEL_ID             0x06
#define MS_OS_20_FEATURE_CCGP_DEVICE          0x07
#define MS_OS_20_FEATURE_VENDOR_REVISION      0x08

uint8_t MS_OS_20_DescriptorSet[] = 
{
	/*** Microsoft OS 2.0 Descriptor Set Header ***/
	10, 0,
	MS_OS_20_SET_HEADER_DESCRIPTOR, 0,
	0x00, 0x00, 0x03, 0x06,		// dwWindowsVersion: 0x06030000 for Windows 8.1
	10+8+20+128, 0,				// wTotalLength
	
	/*** Microsoft OS 2.0 function subset header ***/
	8, 0,
	MS_OS_20_SUBSET_HEADER_FUNCTION, 0,
	0,							// bFirstInterface, first interface to which this subset applies
	0,
	8+20+128, 0,				// wSubsetLength
	
	/*** Microsoft OS 2.0 compatible ID descriptor ***/
	20, 0,
	MS_OS_20_FEATURE_COMPATIBLE_ID, 0,
	'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,		// CompatibleID
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// SubCompatibleID
	
	/*** MS OS 2.0 registry property descriptor ***/
	128, 0,
	MS_OS_20_FEATURE_REG_PROPERTY, 0,
	1, 0,						// wPropertyDataType: 1 = Unicode REG_SZ
	40, 0x00,                  	// wPropertyNameLength
	'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
	'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0,   0, 0,		// PropertyName: "DeviceInterfaceGUID"
	78, 0x00, 					// wPropertyDataLength
	'{', 0, 'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
	'2', 0, '9', 0, '3', 0, 'B', 0, '-', 0, '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
	'A', 0, 'A', 0, '3', 0, '6', 0, '-', 0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0,
	'6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0, '}', 0,   0, 0, 			// PropertyData: "{CDB3B5AD-293B-4663-AA36-1AAE46463776}"
};
