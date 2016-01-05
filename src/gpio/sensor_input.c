/*
 * sensor_input.c
 *
 *	水位紧急传感器
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include "tc_gpio.h"

void InitSensorsGPIO()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// PF口，全部输入，全部Pull UP（拉高）（除蓝牙连接指示灯为浮动）
	/* GPIOF Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);

	/* Configure PD0 and PD2 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	// 底缸紧急水位传感器（磁铁）

	// Debug开关（两位拨码开关）PF14 PF15

	// 旋转编码器（两个方向检测用，一个按键）

	// 淡水外部补充缸的水位探头（磁力浮子）（和底缸的安装方式相反）

	// 蓝牙连接状态指示灯（输入浮动？）
	// TODO:先测试，看后续结果
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	// 配置对应的中断

}

void InitRotaryEncoderGPIO()
{

}

// GPIO中断
void EXTI0_IRQHandler(void)
{

}

void EXTI1_IRQHandler(void)
{

}

void EXTI2_IRQHandler(void)
{

}

void EXTI3_IRQHandler(void)
{

}

void EXTI4_IRQHandler(void)
{

}

void EXTI9_5_IRQHandler(void)
{

}

void EXTI15_10_IRQHandler(void)
{

}

