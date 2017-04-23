/*
 * tc_adc.c
 *
 *	1、测量24v设备的电流，监视是否停机
 *	2、监测备用电池充电电流和电压
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include <tc_adc.h>
#include "Msg.h"
#include "stm32f10x.h"

// ADC input
// 主电源电源（交流供电）
#define AC_ADC_GPIO           	GPIOC
#define AC_ADC_GPIO_CLK       	RCC_APB2Periph_GPIOC
#define AC_ADC_Pin            	GPIO_Pin_0

// 备用电池电源（4个磷酸铁锂电池）
#define BAT_ADC_GPIO          	GPIOC
#define BAT_ADC_GPIO_CLK       	RCC_APB2Periph_GPIOC
#define BAT_ADC_Pin            	GPIO_Pin_1

// 主电源电流
#define MAIN_CURRENT_ADC_GPIO		GPIOC
#define MAIN_CURRENT_ADC_GPIO_CLK   RCC_APB2Periph_GPIOC
#define MAIN_CURRENT_ADC_Pin       	GPIO_Pin_2

// 主泵电流
#define PUMP_CURRENT_ADC_GPIO		GPIOC
#define PUMP_CURRENT_ADC_GPIO_CLK   RCC_APB2Periph_GPIOC
#define PUMP_CURRENT_ADC_Pin        GPIO_Pin_4

// 蛋分电流
#define PS_CURRENT_ADC_GPIO			GPIOC
#define PS_CURRENT_ADC_GPIO_CLK   	RCC_APB2Periph_GPIOC
#define PS_CURRENT_ADC_Pin        	GPIO_Pin_5

// 存放DMA提取的值
__IO uint16_t ADCConvertedValues[5];

void InitADC()
{
	ADC_InitTypeDef 	ADC_InitStructure;
	GPIO_InitTypeDef 	GPIO_InitStructure;
	DMA_InitTypeDef 	DMA_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;

	/* PCLK2 is the APB2 clock */
	/* ADCCLK = PCLK2/6 = 72/6 = 12MHz*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	/* Enable ADC1 clock so that we can talk to it */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	/* Put everything back to power-on defaults */
	ADC_DeInit(ADC1);

	/* ADC1 Configuration ------------------------------------------------------*/
	/* ADC1 and ADC2 operate independently */
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	/* convert multiple channels */
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	/* Don't do contimuous conversions - do them on demand */
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	/* Start conversin by software, not an external trigger */
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	/* Conversions are 12 bit - put them in the lower 12 bits of the result */
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	/* Say how many channels would be used by the sequencer */
	ADC_InitStructure.ADC_NbrOfChannel = 5;

	/* Now do the setup */
	ADC_Init(ADC1, &ADC_InitStructure);

	// 初始化需要的ADC通道配置（后续基本不需要变更）
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_55Cycles5);		// AC-DC
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_55Cycles5);		// Battery
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_55Cycles5);		// Main Current
	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 4, ADC_SampleTime_55Cycles5);		// 蛋分电流？
	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 5, ADC_SampleTime_55Cycles5);		// 主泵电流？

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	//enable DMA for ADC
	ADC_DMACmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while (ADC_GetResetCalibrationStatus(ADC1))
		;
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while (ADC_GetCalibrationStatus(ADC1))
		;

	// 配置DMA1 Channel1做为ADC1的DMA
	/* Enable the DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	  /* Configure and enable DMA1 Channel interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* DMA1 channel1 configuration ----------------------------------------------*/
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ADCConvertedValues;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 5;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);

	/* Enable DMA1 channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);

	/* Enable the DMA Channels Interrupts */
	// TC:Transfer Complete, HT:Half Transfer, TE:Transfer Error interrupts
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

	// 配置ADC用的引脚
	// Enable GPIO Peripheral clock
	RCC_APB2PeriphClockCmd(AC_ADC_GPIO_CLK | BAT_ADC_GPIO_CLK | MAIN_CURRENT_ADC_GPIO_CLK | PUMP_CURRENT_ADC_GPIO_CLK | PS_CURRENT_ADC_GPIO_CLK, ENABLE);

	// Configure pin in output push/pull mode
	GPIO_InitStructure.GPIO_Pin = AC_ADC_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(AC_ADC_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BAT_ADC_Pin;
	GPIO_Init(BAT_ADC_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = MAIN_CURRENT_ADC_Pin;
	GPIO_Init(MAIN_CURRENT_ADC_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PUMP_CURRENT_ADC_Pin;
	GPIO_Init(PUMP_CURRENT_ADC_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PS_CURRENT_ADC_Pin;
	GPIO_Init(PS_CURRENT_ADC_GPIO, &GPIO_InitStructure);

}

void StartGetVoltage()
{
	//Start ADC1 Software Conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// 转换ACD数值到mV或者mA单位的整数电压值
void transformADCValue(uint16_t* pDCVoltage,
						   uint16_t* pBatteryVoltage,
						   uint16_t* pMainCurrent,
						   uint16_t* pProteinSkimmerCurrent,
						   uint16_t* pMainPumpCurrent)
{
	uint32_t	tmp;

	// DC Voltage（需要放大11倍）
	tmp = ADCConvertedValues[0];
	tmp = tmp * 2500 * 11 / 4096;		// 放大100倍进行计算，计算除0.01v为基础单位的数据
	*pDCVoltage = (uint16_t)tmp;

	// Battery Voltage（需要放大6倍）
	tmp = ADCConvertedValues[1];
	tmp = tmp * 2500 * 6 / 4096;
	*pBatteryVoltage = (uint16_t)tmp;

	// Main Current(mA)(185mV/A)
	tmp = ADCConvertedValues[2];
	if (tmp >= 4095)
	{
		// Overflow
		*pMainCurrent = 0;
	}
	else
	{
		tmp = tmp * 2500 / 4096;
		tmp = 2500 - tmp;
		tmp = tmp * 1000 / 185;
		*pMainCurrent = (uint16_t)tmp;
	}

	// 蛋分电流(mA)
	tmp = ADCConvertedValues[3];
	if (tmp >= 4095)
	{
		// Overflow
		*pProteinSkimmerCurrent = 0;
	}
	else
	{
		tmp = tmp * 2500 / 4096;
		tmp = 2500 - tmp;
		*pProteinSkimmerCurrent = (uint16_t)tmp;
	}

	// 主泵电流（mA）
	tmp = ADCConvertedValues[4];
	if (tmp >= 4095)
	{
		// Overflow
		*pMainPumpCurrent = 0;
	}
	else
	{
		tmp = tmp * 2500 / 4096;
		tmp = 2500 - tmp;
		*pMainPumpCurrent = (uint16_t)tmp;
	}
}



// DMA1 Channel1 IRQ
void DMA1_Channel1_IRQHandler(void)
{
	//Test on DMA1 Channel1 Transfer Complete interrupt
	if (DMA_GetITStatus(DMA1_IT_TC1))
	{
		// Notify main task to calculate/convert ADC value
		portBASE_TYPE 	xHigherPriorityTaskWoken = pdFALSE;
		Msg*			pMsg;

		pMsg = MallocMsgFromISR(&xHigherPriorityTaskWoken);
		if (pMsg)		// 消息分配不到，忽略。等下次重新进行ADC后，再通知
		{
			pMsg->Id = MSG_ADC_FINISHED;
			if (MSG_SEND_I(pMsg, &xHigherPriorityTaskWoken) != pdPASS)
			{
				FreeMsgFromISR(pMsg, &xHigherPriorityTaskWoken);
			}
		}

		//Clear DMA1 interrupt pending bits
		DMA_ClearITPendingBit(DMA1_IT_GL1);

		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
	}
}

