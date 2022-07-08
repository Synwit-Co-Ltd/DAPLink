/****************************************************************************************************************************************** 
* �ļ�����: SWM341_dma.c
* ����˵��:	SWM341��Ƭ����DMA����������
* ����֧��:	http://www.synwit.com.cn/e/tool/gbook/?bid=1
* ע������: 
* �汾����:	V1.0.0		2016��1��30��
* ������¼:  
*
*
*******************************************************************************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH CODING INFORMATION 
* REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A RESULT, SYNWIT SHALL NOT BE HELD LIABLE 
* FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT 
* OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN IN CONN-
* -ECTION WITH THEIR PRODUCTS.
*
* COPYRIGHT 2012 Synwit Technology
*******************************************************************************************************************************************/
#include "SWM341.h"
#include "SWM341_dma.h"

/****************************************************************************************************************************************** 
* ��������: DMA_CH_Init()
* ����˵��:	DMAͨ����ʼ��
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
*			DMA_InitStructure * initStruct    ����DMAͨ������趨ֵ�Ľṹ��
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_Init(uint32_t chn, DMA_InitStructure * initStruct)
{
	DMA->EN = 1;			//ÿ��ͨ�������Լ������Ŀ��ؿ��ƣ������ܿ��ؿ�����һֱ������
	
	DMA_CH_Close(chn);		//�رպ�����
	
	DMA->CH[chn].CR = (initStruct->Mode << DMA_CR_AUTORE_Pos) | 
	                  ((initStruct->Count - 1) << DMA_CR_LEN_Pos);
	
	DMA->CH[chn].SRC = initStruct->SrcAddr;
	DMA->CH[chn].DST = initStruct->DstAddr;
	
	DMA->CH[chn].AM = (initStruct->SrcAddrInc << DMA_AM_SRCAM_Pos)  |
					  (initStruct->DstAddrInc << DMA_AM_DSTAM_Pos)  |
					  (initStruct->Unit       << DMA_AM_SRCBIT_Pos) |
					  (initStruct->Unit       << DMA_AM_DSTBIT_Pos);
	
	switch(initStruct->Handshake & DMA_HS_MSK)
	{
	case DMA_HS_NO:
		DMA->CH[chn].MUX = 0;
		break;
	
	case DMA_HS_SRC:
		DMA->CH[chn].MUX = ((initStruct->Handshake & 0xF) << DMA_MUX_SRCHSSIG_Pos) | (1 << DMA_MUX_SRCHSEN_Pos);
		break;
	
	case DMA_HS_DST:
		DMA->CH[chn].MUX = ((initStruct->Handshake & 0xF) << DMA_MUX_DSTHSSIG_Pos) | (1 << DMA_MUX_DSTHSEN_Pos);
		break;
	
	case DMA_HS_EXT:
		DMA->CH[chn].MUX = ((initStruct->Handshake & 0xF) << DMA_MUX_EXTHSSIG_Pos) | (1 << DMA_MUX_EXTHSEN_Pos);
		DMA->CH[chn].CR |= (1 << DMA_CR_STEPOP_Pos);
		break;
	
	default:
		break;
	}
	
	DMA->PRI &= ~(1 << chn);
	DMA->PRI |= (initStruct->Priority << chn);
	
	DMA->IF  = (1 << chn);		//����жϱ�־
	DMA->IE |= (1 << chn);
	if(initStruct->DoneIE)	DMA->IM &= ~(1 << chn);
	else		            DMA->IM |=  (1 << chn);
	
	if(initStruct->DoneIE) NVIC_EnableIRQ(DMA_IRQn);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_Open()
* ����˵��:	DMAͨ�������������������ͨ�����������������䣻����Ӳ������ͨ������������ȳ��ִ����źź�ſ�ʼ����
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_Open(uint32_t chn)
{
	DMA->CH[chn].CR |= (1 << DMA_CR_RXEN_Pos);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_Close()
* ����˵��:	DMAͨ���ر�
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_Close(uint32_t chn)
{
	DMA->CH[chn].CR &= ~(DMA_CR_TXEN_Msk | DMA_CR_RXEN_Msk);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_INTEn()
* ����˵��:	DMA�ж�ʹ�ܣ����ݰ�����ɺ󴥷��ж�
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_INTEn(uint32_t chn)
{
	DMA->IM &= ~(1 << chn);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_INTDis()
* ����˵��:	DMA�жϽ�ֹ�����ݰ�����ɺ󲻴����ж�
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_INTDis(uint32_t chn)
{
	DMA->IM |= (1 << chn);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_INTClr()
* ����˵��:	DMA�жϱ�־���
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void DMA_CH_INTClr(uint32_t chn)
{
	DMA->IF = (1 << chn);
}

/****************************************************************************************************************************************** 
* ��������: DMA_CH_INTStat()
* ����˵��:	DMA�ж�״̬��ѯ
* ��    ��: uint32_t chn			ָ��Ҫ���õ�ͨ������Чֵ��DMA_CH0��DMA_CH1��DMA_CH2��DMA_CH3
* ��    ��: uint32_t				1 ���ݰ������    0 ���ݰ���δ���
* ע������: ��
******************************************************************************************************************************************/
uint32_t DMA_CH_INTStat(uint32_t chn)
{	
	return (DMA->IF & (1 << chn)) ? 1 : 0;
}
