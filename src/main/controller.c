/*
 * controller.c
 *
 *	主处理任务，用于处理独立任务无法处理的联动工作（暂时没有看到有什么需要特别联动的），并将状态数据转发给GUI，以及记录到日志中
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include "controller.h"
#include "Msg.h"

#include <queue.h>

// 定义队列大小
QueueHandle_t		main_queue;
const UBaseType_t 	uxMainQueueSize = 20;


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
	}
}


