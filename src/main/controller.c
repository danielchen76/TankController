/*
 * controller.c
 *
 *	主处理任务，用于处理独立任务无法处理的联动工作（暂时没有看到有什么需要特别联动的），并将状态数据转发给GUI，以及记录到日志中
 *	检查电源状态，根据电源状态，进行主备电源切换，以及切换过程中的水泵、蛋分等的停止和重启，还有设置造浪在使用备用电源时，切换到省电模式
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include <controller.h>
#include "Msg.h"
#include "TimerQueue.h"
#include "StateMachine.h"

#include <queue.h>
#include <task.h>

// 定义队列大小
QueueHandle_t		main_queue;
const UBaseType_t 	uxMainQueueSize = 20;

// Main主消息处理任务的时间定时器
static struMyTimer			s_MainTimerArray[5];
static struMyTimerQueue		s_MainTimerQueue = {s_MainTimerArray, sizeof(s_MainTimerArray) / sizeof(s_MainTimerArray[0])};

// 初始化消息队列
void InitMainMsgQueue( void )
{
	main_queue = xQueueCreate(uxMainQueueSize, sizeof(Msg*));
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


