#include <string.h>
#include "SWM341.h"
#include "vcom_serial.h"
#include "hid_transfer.h"


extern uint8_t  USBD_DeviceDescriptor[];
extern uint8_t  USBD_ConfigDescriptor[];
extern uint8_t *USBD_StringDescriptor[];
extern uint8_t  USBD_BOSDescriptor[];
extern uint16_t USBD_HIDOffset[];
extern uint8_t *USBD_HIDReport[];

void HID_Init(void)
{
	USBD_Info.Mode = USBD_MODE_DEV;
	USBD_Info.Speed = USBD_SPEED_FS;
	USBD_Info.CtrlPkSiz = EP_CTRL_PKSZ;
	USBD_Info.DescDevice = USBD_DeviceDescriptor;
	USBD_Info.DescConfig = USBD_ConfigDescriptor;
	USBD_Info.DescString = USBD_StringDescriptor;
#ifdef DAP_FW_V1
	USBD_Info.DescHIDOffset = USBD_HIDOffset;
	USBD_Info.DescHIDReport = USBD_HIDReport;
#else
	USBD_Info.DescBOS = USBD_BOSDescriptor;
#endif
	USBD_Info.pClassRequest_Callback = HID_ClassRequest;
	USBD_Info.pVendorRequest_Callback = WINUSB_VendorRequest;
	
	USBD_Init();
}


static uint8_t Buffer[HID_INT_OUT_PKSZ];

void USB_Handler(void)
{
	uint32_t devif = USBD->DEVIF;
    uint32_t epif  = USBD->EPIF;
	
    if(devif & USBD_DEVIF_RST_Msk)
    {
        USBD->DEVIF = USBD_DEVIF_RST_Msk;
    }
	else if(devif & USBD_DEVIF_SETCFG_Msk)
	{
		USBD->DEVIF = USBD_DEVIF_SETCFG_Msk;
		
		USBD_RxReady(HID_INT_OUT_EP);
		USBD_RxReady(CDC_BULK_OUT_EP);
	}
	else if(devif & USBD_DEVIF_SUSP_Msk)
	{
		USBD->DEVIF = USBD_DEVIF_SUSP_Msk;
	}
    else if(devif & USBD_DEVIF_SETUP_Msk)
    {
		USBD->SETUPSR = USBD_SETUPSR_DONE_Msk;
		
		if(USBD->SETUPSR & USBD_SETUPSR_SUCC_Msk)
		{
			USBD_ProcessSetupPacket();
		}
    }
	else
    {
        if(epif & USBD_EPIE_INEP0_Msk)					// Control IN
        {
			if(USBD_TxSuccess(0))
			{
				USBD_CtrlIn();
			}
			USBD_TxIntClr(0);
        }
        else if(epif & USBD_EPIE_OUTEP0_Msk)			// Control OUT
        {
			USBD_RxIntClr();
			if(USBD_RxSuccess())
			{
				if(pUSB_Setup->bRequest == SET_LINE_CODE)
				{
					USBD_CtrlOut();
					
					USBD_TxWrite(0, 0, 0);	// Status stage
					
					VCOM_SetLineCoding();
				}
			}
        }
		else if(epif & (1 << HID_INT_IN_EP))			// Interrupt IN
        {
			if(USBD_TxSuccess(HID_INT_IN_EP))
			{
				HID_SetInReport();
			}
			USBD_TxIntClr(HID_INT_IN_EP);
        }
        else if(epif & (1 << (HID_INT_OUT_EP + 16)))	// Interrupt OUT
        {
			USBD_RxIntClr();
			if(USBD_RxSuccess())
			{
				uint8_t size = USBD_RxRead(Buffer, sizeof(Buffer));
				HID_GetOutReport(Buffer, size);
				
				USBD_RxReady(HID_INT_OUT_EP);
			}
        }
		else if(epif & (1 << CDC_INT_IN_EP))
        {
			USBD_TxIntClr(CDC_INT_IN_EP);
        }
        else if(epif & (1 << CDC_BULK_IN_EP))			// Bulk IN
        {
			if(USBD_TxSuccess(CDC_BULK_IN_EP) )
			{
				VCOM_BulkIN_Handler();
			}
			USBD_TxIntClr(CDC_BULK_IN_EP);
        }
        else if(epif & (1 << (CDC_BULK_OUT_EP + 16)))	// Bulk OUT
        {
			USBD_RxIntClr();
			if(USBD_RxSuccess())
			{
				VCOM_BulkOUT_Handler();
			}
        }
    }
}


void HID_ClassRequest(USB_Setup_Packet_t * pSetup)
{
    if(pSetup->bRequestType & 0x80)		// Device to Host
    {
        switch(pSetup->bRequest)
        {
        case USB_HID_GET_REPORT:
        case USB_HID_GET_IDLE:
        case USB_HID_GET_PROTOCOL:
			USBD_Stall0();
            break;
		
		case GET_LINE_CODE:
			/* Data stage */
			USBD_TxWrite(0, (uint8_t *)&LineCfg, sizeof(LineCfg));

            /* Status stage */
			USBD_RxReady(0);
            break;
		
		default:
			USBD_Stall0();
            break;
        }
    }
    else								// Host to Device
    {
        switch(pSetup->bRequest)
        {
        case USB_HID_SET_REPORT:
            if((pSetup->wValue >> 8) == 3)
            {
                USBD_TxWrite(0, 0, 0);
            }
            break;
		
        case USB_HID_SET_IDLE:
            /* Status stage */
            USBD_TxWrite(0, 0, 0);
            break;
		
        case USB_HID_SET_PROTOCOL:
			USBD_Stall0();
			break;
		
		case SET_CONTROL_LINE:
			Vcom.hw_flow = pSetup->wValue;	// hw_flow.0 DTR   hw_flow.1 RTS

            /* Status stage */
			USBD_TxWrite(0, 0, 0);
            break;

        case SET_LINE_CODE:
			USBD_PrepareCtrlOut((uint8_t *)&LineCfg, 7);

            /* Status stage
			   读取数据再后执行 USBD_TxWrite(0, 0, 0)，否则主机可能会发来新的数据覆盖旧数据 */
            break;
		
        default:
			USBD_Stall0();
            break;
        }
    }
}


extern uint8_t MS_OS_20_DescriptorSet[];
void WINUSB_VendorRequest(USB_Setup_Packet_t * pSetup)
{
	uint16_t len;
	
    if(pSetup->bRequestType & 0x80)		// Device to Host
    {
        switch(pSetup->bRequest)
        {
        case WINUSB_VENDOR_CODE:
#define min(a, b)	((a)<(b) ? (a) : (b))
			switch(pSetup->wIndex)
			{
			case 7:
				len = MS_OS_20_DescriptorSet[8] | (MS_OS_20_DescriptorSet[9] << 8);
				USBD_PrepareCtrlIn(MS_OS_20_DescriptorSet, min(pSetup->wLength, len));
				USBD_RxReady(0);
				return;
			}
        }
    }
    else								// Host to Device
    {
    }
	
	USBD_Stall0();
}



/***************************************************************/
#include "DAP_config.h"
#include "DAP.h"

static volatile uint8_t  USB_RequestFlag;       // Request  Buffer Usage Flag
static volatile uint32_t USB_RequestIn;         // Request  Buffer In  Index
static volatile uint32_t USB_RequestOut;        // Request  Buffer Out Index

static volatile uint8_t  USB_ResponseIdle = 1;  // Response Buffer Idle  Flag
static volatile uint8_t  USB_ResponseFlag;      // Response Buffer Usage Flag
static volatile uint32_t USB_ResponseIn;        // Response Buffer In  Index
static volatile uint32_t USB_ResponseOut;       // Response Buffer Out Index

static uint8_t  USB_Request [DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Request  Buffer
static uint8_t  USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Response Buffer
static uint16_t USB_ResponseSize[DAP_PACKET_COUNT];				  // number of bytes in response


uint8_t usbd_hid_process(void)
{
	uint32_t n;

	// Process pending requests
	if((USB_RequestOut != USB_RequestIn) || USB_RequestFlag)
	{
		USB_ResponseSize[USB_ResponseIn] = DAP_ProcessCommand(USB_Request[USB_RequestOut], USB_Response[USB_ResponseIn]);

		// Update request index and flag
		n = USB_RequestOut + 1;
		if(n == DAP_PACKET_COUNT)
			n = 0;
		USB_RequestOut = n;

		if(USB_RequestOut == USB_RequestIn)
			USB_RequestFlag = 0;

		if(USB_ResponseIdle)
		{	// Request that data is send back to host
			USB_ResponseIdle = 0;
			
#ifdef DAP_FW_V1
			USBD_TxWrite(HID_INT_IN_EP, USB_Response[USB_ResponseIn], DAP_PACKET_SIZE);
#else
			USBD_TxWrite(HID_INT_IN_EP, USB_Response[USB_ResponseIn], USB_ResponseSize[USB_ResponseIn]);
#endif
		}
		else
		{	// Update response index and flag
			n = USB_ResponseIn + 1;
			if (n == DAP_PACKET_COUNT)
				n = 0;
			USB_ResponseIn = n;

			if (USB_ResponseIn == USB_ResponseOut)
				USB_ResponseFlag = 1;
		}
		return 1;
	}
	return 0;
}


void HID_GetOutReport(uint8_t *EpBuf, uint32_t len)
{
    if(EpBuf[0] == ID_DAP_TransferAbort)
	{
		DAP_TransferAbort = 1;
		return;
	}
	
	if(USB_RequestFlag && (USB_RequestIn == USB_RequestOut))
		return;  // Discard packet when buffer is full

	// Store data into request packet buffer
	memcpy(USB_Request[USB_RequestIn], EpBuf, len);

	USB_RequestIn++;
	if(USB_RequestIn == DAP_PACKET_COUNT)
		USB_RequestIn = 0;
	if(USB_RequestIn == USB_RequestOut)
		USB_RequestFlag = 1;
}


void HID_SetInReport(void)
{
	if((USB_ResponseOut != USB_ResponseIn) || USB_ResponseFlag)
	{
#ifdef DAP_FW_V1
		USBD_TxWrite(HID_INT_IN_EP, USB_Response[USB_ResponseOut], DAP_PACKET_SIZE);
#else
		USBD_TxWrite(HID_INT_IN_EP, USB_Response[USB_ResponseOut], USB_ResponseSize[USB_ResponseOut]);
#endif
		USB_ResponseOut++;
		if (USB_ResponseOut == DAP_PACKET_COUNT)
			USB_ResponseOut = 0;
		if (USB_ResponseOut == USB_ResponseIn)
			USB_ResponseFlag = 0;
	}
	else
	{
		USB_ResponseIdle = 1;
	}
}
