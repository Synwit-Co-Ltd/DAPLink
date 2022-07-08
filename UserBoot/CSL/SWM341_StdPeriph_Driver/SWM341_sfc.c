/****************************************************************************************************************************************** 
* �ļ�����: SWM341_sfc.c
* ����˵��:	SWM341��Ƭ����SFC��Serial Flash Controller��ģ��������
* ����֧��:	http://www.synwit.com.cn/e/tool/gbook/?bid=1
* ע������: 
* �汾����:	V1.1.0		2017��10��25��
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
#include "SWM341_sfc.h"


/****************************************************************************************************************************************** 
* ��������:	SFC_Init()
* ����˵��:	SFC��Serial Flash Controller����ʼ��
* ��    ��: SFC_InitStructure * initStruct	SFC��ʼ������ֵ
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void SFC_Init(SFC_InitStructure * initStruct)
{
	SYS->CLKEN1 |= (1 << SYS_CLKEN1_SFC_Pos);
	
	*((__IO uint32_t *)((uint32_t )&SFC->CFG + 0x3F4)) = 7;
	
	SFC->CFG &= ~(SFC_CFG_CLKDIV_Msk | SFC_CFG_DATA4L_RD_Msk | SFC_CFG_DATA4L_PP_Msk);
	SFC->CFG |= (initStruct->ClkDiv << SFC_CFG_CLKDIV_Pos) |
				(initStruct->Width_Read << SFC_CFG_DATA4L_RD_Pos) |
				(initStruct->Width_PageProgram << SFC_CFG_DATA4L_PP_Pos);
	
	SFC->CFG |= (1 << SFC_CFG_CMDWREN_Pos);
	SFC->CMDAHB &= ~(SFC_CMDAHB_READ_Msk | SFC_CMDAHB_PP_Msk);
	SFC->CMDAHB |= (initStruct->Cmd_Read << SFC_CMDAHB_READ_Pos) |
				   (initStruct->Cmd_PageProgram << SFC_CMDAHB_PP_Pos);
	SFC->CFG &= ~(1 << SFC_CFG_CMDWREN_Pos);
	
	SFC->TIM &= ~(SFC_TIM_WIP_CHK_ITV_Msk | SFC_TIM_WIP_CHK_LMT_Msk);
	SFC->TIM |= ((CyclesPerUs / 2) << SFC_TIM_WIP_CHK_ITV_Pos) |
				(255 << SFC_TIM_WIP_CHK_LMT_Pos);
}

/****************************************************************************************************************************************** 
* ��������:	SFC_ReadJEDEC()
* ����˵��:	��ȡ JEDEC ID
* ��    ��: ��
* ��    ��: uint32_t			JEDEC ID
* ע������: ��
******************************************************************************************************************************************/
uint32_t SFC_ReadJEDEC(void)
{
	SFC->CFG &= ~SFC_CFG_CMDTYPE_Msk;
	SFC->CFG |= (1 << SFC_CFG_CMDWREN_Pos) |
				(2 << SFC_CFG_CMDTYPE_Pos);
	SFC->CMD = SFC_CMD_READ_JEDEC;
	
	SFC->GO = 1;
	while(SFC->GO);
	
	return SFC->DATA;
}

/****************************************************************************************************************************************** 
* ��������:	SFC_Erase()
* ����˵��:	SPI Flash����������ÿ������4K�ֽ�
* ��    ��: uint32_t addr		Ҫ���������ĵ�ַ������4K���룬��addr%4096 == 0
* ��    ��: uint32_t			SFC_RES_OK��SFC_RES_TO��SFC_RES_ERR
* ע������: ��
******************************************************************************************************************************************/
uint32_t SFC_Erase(uint32_t addr)
{
	return SFC_EraseEx(addr, SFC_CMD_ERASE_SECTOR);
}

/****************************************************************************************************************************************** 
* ��������:	SFC_EraseEx()
* ����˵��:	SPI Flash������ͨ���ṩ��ͬ��������֧��Ƭ���������������
* ��    ��: uint32_t addr		Ҫ�����Ŀ�ĵ�ַ���� addr == 0xFFFFFFFF ʱ��ִ��Ƭ��
*			uint8_t cmd			���������룬��ЩSPI Flash֧�ֶ��ִ�С�Ŀ�Ĳ�������ͬ����Сʹ�ò�ͬ������
* ��    ��: uint32_t			SFC_RES_OK��SFC_RES_TO��SFC_RES_ERR
* ע������: ��
******************************************************************************************************************************************/
uint32_t SFC_EraseEx(uint32_t addr, uint8_t cmd)
{
	uint32_t res;
	uint8_t type = (addr == 0xFFFFFFFF) ? 5 : 7;
	
	SFC->ADDR = addr;
	
	SFC->CFG &= ~SFC_CFG_CMDTYPE_Msk;
	SFC->CFG |= (1 << SFC_CFG_WREN_Pos) |
				(1 << SFC_CFG_CMDWREN_Pos) |
				(type << SFC_CFG_CMDTYPE_Pos);
	SFC->CMD = cmd;
	
	SFC->GO = 1;
	while(SFC->GO);
	SFC->CFG &= ~SFC_CFG_WREN_Msk;
	
	if(SFC->IF & SFC_IF_ERR_Msk)     res = SFC_RES_ERR;
	else if(SFC->IF & SFC_IF_TO_Msk) res = SFC_RES_TO;
	else                             res = SFC_RES_OK;
	
	return res;
}

/****************************************************************************************************************************************** 
* ��������:	SFC_Write()
* ����˵��:	SPI Flash����д��
* ��    ��: uint32_t addr		����Ҫд�뵽Flash�еĵ�ַ���ֶ���
*			uint32_t buff[]		Ҫд��Flash�е�����
*			uint32_t cnt		Ҫд�����ݵĸ���������Ϊ��λ�����64
* ��    ��: uint32_t			SFC_RES_OK��SFC_RES_TO��SFC_RES_ERR
* ע������: Ҫд������ݱ���ȫ����ͬһҳ�ڣ���addr/256 == (addr+(cnt-1)*4)/256
******************************************************************************************************************************************/
uint32_t SFC_Write(uint32_t addr, uint32_t buff[], uint32_t cnt)
{	
	uint32_t res, i;
	
	SFC->CFG |= (1 << SFC_CFG_WREN_Pos);
	for(i = 0; i < cnt; i++)
		*((volatile unsigned int *)(0x70000000+addr+i*4)) = buff[i];
	
	while(SFC->SR & SFC_SR_BUSY_Msk) __NOP();
	SFC->CFG &= ~SFC_CFG_WREN_Msk;
	
	if(SFC->IF & SFC_IF_ERR_Msk)     res = SFC_RES_ERR;
	else if(SFC->IF & SFC_IF_TO_Msk) res = SFC_RES_TO;
	else                             res = SFC_RES_OK;
	
	return res;
}

/****************************************************************************************************************************************** 
* ��������:	SFC_Read()
* ����˵��:	SPI Flash���ݶ�ȡ
* ��    ��: uint32_t addr		Ҫ��ȡ��������Flash�еĵ�ַ���ֶ���
*			uint32_t buff[]		��ȡ�������ݴ���buffָ����ڴ�
*			uint32_t cnt		Ҫ��ȡ�����ݵĸ���������Ϊ��λ
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void SFC_Read(uint32_t addr, uint32_t buff[], uint32_t cnt)
{	
	uint32_t i;
	
	for(i = 0; i < cnt; i++)
		buff[i] = *((volatile unsigned int *)(0x70000000+addr+i*4));
}


/****************************************************************************************************************************************** 
* ��������:	SFC_ReadStatusReg()
* ����˵��:	SPI Flash��ȡ״̬�Ĵ���
* ��    ��: uint8_t cmd			��ȡʹ�õ�������
* ��    ��: uint8_t				������״̬�Ĵ���ֵ
* ע������: ��
******************************************************************************************************************************************/
uint8_t SFC_ReadStatusReg(uint8_t cmd)
{
	SFC->CFG &= ~SFC_CFG_CMDTYPE_Msk;
	SFC->CFG |= (1 << SFC_CFG_CMDWREN_Pos) |
				(1 << SFC_CFG_CMDTYPE_Pos);
	SFC->CMD = cmd;
	
	SFC->GO = 1;
	while(SFC->GO);
	
	return SFC->DATA;
}


/****************************************************************************************************************************************** 
* ��������:	SFC_WriteStatusReg()
* ����˵��:	SPI Flashд��״̬�Ĵ���
* ��    ��: uint8_t cmd			д��ʹ�õ�������
*			uint16_t reg		Ҫд���״̬�Ĵ���ֵ
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void SFC_WriteStatusReg(uint8_t cmd, uint16_t reg)
{
	SFC->CFG &= ~SFC_CFG_CMDTYPE_Msk;
	SFC->CFG |= (1 << SFC_CFG_WREN_Pos) |
				(1 << SFC_CFG_CMDWREN_Pos) |
				(6 << SFC_CFG_CMDTYPE_Pos);
	SFC->CMD = cmd;
	
	SFC->DATA = reg;
	
	SFC->GO = 1;
	while(SFC->GO);
}


/****************************************************************************************************************************************** 
* ��������:	SFC_QuadSwitch()
* ����˵��:	SPI Flash Quadģʽ����
* ��    ��: uint8_t on			1 ���� Quad ģʽ   0 �ر� Quad ģʽ
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
void SFC_QuadSwitch(uint8_t on)
{
	uint16_t reg = (SFC_ReadStatusReg(SFC_CMD_READ_STATUS_REG2) << 8) | 
				   (SFC_ReadStatusReg(SFC_CMD_READ_STATUS_REG1) << 0);
	
	if(on)
		reg |= (1 << SFC_STATUS_REG_QUAD_Pos);
	else
		reg &=~(1 << SFC_STATUS_REG_QUAD_Pos);
	
	SFC_WriteStatusReg(SFC_CMD_WRITE_STATUS_REG1, reg);
}


/****************************************************************************************************************************************** 
* ��������:	SFC_QuadState()
* ����˵��:	SPI Flash Quadģʽ����״̬��ѯ
* ��    ��: uint8_t on			1 ���� Quad ģʽ   0 �ر� Quad ģʽ
* ��    ��: ��
* ע������: ��
******************************************************************************************************************************************/
uint8_t SFC_QuadState(void)
{
	uint8_t reg = SFC_ReadStatusReg(SFC_CMD_READ_STATUS_REG2);
	
	if(reg & (1 << (SFC_STATUS_REG_QUAD_Pos - 8)))
		return 1;
	else
		return 0;
}
