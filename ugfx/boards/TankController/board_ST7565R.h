/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include <stm32f10x.h>
#include <FreeRTOS.h>
#include <task.h>

#define sLCD_SPI						SPI2
#define sLCD_SPI_CLK					RCC_APB1Periph_SPI2

#define sLCD_SPI_SCK_PIN				GPIO_Pin_13					/* PB.13 */
#define sLCD_SPI_SCK_GPIO_PORT			GPIOB                       /* GPIOB */
#define sLCD_SPI_SCK_GPIO_CLK			RCC_APB2Periph_GPIOB

#define sLCD_CD_PIN						GPIO_Pin_12					/* PB.12 */
#define sLCD_CD_GPIO_PORT				GPIOB                       /* GPIOB */
#define sLCD_CD_GPIO_CLK				RCC_APB2Periph_GPIOB

#define sLCD_SPI_MOSI_PIN				GPIO_Pin_15					/* PB.15 */
#define sLCD_SPI_MOSI_GPIO_PORT			GPIOB                       /* GPIOB */
#define sLCD_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOB

#define sLCD_RESET_PIN					GPIO_Pin_14                  /* PB.14 */
#define sLCD_RESET_GPIO_PORT			GPIOB                       /* GPIOB */
#define sLCD_RESET_GPIO_CLK				RCC_APB2Periph_GPIOB

#define sLCD_BL_PIN
#define sLCD_BL_GPIO_PORT
#define sLCD_BL_GPIO_CLK

#define sLCD_CD_COMMAND()				GPIO_ResetBits(sLCD_CD_GPIO_PORT, sLCD_CD_PIN)		// Command
#define sLCD_CD_DATA()					GPIO_SetBits(sLCD_CD_GPIO_PORT, sLCD_CD_PIN)		// Data

#define sLCD_SET_RST()					GPIO_ResetBits(sLCD_RESET_GPIO_PORT, sLCD_RESET_PIN)
#define sLCD_CLR_RST()					GPIO_SetBits(sLCD_RESET_GPIO_PORT, sLCD_RESET_PIN)

// DMA config
DMA_InitTypeDef    DMA_InitStructure;

#define SPI_MASTER_DMA               DMA1
#define SPI_MASTER_DMA_CLK           RCC_AHBPeriph_DMA1
#define SPI_MASTER_Tx_DMA_Channel    DMA1_Channel5
#define SPI_MASTER_Tx_DMA_FLAG       DMA1_FLAG_TC5
#define SPI_MASTER_DR_Base           0x4000380C

#define ST7565_LCD_BIAS         ST7565_LCD_BIAS_9
#define ST7565_ADC              ST7565_ADC_NORMAL
#define ST7565_COM_SCAN         ST7565_COM_SCAN_DEC
#define ST7565_PAGE_ORDER       0,1,2,3,4,5,6,7
#define GDISP_INITIAL_CONTRAST	20


static GFXINLINE void init_board(GDisplay *g) {
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;

	// As we are not using multiple displays we set g->board to NULL as we don't use it.
	g->board = 0;

	switch(g->controllerdisplay)
	{
		case 0:											// Set up for Display 0
		// 初始化SPI使用的管脚和时钟，以及额外的GPIO
		// 使用SPI2

		/*!< sLCD_SPI_CS_GPIO, sLCD_SPI_MOSI_GPIO, sLCD_SPI_MISO_GPIO
		 and sLCD_SPI_SCK_GPIO Periph clock enable */
		RCC_APB2PeriphClockCmd(
				sLCD_RESET_GPIO_CLK | sLCD_SPI_MOSI_GPIO_CLK | sLCD_CD_GPIO_CLK |
				sLCD_SPI_SCK_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

		/*!< sLCD_SPI Periph clock enable */
		RCC_APB1PeriphClockCmd(sLCD_SPI_CLK, ENABLE);

		/*!< Configure sLCD_SPI pins: SCK */
		GPIO_InitStructure.GPIO_Pin = sLCD_SPI_SCK_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(sLCD_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

		/*!< Configure sLCD_SPI pins: MOSI */
		GPIO_InitStructure.GPIO_Pin = sLCD_SPI_MOSI_PIN;
		GPIO_Init(sLCD_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

		/*!< Configure sLCD_SPI pins: CD */
		GPIO_InitStructure.GPIO_Pin = sLCD_CD_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(sLCD_CD_GPIO_PORT, &GPIO_InitStructure);

		/*!< Configure sLCD_RESET_PIN pin: sLCD Card RESET pin */
		GPIO_InitStructure.GPIO_Pin = sLCD_RESET_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(sLCD_RESET_GPIO_PORT, &GPIO_InitStructure);

		// LED control Pin (PWM)

		/*!< SPI configuration */
		SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
		SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
		SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
		SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
#else
		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
#endif

		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI_InitStructure.SPI_CRCPolynomial = 7;
		SPI_Init(sLCD_SPI, &SPI_InitStructure);

		// Initialize DMA for SPI
		RCC_AHBPeriphClockCmd(SPI_MASTER_DMA_CLK, ENABLE);

		/* SPI_MASTER_Tx_DMA_Channel configuration ---------------------------------*/
		DMA_DeInit(SPI_MASTER_Tx_DMA_Channel);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SPI_MASTER_DR_Base;
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;		//0
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_BufferSize = 0;//0
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		//DMA_Init(SPI_MASTER_Tx_DMA_Channel, &DMA_InitStructure);

		/*!< Enable the sLCD_SPI  */
		SPI_Cmd(sLCD_SPI, ENABLE);

		break;
	}

}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	if(state)
		sLCD_SET_RST();
	else
		sLCD_CLR_RST();
}

static GFXINLINE void set_backlight(GDisplay *g, uint8_t percent) {
	(void) g;
	(void) percent;

	// 设置背光亮度
}

static GFXINLINE void acquire_bus(GDisplay *g) {
	(void) g;
}

static GFXINLINE void release_bus(GDisplay *g) {
	(void) g;
}

static GFXINLINE void write_cmd(GDisplay *g, uint8_t cmd) {
	(void) g;

	sLCD_CD_COMMAND();

	/*!< Loop while DR register in not emplty */
	while (SPI_I2S_GetFlagStatus(sLCD_SPI, SPI_I2S_FLAG_TXE) == RESET);

	/*!< Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(sLCD_SPI, cmd);
}

static GFXINLINE void write_data(GDisplay *g, uint8_t* data, uint16_t length) {
	(void) g;

	uint16_t		i;

	sLCD_CD_DATA();

	// 写入数据为10以内时，使用pull方式（1个字节不能使用DMA）
	// 大于等于10字节的，使用DMA方式。
	if (length < 1000)
	{
		for (i = 0;  i < length; i++)
		{
			while (SPI_I2S_GetFlagStatus(sLCD_SPI, SPI_I2S_FLAG_TXE) == RESET);
			SPI_I2S_SendData(sLCD_SPI, data[i]);
		}
	}
	else
	{
		while (SPI_I2S_GetFlagStatus(sLCD_SPI, SPI_I2S_FLAG_TXE) == RESET);

		// 根据数据地址，重新初始化DMA通道
		DMA_DeInit(SPI_MASTER_Tx_DMA_Channel);
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)data;
		DMA_InitStructure.DMA_BufferSize = length;
		DMA_Init(SPI_MASTER_Tx_DMA_Channel, &DMA_InitStructure);

		// 启动SPI的DMA功能
		SPI_I2S_DMACmd(sLCD_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

		/* Enable DMA channels */
		DMA_Cmd(SPI_MASTER_Tx_DMA_Channel, ENABLE);

		// 等待DMA完成（不能异步处理，否则可能会导致command命令和数据并行发送）
		while(!DMA_GetFlagStatus(SPI_MASTER_Tx_DMA_FLAG))
		{
			taskYIELD();
		};

		// 完成DMA发送后，关闭SPI的DMA和DMA通道，等下次发送再次初始化开启
		SPI_I2S_DMACmd(sLCD_SPI, SPI_I2S_DMAReq_Tx, DISABLE);
		DMA_Cmd(SPI_MASTER_Tx_DMA_Channel, DISABLE);
	}
}

#endif /* _GDISP_LLD_BOARD_H */
