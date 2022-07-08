#include "SWM341.h"
#include "vcom_serial.h"
#include "hid_transfer.h"


void VCOM_Init(void)
{
	UART_InitStructure UART_initStruct;
	
	PORT_Init(PORTM, PIN0, PORTM_PIN0_UART0_RX, 1);	//GPIOM.0配置为UART0输入引脚
	PORT_Init(PORTM, PIN1, PORTM_PIN1_UART0_TX, 0);	//GPIOM.1配置为UART0输出引脚
 	
 	UART_initStruct.Baudrate = 57600;
	UART_initStruct.DataBits = UART_DATA_8BIT;
	UART_initStruct.Parity = UART_PARITY_NONE;
	UART_initStruct.StopBits = UART_STOP_1BIT;
	UART_initStruct.RXThreshold = 3;
	UART_initStruct.RXThresholdIEn = 1;
	UART_initStruct.TXThreshold = 3;
	UART_initStruct.TXThresholdIEn = 0;
	UART_initStruct.TimeoutTime = 10;
	UART_initStruct.TimeoutIEn = 1;
 	UART_Init(UART0, &UART_initStruct);
	UART_Open(UART0);
}


volatile VCOM Vcom;

void VCOM_BulkIN_Handler(void)
{
    Vcom.in_bytes = 0;
}

void VCOM_BulkOUT_Handler(void)
{
    Vcom.out_bytes = USBD_RxRead((uint8_t *)Vcom.out_buff, 64);

	Vcom.out_ready = 1;
}


STR_VCOM_LINE_CODING LineCfg = {57600, 0, 0, 8};   // Baud rate : 57600, stop bits, parity bits, data bits

void VCOM_SetLineCoding(void)
{
    uint8_t parity;

    NVIC_DisableIRQ(UART0_IRQn);
	
	// Reset software FIFO
	Vcom.rx_bytes = 0;
	Vcom.rx_head = 0;
	Vcom.rx_tail = 0;

	Vcom.tx_bytes = 0;
	Vcom.tx_head = 0;
	Vcom.tx_tail = 0;
	
	switch(LineCfg.u8ParityType)
	{
	case 0:  parity = UART_PARITY_NONE; break;
	case 1:  parity = UART_PARITY_ODD;  break;
	case 2:  parity = UART_PARITY_EVEN; break;
	case 3:  parity = UART_PARITY_ONE;  break;
	case 4:  parity = UART_PARITY_ZERO; break;
	default: parity = UART_PARITY_NONE;	break;
	}
	
	UART0->CTRL &= ~(UART_CTRL_DATA9b_Msk | UART_CTRL_PARITY_Msk | UART_CTRL_STOP2b_Msk);
	UART0->CTRL |= ((LineCfg.u8DataBits == 9)   << UART_CTRL_DATA9b_Pos) |
				   (parity                      << UART_CTRL_PARITY_Pos) |
				   ((LineCfg.u8CharFormat == 2) << UART_CTRL_STOP2b_Pos);
	
	UART_SetBaudrate(UART0, LineCfg.u32DTERate);
	
	NVIC_EnableIRQ(UART0_IRQn);
}


extern uint32_t SysTick_Count;
static uint32_t SysTick_Count_Save;

void VCOM_TransferData(void)
{
    int32_t i, len;

    /* Check whether USB is ready for next packet or not */
    if(Vcom.in_bytes == 0)
    {
        /* Check whether we have new COM Rx data to send to USB or not */
        if(Vcom.rx_bytes && ((Vcom.rx_bytes >= CDC_BULK_IN_PKSZ) || (SysTick_Count_Save != SysTick_Count)))
        {
			SysTick_Count_Save = SysTick_Count;
			
            len = Vcom.rx_bytes;
            if(len > CDC_BULK_IN_PKSZ)
                len = CDC_BULK_IN_PKSZ;

            for(i = 0; i < len; i++)
            {
                Vcom.in_buff[i] = Vcom.rx_buff[Vcom.rx_head++];
                if(Vcom.rx_head >= RX_BUFF_SIZE)
                    Vcom.rx_head = 0;
            }

			__disable_irq();
            Vcom.rx_bytes -= len;
            __enable_irq();

            Vcom.in_bytes = len;
            USBD_TxWrite(CDC_BULK_IN_EP, (uint8_t *)Vcom.in_buff, len);
        }
        else
        {
            /* Prepare a zero packet if previous packet size is CDC_BULK_IN_PKSZ and
               no more data to send at this moment to note Host the transfer has been done */
            if(USBD->INEP[CDC_BULK_IN_EP].TXTRSZ == CDC_BULK_IN_PKSZ)
                USBD_TxWrite(CDC_BULK_IN_EP, 0, 0);
        }
    }

    /* Process the Bulk out data when bulk out data is ready. */
    if(Vcom.out_ready && (Vcom.out_bytes <= TX_BUFF_SIZE - Vcom.tx_bytes))
    {
        for(i = 0; i < Vcom.out_bytes; i++)
        {
            Vcom.tx_buff[Vcom.tx_tail++] = Vcom.out_buff[i];
            if(Vcom.tx_tail >= TX_BUFF_SIZE)
                Vcom.tx_tail = 0;
        }

        __disable_irq();
        Vcom.tx_bytes += Vcom.out_bytes;
		__enable_irq();

        Vcom.out_ready = 0; 	// Clear bulk out ready flag

        /* Ready to get next BULK out */
		USBD_RxReady(CDC_BULK_OUT_EP);
    }

    /* Process the software Tx FIFO */
    if(Vcom.tx_bytes)
    {
        while(!UART_IsTXFIFOFull(UART0))
		{
			UART0->DATA = Vcom.tx_buff[Vcom.tx_head++];
			if(Vcom.tx_head >= TX_BUFF_SIZE)
				Vcom.tx_head = 0;
			Vcom.tx_bytes--;
			
			if(Vcom.tx_bytes == 0)
				break;
		}
    }
}


void UART0_Handler(void)
{
    uint32_t chr;
	
	if(UART_INTRXThresholdStat(UART0) || UART_INTTimeoutStat(UART0))
	{
		while(!UART_IsRXFIFOEmpty(UART0))
		{
			UART_ReadByte(UART0, &chr);
			
			if(Vcom.rx_bytes < RX_BUFF_SIZE)  	// Check if buffer full
			{
				Vcom.rx_buff[Vcom.rx_tail++] = chr;
				if(Vcom.rx_tail >= RX_BUFF_SIZE)
					Vcom.rx_tail = 0;
				Vcom.rx_bytes++;
			}
		}
	}
}
