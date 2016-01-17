/*
 * TimerQueue.h
 *
 *  Created on: Jan 16, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_TIMERQUEUE_H_
#define INCLUDE_TIMERQUEUE_H_

#include <FreeRTOS.h>

// 时间超时后的回调函数
typedef void (*TimerCallback_t)(void* pvParameters);

// 自定义Timer的参数结构
typedef struct
{
	TickType_t		tLastTick;			// Timer启动时的Tick数（不能使用tick数为0作为是否有效判断，因为系统的tick会回转到0）
	TickType_t		timeout;			// 超时时间(tick)
	TimerCallback_t	pFunc;				// 超时回调（回调函数为NULL，也表示timer无效）
	void*			pvParameters;		// 回调函数函数携带的参数
	BaseType_t		bReload;			// 是否是周期定时器（周期定时器只能显式删除）
} struMyTimer;

typedef struct
{
	struMyTimer*	pQueue;
	uint8_t			size;
} struMyTimerQueue;

void InitTimerQueue(struMyTimerQueue* pTimerQueue);
int16_t AddTimer(struMyTimerQueue* pTimerQueue, TickType_t tickNow, TickType_t timeout, BaseType_t bReload, TimerCallback_t pFunc, void* pvParameters);
void RemoveTimer(struMyTimerQueue* pTimerQueue, int16_t timerIndex);

void CheckTimerQueue(struMyTimerQueue* pTimerQueue, TickType_t tickNow);

#endif /* INCLUDE_TIMERQUEUE_H_ */
