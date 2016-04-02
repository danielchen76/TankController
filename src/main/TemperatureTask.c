/*
 * TemperatureTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#include "TemperatureTask.h"
#include "Msg.h"
#include "util.h"
#include "tc_serial.h"
#include "tc_gpio.h"
#include "logTask.h"
#include "setting.h"
#include "TimerQueue.h"
#include "StateMachine.h"

#include <queue.h>
#include <string.h>
#include <stdlib.h>
#include <task.h>

#include "stm32f10x.h"

// 定义队列大小（温度控制用）
QueueHandle_t		temperature_queue;
const UBaseType_t 	uxTempQueueSize = 5;

void ProcessTemperature(void* pvParameters);

// 初始化消息队列
void InitTempMsgQueue( void )
{
	temperature_queue = xQueueCreate(uxTempQueueSize, sizeof(Msg*));
}

void MsgHeaterCtrl( Msg* msg )
{
	// Msg_Switch，加热棒还是用扩展参数做独立控制
}

void MsgChillerCtrl( Msg* msg )
{
	// Msg_Switch
}

static struEntry	Entries[] =
{
	{MSG_HEATER,		MsgHeaterCtrl},
	{MSG_CHILLER,		MsgChillerCtrl},
};

static void MsgProcess_entry( void )
{
	Msg* msg;
	unsigned int i;
	BaseType_t ret;

	ret = xQueueReceive(temperature_queue, &msg, pdMS_TO_TICKS(100));

	if (ret == pdPASS)
	{
		// 对消息进行处理
		for (i = 0; i < (sizeof(Entries) / sizeof(Entries[0])); i++)
		{
			if (Entries[i].MsgId == ((Msg*) msg)->Id)
			{
				Entries[i].Entry((Msg*) msg);
				break;
			}
		}

		// 完成处理后，将Msg设置为空闲
		FreeMsg(msg);
	}
}

// 温度监控任务使用的定时器
static struMyTimer			s_TempTimerArray[5];
static struMyTimerQueue		s_TempTimerQueue = {s_TempTimerArray, sizeof(s_TempTimerArray) / sizeof(s_TempTimerArray[0])};

static const TickType_t	c_TempCheckInterval		= pdMS_TO_TICKS(1000);		// 温度检测，每1秒一次

static TickType_t			s_NowTick;

// 当前记录的温度数据和简单状态
static int16_t				s_MainTemperature = 0;
static int16_t				s_SubTemperature = 0;

void InitTempControlTask( void )
{
	// 初始化Timer队列
	InitTimerQueue(&s_TempTimerQueue);

	// 添加Timer Callback
	AddTimer(&s_TempTimerQueue, xTaskGetTickCount(), c_TempCheckInterval, pdTRUE, ProcessTemperature, NULL);
	LogOutput(LOG_INFO, "1 second timer, for check temperature and run state machine.");
}

// 温度监视和控制
void TempControlTask( void * pvParameters )
{
	(void)pvParameters;

	// 初始化温度监控任务的数据
	InitTempControlTask();

	while (1)
	{
		// 处理消息
		MsgProcess_entry();

		s_NowTick = xTaskGetTickCount();

		// 定时器队列检查
		CheckTimerQueue(&s_TempTimerQueue, s_NowTick);
	}
}

// 定时器回调，用于处理定时任务
void ProcessTemperature(void* pvParameters)
{
	// 检测主缸和底缸温度

	// 判断是否需要启动加热棒/冷水机

	// 冷水机是否配置为自动控制（如果自动控制则不需要程序控制，一直开启）

	// 判断是否进入换水状态（抽水泵开始工作，就禁用加热棒）

}


