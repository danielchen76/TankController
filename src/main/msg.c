/*
 * Msg.c
 *
 *  Created on: Dec 9, 2015
 *      Author: daniel
 */

#include "Msg.h"
#include <FreeRTOS.h>
#include <semphr.h>

#include "stm32f10x.h"

#define TOTAL_MSG_NUMBER			32

static Msg		s_MsgArray[TOTAL_MSG_NUMBER];
//static uint8_t	s_MsgInUsed[TOTAL_MSG_NUMBER];
static uint8_t	s_MsgNumber = 0;

static SemaphoreHandle_t		s_ArrayMutex;

void InitMsgArray()
{
	// 初始化消息数组
	for (uint8_t i = 0; i < TOTAL_MSG_NUMBER; ++i)
	{
		s_MsgArray[i].Id = MSG_UNUSED;
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
		if (s_MsgArray[i].Id == MSG_UNUSED)
		{
			s_MsgArray[i].Id = MSG_USED;
			pMsg = &s_MsgArray[i];
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
	assert_param(pMsg);

	// Lock
	xSemaphoreTake(s_ArrayMutex, portMAX_DELAY);

	pMsg->Id = MSG_UNUSED;
	s_MsgNumber--;

	// Unlock
	xSemaphoreGive(s_ArrayMutex);
}


