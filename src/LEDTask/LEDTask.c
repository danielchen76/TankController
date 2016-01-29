/*
 * LEDTask.c
 *
 *  Created on: Dec 6, 2015
 *      Author: daniel
 */

#include <LEDTask.h>
#include "Msg.h"
#include <queue.h>
#include <task.h>

#include "stm32f10x.h"

#include "logTask.h"

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
#define LED_NORMAL_ON_TIME			2000
#define LED_NORMAL_OFF_TIME			50

#define LED_ERROR_ON_TIME			500
#define LED_ERROR_OFF_TIME			100

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

void SetDebugLED(BaseType_t bOn)
{
	bOn ? INTERNAL_LED_ON : INTERNAL_LED_OFF;
}

// 手动清除内部LED的告警状态
static BaseType_t cmd_clearErr( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	(void)pcCommandString;

	SetErrorLED(pdFALSE);
	snprintf(pcWriteBuffer, xWriteBufferLen, "Clear LED error state.\r\n");

	return pdFALSE;
}


const CLI_Command_Definition_t cmd_def_clearErr =
{
	"clearErr",
	"\r\nclearErr\r\n Clear internal LED error state.\r\n",
	cmd_clearErr, /* The function to run. */
	0
};
