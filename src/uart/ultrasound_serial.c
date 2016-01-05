/*
 * ultrasound_serial.c
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 *
 *      UltraSound sensor, for water level.
 */

#include "tc_serial.h"

#include "stm32f10x.h"

#include <task.h>

#define USARTwl                   USART2
#define USARTwl_GPIO              GPIOA
#define USARTwl_CLK               RCC_APB1Periph_USART2
#define USARTwl_GPIO_CLK          RCC_APB2Periph_GPIOA
#define USARTwl_RxPin             GPIO_Pin_3
#define USARTwl_TxPin             GPIO_Pin_2

// CD4052的A、B引脚
// Pin A
#define CD4052_A_GPIO           	GPIOE
#define CD4052_A_GPIO_CLK       	RCC_APB2Periph_GPIOE
#define CD4052_A_Pin            	GPIO_Pin_0

// Pin B
#define CD4052_B_GPIO           	GPIOE
#define CD4052_B_GPIO_CLK       	RCC_APB2Periph_GPIOE
#define CD4052_B_Pin            	GPIO_Pin_1

static uint8_t s_Buffer[2];			// 缓存两个字节的距离数据
static uint8_t s_BufferPos;

// 初始化超声波传感器使用的UART，和需要进行多路分配使用的GPIO口
void InitUltraSoundSensors(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;

	/* Enable USART Clock */
	RCC_APB1PeriphClockCmd(USARTwl_CLK, ENABLE);
	RCC_APB1PeriphClockCmd(USARTwl_GPIO_CLK, ENABLE);

	/*!< GPIO configuration */
	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = USARTwl_RxPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(USARTwl_GPIO, &GPIO_InitStructure);

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = USARTwl_TxPin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(USARTwl_GPIO, &GPIO_InitStructure);

	// 初始化模拟开关CD4052的控制引脚
	RCC_APB2PeriphClockCmd(CD4052_A_GPIO_CLK | CD4052_B_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = CD4052_A_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(CD4052_A_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = CD4052_B_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(CD4052_B_GPIO, &GPIO_InitStructure);

	// 初始化串口2，用于超声波距离测量
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure USART */
	USART_Init(USARTwl, &USART_InitStructure);

	// 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART */
	USART_Cmd(USARTwl, ENABLE);
}

BaseType_t GetDistance(uint8_t Port, uint16_t* pData)
{
	BitAction pinA, pinB;

	assert_param(pData);

	// 判断Port合法性
	if (Port > 3)
	{
		assert_param(pdFALSE);
		return pdFALSE;
	}

	// 切换模拟开关
	pinA = (Port & 0x01) ? Bit_SET : Bit_RESET;
	pinB = (Port & 0x02) ? Bit_SET : Bit_RESET;
	GPIO_WriteBit(CD4052_A_GPIO, CD4052_A_Pin, pinA);
	GPIO_WriteBit(CD4052_B_GPIO, CD4052_B_Pin, pinB);

	// 检查接收缓冲区是否有数据，如果有则需要清空（避免因为切换模拟开关带来干扰数据进入接收缓冲区）
	vTaskDelay(2 / portTICK_RATE_MS);			// 延迟2ms，等待切换后稳定

	if (USART_GetFlagStatus(USARTwl, USART_FLAG_RXNE) != RESET)
	{
		// 有遗留数据，清除标志位
		USART_ClearFlag(USARTwl, USART_FLAG_RXNE);
	}

	// 复位接收缓冲区的指针
	s_BufferPos = 0;

	USART_ITConfig(USARTwl, USART_IT_RXNE, ENABLE);

	// 发送测量距离命令
	USART_SendData(USARTwl, 0x55);
	/* Loop until USART DR register is empty */
	while (USART_GetFlagStatus(USARTwl, USART_FLAG_TXE) == RESET)
	{
		taskYIELD();
	}

	// 使用中断方式，然后控制在100ms内需要完成读取，否则按超时失败。读取两个字节的距离数据
	// 等待结果
	for (uint8_t i = 0; i < 100; i++)
	{
		vTaskDelay(1 / portTICK_RATE_MS);

		if (s_BufferPos == 2)
		{
			// 拼接数据，返回结果
			*pData  = s_Buffer[0] * 256 + s_Buffer[1];

			return pdTRUE;
		}
	}

	// 超时
	USART_ITConfig(USARTwl, USART_IT_RXNE, DISABLE);

	return pdFALSE;
}

void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USARTwl, USART_IT_RXNE) != RESET)
	{
		if (s_BufferPos < 2)
		{
			/* Read one byte from the receive data register */
			s_Buffer[s_BufferPos++] = USART_ReceiveData(USARTwl);
		}

		if (s_BufferPos >= 2)
		{
			/* Disable the USARTy Receive interrupt */
			USART_ITConfig(USARTwl, USART_IT_RXNE, DISABLE);
		}
	}
}
