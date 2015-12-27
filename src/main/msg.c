/*
 * Msg.c
 *
 *  Created on: Dec 9, 2015
 *      Author: daniel
 */

#include "Msg.h"
#include <FreeRTOS.h>
#include <semphr.h>

#define TOTAL_MSG_NUMBER			32

static Msg		s_MsgArray[TOTAL_MSG_NUMBER];
static uint8_t	s_MsgInUsed[TOTAL_MSG_NUMBER];
static uint8_t	s_MsgNumber = 0;

static SemaphoreHandle_t		s_ArrayMutex;

void InitMsgArray()
{
	// 初始化消息数组
	for (uint8_t i = 0; i < TOTAL_MSG_NUMBER; ++i)
	{
		s_MsgInUsed[i] = 0;
	}

	// 初始化互斥量
	s_ArrayMutex = xSemaphoreCreateMutex();
}

// 从数组中获取一个空白的Msg
Msg* MallocMsg()
{
	Msg*		pMsg = NULL;

	// Lock
	xSemaphoreTake(s_ArrayMutex, portMAX_DELAY);

	for (uint8_t i = 0; i < TOTAL_MSG_NUMBER; ++i)
	{
		if (s_MsgInUsed[i] == 0)
		{
			s_MsgInUsed[i] = 1;
			pMsg = s_MsgArray + i;
			s_MsgNumber++;
			break;
		}
	}

	// Unlock
	xSemaphoreGive(s_ArrayMutex);

	return pMsg;
}

// 将使用后的Msg退回数组中，并标记为空闲状态
void FreeMsg(Msg* pMsg)
{
	// Lock
	xSemaphoreTake(s_ArrayMutex, portMAX_DELAY);

	for (uint8_t i = 0; i < TOTAL_MSG_NUMBER; ++i)
	{
		// TODO: 可以考虑优化这部分代码，以快速查找到Msg的位置
		if (pMsg == (s_MsgArray + i))
		{
			s_MsgInUsed[i] = 0;
			s_MsgNumber--;
			return;
		}
	}

	// Unlock
	xSemaphoreGive(s_ArrayMutex);
}


