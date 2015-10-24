/*
 * DS18B20.c
 *
 *  Created on: Apr 28, 2015
 *      Author: Daniel
 */

#include "DS18B20.h"

void initTIM5();
void Delay_us(uint16_t us);

BaseType_t DS18B20_Reset(uint8_t id);
void DS18B20_WriteBit0(uint8_t id);
void DS18B20_WriteBit1(uint8_t id);
uint8_t DS18B20_ReadBit(uint8_t id);
void DS18B20_WriteByte(uint8_t id, uint8_t udata);
uint8_t DS18B20_ReadByte(uint8_t id);

// 定义每个DS18B20的GPIO口
static struDS18B20_GPIO_t		s_DS18B20s[] =
{
		{GPIO_Pin_1, GPIOA, RCC_APB2Periph_GPIOA},
		{GPIO_Pin_2, GPIOA, RCC_APB2Periph_GPIOA}
};

static uint8_t		s_Scratchpad[9];

BaseType_t InitDS18B20()
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	uint8_t				i;

	// 初始化延时us用的定时器TIM5
	initTIM5();

	// 初始化所有的DS18B20
	for (i = 0; i < (sizeof(s_DS18B20s) / sizeof(s_DS18B20s[0])); ++i)
	{
		// 初始化DS18B20使用的管脚
		RCC_APB2PeriphClockCmd(s_DS18B20s[i].portclk, ENABLE);

		/*!< Configure DS18B20 DQ */
		GPIO_InitStructure.GPIO_Pin = s_DS18B20s[i].pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(s_DS18B20s[i].port, &GPIO_InitStructure);

		// Reset DS18B20
		if (DS18B20_Reset(i) != pdTRUE)
		{
			return pdFALSE;
		}

		// Init DS18B20
		DS18B20_WriteByte(i, DS18B20_COMMAND_SKIPROM);
		DS18B20_WriteByte(i, DS18B20_COMMAND_WRITESCRATCHPAD);
		DS18B20_WriteByte(i, 0x64);
		DS18B20_WriteByte(i, 0x8A);
		DS18B20_WriteByte(i, 0x7F);		// 12bit精度
	}

	return pdTRUE;
}

// 初始化TIM5，用于做us延迟使用
void initTIM5()
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	uint16_t PrescalerValue = 0;

	/* PCLK1 = HCLK/2 HCLK在不是1:1分频时，会自动做2倍频*/
	RCC_PCLK1Config(RCC_HCLK_Div4);

	// TIM5，使用1MHz时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	/*Compute the prescaler value */
	PrescalerValue = (uint16_t) (SystemCoreClock / 2000000) - 1;
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;

	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
}
void Delay_us(uint16_t us)
{
	TIM_SetCounter(TIM5, us);
	TIM_ClearFlag(TIM5, TIM_FLAG_Update);

	TIM_Cmd(TIM5, ENABLE);

	// 判断是否Underflow
	while (TIM_GetFlagStatus(TIM5, TIM_FLAG_Update) != SET)
	{
	}
}

BaseType_t DS18B20_Reset(uint8_t id)
{
	uint16_t		i;

	DS18B20_H(id);
	Delay_us(20); /*  10us延时  */
	DS18B20_L(id);
	Delay_us(550); /*  550us延时  */
	DS18B20_H(id);

	// 判断DS18B20是否在线（每隔1ms检查一次）
	for (i = 0; i < 1000; ++i)
	{
		if ( GPIO_ReadInputDataBit(s_DS18B20s[id].port, s_DS18B20s[id].pin) )
		{
			// 没有响应，继续等待
			Delay_us(100);
		}
		else
		{
			// 下啦，DS18B20响应
			Delay_us(500);		/* 500us延时 */
			DS18B20_H(id);

			return pdTRUE;
		}
	}

	// 超时没有响应，说明DS18B20没有响应
	return pdFALSE;
}

void DS18B20_WriteBit0(uint8_t id)
{
	DS18B20_H(id);
	Delay_us(1); /*  1us延时  */
	DS18B20_L(id);
	Delay_us(55); /*  55us延时  */
	DS18B20_H(id);
	Delay_us(1); /*  1us延时  */
}

void DS18B20_WriteBit1(uint8_t id)
{
	DS18B20_H(id);
	Delay_us(1); /*  1us延时  */
	DS18B20_L(id);
	Delay_us(5); /*  5us延时  */
	DS18B20_H(id);
	Delay_us(5); /*  5us延时  */
	Delay_us(50); /*  50us延时  */
}

uint8_t DS18B20_ReadBit(uint8_t id)
{
	uint8_t bdata;
	DS18B20_H(id);
	Delay_us(1); /*  1us延时  */
	DS18B20_L(id);
	Delay_us(4); /*  4us延时  */
	DS18B20_H(id);
	Delay_us(8); /*  8us延时  */
	bdata = GPIO_ReadInputDataBit(s_DS18B20s[id].port, s_DS18B20s[id].pin);
	Delay_us(60); /*  60us延时  */
	Delay_us(2); /*  2us延时  */
	return bdata;
}

void DS18B20_WriteByte(uint8_t id, uint8_t udata)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		if (udata & 0x01)
			DS18B20_WriteBit1(id);
		else
			DS18B20_WriteBit0(id);
		udata = udata >> 1;
	}
	Delay_us(10); /*  10us延时  */
}

uint8_t DS18B20_ReadByte(uint8_t id)
{
	uint8_t i, udata, j;
	udata = 0;
	for (i = 0; i < 8; i++)
	{
		udata = udata >> 1;
		j = DS18B20_ReadBit(id);
		if (j == 0x01)
			udata |= 0x80;
		else
			udata |= 0x00;
		Delay_us(2); /*  2us延时  */
	}
	return udata;
}

// 读取DS18B20温度，放大100倍。id为温度传感器编号（目前可以支持2个，主缸和底缸各一个）
int16_t GetTemperature(uint8_t id)
{
	uint8_t		i;
	uint16_t	temp;
	int32_t		temperature;

	DS18B20_Reset(id);
	DS18B20_WriteByte(id, DS18B20_COMMAND_SKIPROM);
	DS18B20_WriteByte(id, DS18B20_COMMAND_CONVERTT);

	// todo:需要更换为FreeRTOS的延迟函数，以便可以进行任务切换
	for (i = 0; i < 9; ++i)
	{
		Delay_us(50 * 1000);
		Delay_us(50 * 1000);
	}

	DS18B20_Reset(id);
	DS18B20_WriteByte(id, DS18B20_COMMAND_SKIPROM);
	DS18B20_WriteByte(id, DS18B20_COMMAND_READSCRATCHPAD);

	for (i = 0; i < sizeof(s_Scratchpad) / sizeof(s_Scratchpad[0]); ++i)
	{
		s_Scratchpad[i] = DS18B20_ReadByte(id);
	}

	// todo:CRC校验

	temp = s_Scratchpad[1] << 8;
	temp += s_Scratchpad[0];

	// 转换为摄氏度，并放大100倍
	temperature = (int16_t)temp;		// 先进行符号位扩展
	temperature = temperature * 625 / 100;

	// todo:看看是否需要进行四舍五入的处理？

	return (int16_t)temperature;
}

