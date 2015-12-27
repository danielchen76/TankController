/*
 * WaterLevelTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */


#include "WaterLevelTask.h"
#include "Msg.h"

#include <queue.h>

// 定义队列大小（温度控制用）
QueueHandle_t		waterlevel_queue;
const UBaseType_t 	uxWaterLevelQueueSize = 10;			// 考虑到会手工控制抽水和补水，所以将消息队列加大


void InitWaterLevelMsgQueue( void )
{
	waterlevel_queue = xQueueCreate(uxWaterLevelQueueSize, sizeof(Msg*));
}

void MsgRoPumpSwitch(Msg* msg)
{

}

static struEntry	Entries[] =
{
	{MSG_MAIN_TANK_TEMP,		MsgRoPumpSwitch},

};

void MsgProcess_entry(void)
{
	Msg*			msg;
	unsigned int	i;
	BaseType_t		ret;

	// 循环读取队列，然后处理
	while(1) {
		ret = xQueueReceive(waterlevel_queue, &msg, pdMS_TO_TICKS(100));

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

void WaterLevelControlTask( void * pvParameters)
{

}




