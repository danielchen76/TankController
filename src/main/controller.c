/*
 * controller.c
 *
 *	主处理任务，用于处理独立任务无法处理的联动工作（暂时没有看到有什么需要特别联动的），并将状态数据转发给GUI，以及记录到日志中
 *	检查电源状态，根据电源状态，进行主备电源切换，以及切换过程中的水泵、蛋分等的停止和重启（发送消息到对应任务），还有设置造浪在使用备用电源时，切换到省电模式
 *	控制主缸造浪、底缸造浪泵的启停
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include <controller.h>
#include "Msg.h"
#include "TimerQueue.h"
#include "StateMachine.h"
#include "tc_gpio.h"

#include <queue.h>
#include <task.h>

// 定义队列大小
QueueHandle_t		main_queue;
const UBaseType_t 	uxMainQueueSize = 20;

// Main主消息处理任务的时间定时器
static struMyTimer			s_MainTimerArray[5];
static struMyTimerQueue		s_MainTimerQueue = {s_MainTimerArray, sizeof(s_MainTimerArray) / sizeof(s_MainTimerArray[0])};

// 造浪泵（主缸、底缸）延迟10秒后启动，避免同时启动，影响电源
#define WAVEMAKER_DELAY		10000			// 10seconds
static int16_t				s_WaveMakerDelayIndex = -1;

// 初始化消息队列
void InitMainMsgQueue( void )
{
	main_queue = xQueueCreate(uxMainQueueSize, sizeof(Msg*));
}

void WaveMakerDelayCallback(void* pvParameters)
{
	BaseType_t	bOn = (BaseType_t)pvParameters;

	// Main tank wave pump
	Switch_WaveMaker(bOn);

	// Sub tank wave pump
	Switch_SubWaveMaker(bOn);

	s_WaveMakerDelayIndex = -1;
}

void WaveMaker(BaseType_t bOn, BaseType_t bDelay)
{
	// if delay
	if (bDelay)
	{
		// 造浪泵（主缸、底缸）延迟10秒后启动，避免同时启动，影响电源
		s_WaveMakerDelayIndex = AddTimer(&s_MainTimerQueue, xTaskGetTickCount(), pdMS_TO_TICKS(WAVEMAKER_DELAY), pdFALSE, WaveMakerDelayCallback, (void*)pdTRUE);
	}
	else
	{
		// Main tank wave pump
		Switch_WaveMaker(bOn);

		// Sub tank wave pump
		Switch_SubWaveMaker(bOn);

		if ((!bOn) && (s_WaveMakerDelayIndex >= 0))
		{
			// Check delay timer, cancel
			RemoveTimer(&s_MainTimerQueue, s_WaveMakerDelayIndex);
			s_WaveMakerDelayIndex = -1;
		}
	}
}


// 消息处理函数（多个）

void MsgTemp(Msg* msg)
{

}

void MsgMainWaterLevel(Msg* msg)
{

}

void MsgSubWaterLevel(Msg* msg)
{

}

void MsgRoWaterLevel(Msg* msg)
{

}


static struEntry	Entries[] =
{
	{MSG_MAIN_TANK_TEMP,		MsgTemp},
	{MSG_SUB_TANK_TEMP,			MsgTemp},
	{MSG_MAIN_WATER_LEVEL,		MsgMainWaterLevel},
	{MSG_SUB_WATER_LEVEL,		MsgSubWaterLevel},
	{MSG_RO_WATER_LEVEL,		MsgRoWaterLevel},
};

// 主处理循环，使用主线程（不需要独立开启一个线程）
void controller_entry(void)
{
	Msg*			msg;
	unsigned int	i;
	BaseType_t		ret;

	// 初始化定时器队列
	InitTimerQueue(&s_MainTimerQueue);

	// 启动一次性定时器，延迟启动造浪泵
	WaveMaker(pdTRUE, pdTRUE);

	// 循环读取队列，然后处理
	while(1) {
		ret = xQueueReceive(main_queue, &msg, pdMS_TO_TICKS(100));

		if (ret == pdPASS)
		{
			// 对消息进行处理
			for (i = 0; i < (sizeof(Entries) / sizeof(Entries[0])); i++)
			{
				if (Entries[i].MsgId == ((Msg*)msg)->Id)
				{
					Entries[i].Entry((Msg*)msg);
					break;
				}
			}

			// 完成处理后，将Msg设置为空闲
			FreeMsg(msg);
		}

		// 定时器队列检查
		CheckTimerQueue(&s_MainTimerQueue, xTaskGetTickCount());
	}
}


