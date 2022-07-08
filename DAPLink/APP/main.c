#include "SWM341.h"
#include "vcom_serial.h"
#include "hid_transfer.h"

#include "DAP.h"


int main(void)
{	
 	SystemInit();
	
	DAP_Setup();
	
	HID_Init();
	
	VCOM_Init();
	
	USBD_Open();
	
	SysTick_Config(CyclesPerUs * 1000);		// 1ms interrupt
	
	GPIO_Init(GPIOC, PIN12, 1, 0, 0, 0);		// PC12 => UART TXD 指示
	GPIO_Init(GPIOC, PIN13, 1, 0, 0, 0);		// PC13 => UART RXD 指示
	
    while(1)
    {
		usbd_hid_process();
		
		VCOM_TransferData();
		
		GPIOC->DATAPIN13 = (UART0->CTRL & UART_CTRL_RXNE_Msk) ? 1 : 0;
		GPIOC->DATAPIN12 = (UART0->CTRL & UART_CTRL_TXIDLE_Msk) ? 0 : 1;
    }
}


uint32_t SysTick_Count = 0;

void SysTick_Handler(void)
{
	SysTick_Count++;
}


/****************************************************************************************************************************************** 
* 函数名称: fputc()
* 功能说明: printf()使用此函数完成实际的串口打印动作
* 输    入: int ch		要打印的字符
*			FILE *f		文件句柄
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
int fputc(int ch, FILE *f)
{
	UART_WriteByte(UART0, ch);
	
	while(UART_IsTXBusy(UART0));
 	
	return ch;
}
