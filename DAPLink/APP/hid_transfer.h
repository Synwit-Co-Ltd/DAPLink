#ifndef __HID_TRANSFER_H__
#define __HID_TRANSFER_H__


#define USBD_VID	0x0417
#ifdef DAP_FW_V1
#define USBD_PID    0x5023
#else
#define USBD_PID    0x7692
#endif


#define WINUSB_VENDOR_CODE	0x34


/* Define the EP number */
#define HID_INT_IN_EP       1
#define HID_INT_OUT_EP      1
#define CDC_INT_IN_EP     	2
#define CDC_BULK_IN_EP    	3
#define CDC_BULK_OUT_EP   	3

#define EP_CTRL_PKSZ		64
#define HID_INT_IN_PKSZ		64
#define HID_INT_OUT_PKSZ	64
#define CDC_INT_IN_PKSZ		8
#define CDC_BULK_IN_PKSZ	64
#define CDC_BULK_OUT_PKSZ	64



void HID_Init(void);
void HID_ClassRequest(USB_Setup_Packet_t * pSetup);
void WINUSB_VendorRequest(USB_Setup_Packet_t * pSetup);

void HID_SetInReport(void);
void HID_GetOutReport(uint8_t *buf, uint32_t u32Size);

uint8_t usbd_hid_process(void);


#endif
