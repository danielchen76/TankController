/*
 * TimerQueue.c
 *
 *  Created on: Jan 16, 2016
 *      Author: daniel
 */

#include "TimerQueue.h"

#include "stm32f10x.h"
#include <string.h>
#include <task.h>

#include "util.h"

// 任务内定时器队列，只能单任务使用。每隔任务需要初始化自己使用的定时器队列
void InitTimerQueue(struMyTimerQueue* pTimerQueue)
{
	configASSERT(pTimerQueue->size > 0);

	// 全部清零
	memset(pTimerQueue->pQueue, 0, sizeof(struMyTimer) * pTimerQueue->size);
}

int16_t AddTimer(struMyTimerQueue* pTimerQueue, TickType_t tickNow, TickType_t timeout, BaseType_t bReload, TimerCallback_t pFunc, void* pvParameters)
{
	// 暂不考虑互斥问题，约束TimerQueue只能在单个任务中使用
	struMyTimer*	pTimer;

	for (uint8_t i = 0; i < pTimerQueue->size; i++)
	{
		pTimer = pTimerQueue->pQueue + i;
		if (pTimer->pFunc == NULL)
		{
			pTimer->tLastTick 		= tickNow;
			pTimer->timeout 		= timeout;
			pTimer->pFunc 			= pFunc;
			pTimer->bReload 		= bReload;
			pTimer->pvParameters 	= pvParameters;

			return (int16_t)i;
		}
	}

	configASSERT(pdFALSE);
	return -1;
}

void RemoveTimer(struMyTimerQueue* pTimerQueue, int16_t timerIndex)
{
	configASSERT(timerIndex < pTimerQueue->size);

	// 清除回调函数（其他的参数在add时都会重新赋值）
	pTimerQueue->pQueue[timerIndex].pFunc = NULL;
}

void UpdateTimer(struMyTimerQueue* pTimerQueue, int16_t timerIndex, TickType_t timeout, BaseType_t bReload, void* pvParameters)
{
	struMyTimer*	pTimer;
	configASSERT(timerIndex < pTimerQueue->size);

	pTimer = pTimerQueue->pQueue + timerIndex;
	if (pTimer->pFunc)
	{
		// 更新定时器参数
		pTimer->timeout 		= timeout;
		pTimer->bReload 		= bReload;
		pTimer->pvParameters 	= pvParameters;
	}
}

// 更新定时器，定时器重新计时（用于菜单超时等）
void ResetTimer(struMyTimerQueue* pTimerQueue, int16_t timerIndex)
{
	configASSERT(timerIndex < pTimerQueue->size);

	pTimerQueue->pQueue[timerIndex].tLastTick = xTaskGetTickCount();
}

// 检查Timer队列，对超时的部分进行回调执行
void CheckTimerQueue(struMyTimerQueue* pTimerQueue, TickType_t tickNow)
{
	struMyTimer*		pTimer;

	// 逐个比较
	for (uint8_t i = 0; i < pTimerQueue->size; i++)
	{
		pTimer = pTimerQueue->pQueue + i;

		// 判断定时器是否在使用
		if (!pTimer->pFunc)
		{
			continue;
		}

		if (IsTimeout(tickNow, pTimer->tLastTick, pTimer->timeout))
		{
			// 超时了，需要回调
			pTimer->pFunc(pTimer->pvParameters);

			// 如果是周期定时器，则需要重新设定Lasttick
			if (pTimer->bReload)
			{
				pTimer->tLastTick = tickNow;
			}
			else
			{
				pTimer->pFunc = NULL;
			}
		}
	}
}

