/*
 * TemperatureTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#include "TemperatureTask.h"
#include "Msg.h"

#include <queue.h>

// 定义队列大小（温度控制用）
QueueHandle_t		temp_queue;
const UBaseType_t 	uxTempQueueSize = 5;

// 初始化消息队列
void InitTempMsgQueue( void )
{
	temp_queue = xQueueCreate(uxTempQueueSize, sizeof(Msg*));
}

// 温度监视和控制
void TempControlTask( void * pvParameters)
{

}
