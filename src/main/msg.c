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

static StaticSemaphore_t 		xSemaphoreBuffer;
static SemaphoreHandle_t		s_ArrayMutex;

void InitMsgArray()
{
	// 初始化消息数组
	for (uint8_t i = 0; i < TOTAL_MSG_NUMBER; ++i)
	{
		s_MsgArray[i].Id = MSG_UNUSED;
	}

	// 初始化互斥量
	//s_ArrayMutex = xSemaphoreCreateBinary();
	s_ArrayMutex = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer);

	// Default is empty state. Need Give once.
	xSemaphoreGive(s_ArrayMutex);
}

static Msg* GetBlankMsg()
{
	Msg*		pMsg = NULL;

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

	return pMsg;
}

// 从数组中获取一个空白的Msg
Msg* MallocMsg()
{
	Msg*		pMsg = NULL;

	// Lock
	xSemaphoreTake(s_ArrayMutex, portMAX_DELAY);

	pMsg = GetBlankMsg();

	// Unlock
	xSemaphoreGive(s_ArrayMutex);

	return pMsg;
}

Msg* MallocMsgFromISR(portBASE_TYPE* pWoken)
{
	Msg*		pMsg = NULL;

	// Lock
	xSemaphoreTakeFromISR(s_ArrayMutex, pWoken);

	pMsg = GetBlankMsg();

	// Unlock
	xSemaphoreGiveFromISR(s_ArrayMutex, pWoken);

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

void FreeMsgFromISR(Msg* pMsg, portBASE_TYPE* pWoken)
{
	assert_param(pMsg);
	// Lock
	xSemaphoreTakeFromISR(s_ArrayMutex, pWoken);

	pMsg->Id = MSG_UNUSED;
	s_MsgNumber--;

	// Unlock
	xSemaphoreGiveFromISR(s_ArrayMutex, pWoken);
}

