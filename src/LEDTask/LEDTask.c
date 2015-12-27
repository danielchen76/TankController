/*
 * LEDTask.c
 *
 *  Created on: Dec 6, 2015
 *      Author: daniel
 */

#include "LEDTask.h"
#include "Msg.h"
#include <queue.h>
#include <task.h>

#include "stm32f10x.h"
#include "..\uart\FreeRTOS_CLI.h"

// 定义队列大小
QueueHandle_t		led_queue;
const UBaseType_t 	uxLEDQueueSize = 5;

// 板载LED在PB9上
#define LED_INTERNAL_GPIO           GPIOB
#define LED_INTERNAL_GPIO_CLK       RCC_APB2Periph_GPIOB
#define LED_INTERNAL_Pin            GPIO_Pin_9

#define INTERNAL_LED_OFF			GPIO_ResetBits(LED_INTERNAL_GPIO, LED_INTERNAL_Pin)
#define INTERNAL_LED_ON				GPIO_SetBits(LED_INTERNAL_GPIO, LED_INTERNAL_Pin)

static BaseType_t					s_bError = pdFALSE;

// 单位：ms
#define LED_NORMAL_ON_TIME			1000
#define LED_NORMAL_OFF_TIME			1000

#define LED_ERROR_ON_TIME			300
#define LED_ERROR_OFF_TIME			300

void InitLEDTask( void )
{
	// 创建LED指示灯使用的队列，用于接收来自其他Task的通知消息，根据通知消息的内容进行不同优先级来显示LED灯的状态
	led_queue = xQueueCreate(uxLEDQueueSize, sizeof(Msg*));

	// 初始化LED引脚
	GPIO_InitTypeDef 	GPIO_InitStructure;

	// Enable GPIO Peripheral clock
	RCC_APB2PeriphClockCmd(LED_INTERNAL_GPIO_CLK, ENABLE);

	// Configure pin in output push/pull mode
	GPIO_InitStructure.GPIO_Pin = LED_INTERNAL_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_INTERNAL_GPIO, &GPIO_InitStructure);
}

void LEDTask( void * pvParameters)
{
	(void)pvParameters;

	uint16_t		usON = LED_NORMAL_ON_TIME;
	uint16_t		usOFF = LED_NORMAL_OFF_TIME;

	do
	{
		if (s_bError)
		{
			usON = LED_ERROR_ON_TIME;
			usOFF = LED_ERROR_OFF_TIME;
		}
		else
		{
			usON = LED_NORMAL_ON_TIME;
			usOFF = LED_NORMAL_OFF_TIME;
		}

		vTaskDelay(usON / portTICK_RATE_MS);
		INTERNAL_LED_ON;

		vTaskDelay(usOFF / portTICK_RATE_MS);
		INTERNAL_LED_OFF;
	} while (1);
}


void SetErrorLED(BaseType_t bError)
{
	// 设置LED灯告警状态（正常：慢闪；异常/有错误：快闪）
	s_bError = bError;
}


