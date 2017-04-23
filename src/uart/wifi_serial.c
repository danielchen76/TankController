/*
 * wifi_serial.c
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include "stm32f10x.h"
#include "tc_serial.h"

// ESP8266-01 USART3
#define USARTwifi                   USART3
#define USARTwifi_GPIO              GPIOD
#define USARTwifi_CLK               RCC_APB1Periph_USART3
#define USARTwifi_GPIO_CLK          RCC_APB2Periph_GPIOD
#define USARTwifi_RxPin             GPIO_Pin_9
#define USARTwifi_TxPin             GPIO_Pin_8

// ESP8266-01 CH.PD (PE3)
#define WIFI_PD_GPIO           		GPIOE
#define WIFI_PD_GPIO_CLK       		RCC_APB2Periph_GPIOE
#define WIFI_PD_Pin            		GPIO_Pin_3

// ESP8266-01 RST (PE4)
#define WIFI_RST_GPIO           	GPIOE
#define WIFI_RST_GPIO_CLK       	RCC_APB2Periph_GPIOE
#define WIFI_RST_Pin            	GPIO_Pin_4


void InitWifiSerial()
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;

	/* Enable USART Clock */
	RCC_APB1PeriphClockCmd(USARTwifi_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(USARTwifi_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/*!< GPIO configuration */
	GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);

	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = USARTwifi_RxPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(USARTwifi_GPIO, &GPIO_InitStructure);

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = USARTwifi_TxPin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(USARTwifi_GPIO, &GPIO_InitStructure);

	// 初始化ESP8266的两个控制引脚
	RCC_APB2PeriphClockCmd(WIFI_PD_GPIO_CLK | WIFI_RST_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = WIFI_PD_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(WIFI_PD_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WIFI_RST_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(WIFI_RST_GPIO, &GPIO_InitStructure);

	// 初始化串口3，用于wifi模块（需要手工设置模块的波特率，默认是9600，需要手动调整到115200）
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =	USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure USART */
	USART_Init(USARTwifi, &USART_InitStructure);

	// 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART */
	USART_Cmd(USARTwifi, ENABLE);
}

// WIFI使用独立的task进行工作，用于接收指令，以及发送状态。所有这些都是基于“请求-应答”机制
// 请求超时后，延迟重发。有排队机制。
// 考虑加入“熔断”机制，在多次连续超时后，不再进行排队，而是由握手消息来执行连接检查。所有的日志、请求等，全部按失败处理。直到连接恢复


void USART3_IRQHandler(void)
{
	uint8_t		data;
	uint16_t 	sr = USARTwifi->SR;

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (sr & (USART_FLAG_RXNE | USART_FLAG_ORE))
	{
		data = USART_ReceiveData(USARTwifi);

		// data，接收到缓冲区，然后触发后半段数据分析

		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
	}
}

