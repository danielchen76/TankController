/*
 * WaterLevelTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */


#include "WaterLevelTask.h"
#include "Msg.h"

#include <queue.h>
#include <string.h>

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

// 当前水位
typedef struct
{
	uint16_t WaterLevelBuffers[10];		// 每500ms采样一个数据，5秒数据至少50个
	uint16_t BufferPos;					// 当前Buffer位置
	uint16_t WaterLevel_2s;				// 2s采样的平均值
	uint16_t WaterLevel_5s;				// 5s采样的平均值
} struWaterLevelData;

#define MAIN_TANK			0
#define SUB_TANK			1
#define RO_TANK				2

#define ULTRASOUND_NUM		3

static struWaterLevelData			s_WaterLeve[ULTRASOUND_NUM];			// 主缸/底缸/RO缸

// 临时补水量（临时目标水位）

void InitWaterLevelControlTask()
{
	for (uint8_t i = 0; i < ULTRASOUND_NUM; i++)
	{
		memset(s_WaterLeve[i].WaterLevelBuffers, 0, sizeof(s_WaterLeve[i].WaterLevelBuffers));

		s_WaterLeve[i].BufferPos = 0;

	}
}
//
void WaterLevelControlTask( void * pvParameters)
{
	// 初始化水位控制任务的所有数据
	InitWaterLevelControlTask();

	// 读取消息队列

	// 检测水位

	// 计算水位平均值

	// 根据当前状态进行处理
}

// 水位采集和N秒平均值的计算，保留5秒平均值 2秒平均值 瞬时值等。

// 正常运作（补充淡水）

// 停机（排出海水）

// 停机（加入海水）

// 启动过程（中间态）

// 另一条补水线路：RO水缸缺水（水少于1/2），就检测外部RO水缸是否有水，如果有水就启动补充RO水的流程。




