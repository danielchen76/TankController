/*
 * controller.c
 *
 *	主处理任务，用于处理独立任务无法处理的联动工作（暂时没有看到有什么需要特别联动的），并将状态数据转发给GUI，以及记录到日志中
 *	检查电源状态，根据电源状态，进行主备电源切换，以及切换过程中的水泵、蛋分等的停止和重启（发送消息到对应任务），还有设置造浪在使用备用电源时，切换到省电模式
 *	控制主缸造浪、底缸造浪泵的启停
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include <controller.h>
#include "Msg.h"
#include "TimerQueue.h"
#include "StateMachine.h"
#include "tc_gpio.h"
#include "tc_rtc.h"

#include <queue.h>
#include <task.h>
#include <string.h>
#include <stdlib.h>

// 定义队列大小
QueueHandle_t		main_queue;
const UBaseType_t 	uxMainQueueSize = 20;

// Main主消息处理任务的时间定时器
static struMyTimer			s_MainTimerArray[5];
static struMyTimerQueue		s_MainTimerQueue = {s_MainTimerArray, sizeof(s_MainTimerArray) / sizeof(s_MainTimerArray[0])};

// 造浪泵（主缸、底缸）延迟10秒后启动，避免同时启动，影响电源
#define WAVEMAKER_DELAY		10000			// 10seconds
static int16_t				s_WaveMakerDelayIndex = -1;

// 初始化消息队列
void InitMainMsgQueue( void )
{
	main_queue = xQueueCreate(uxMainQueueSize, sizeof(Msg*));
}

void WaveMakerDelayCallback(void* pvParameters)
{
	BaseType_t	bOn = (BaseType_t)pvParameters;

	// Main tank wave pump
	Switch_WaveMaker(bOn);

	// Sub tank wave pump
	Switch_SubWaveMaker(bOn);

	s_WaveMakerDelayIndex = -1;
}

void WaveMaker(BaseType_t bOn, BaseType_t bDelay)
{
	// if delay
	if (bDelay)
	{
		// 造浪泵（主缸、底缸）延迟10秒后启动，避免同时启动，影响电源
		s_WaveMakerDelayIndex = AddTimer(&s_MainTimerQueue, xTaskGetTickCount(), pdMS_TO_TICKS(WAVEMAKER_DELAY), pdFALSE, WaveMakerDelayCallback, (void*)pdTRUE);
	}
	else
	{
		// Main tank wave pump
		Switch_WaveMaker(bOn);

		// Sub tank wave pump
		Switch_SubWaveMaker(bOn);

		if ((!bOn) && (s_WaveMakerDelayIndex >= 0))
		{
			// Check delay timer, cancel
			RemoveTimer(&s_MainTimerQueue, s_WaveMakerDelayIndex);
			s_WaveMakerDelayIndex = -1;
		}
	}
}


// 消息处理函数（多个）

static void MsgTemp(Msg* msg)
{

}

static void MsgMainWaterLevel(Msg* msg)
{

}

static void MsgSubWaterLevel(Msg* msg)
{

}

static void MsgRoWaterLevel(Msg* msg)
{

}

static void MsgStopAllPump(Msg* msg)
{
}


static struEntry	Entries[] =
{
	{MSG_MAIN_TANK_TEMP,		MsgTemp},
	{MSG_SUB_TANK_TEMP,			MsgTemp},
	{MSG_MAIN_WATER_LEVEL,		MsgMainWaterLevel},
	{MSG_SUB_WATER_LEVEL,		MsgSubWaterLevel},
	{MSG_RO_WATER_LEVEL,		MsgRoWaterLevel},
	{MSG_STOP_ALL_PUMP, 		MsgStopAllPump},
};

// 主处理循环，使用主线程（不需要独立开启一个线程）
void controller_entry(void)
{
	Msg*			msg;
	unsigned int	i;
	BaseType_t		ret;

	// 初始化定时器队列
	InitTimerQueue(&s_MainTimerQueue);

	// 启动一次性定时器，延迟启动造浪泵
	WaveMaker(pdTRUE, pdTRUE);

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






















// -------------------------------------Command line-------------------------------------
static BaseType_t cmd_sys( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *	pcParameter;
	BaseType_t 		xParameterStringLength;

	// 分析第一个参数
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	configASSERT( pcParameter );
	if (strncasecmp(pcParameter, "stop", xParameterStringLength) == 0)
	{
		// 停机命令

		snprintf(pcWriteBuffer, xWriteBufferLen, "System stop.\r\n");
	}
	else if (strncasecmp(pcParameter, "start", xParameterStringLength) == 0)
	{
		// 启动命令

		snprintf(pcWriteBuffer, xWriteBufferLen, "System start.\r\n");
	}
	else if (strncasecmp(pcParameter, "pause5", xParameterStringLength) == 0)
	{
		// 暂停5分钟

		snprintf(pcWriteBuffer, xWriteBufferLen, "System pause 5 minutes.\r\n");
	}
	else if (strncasecmp(pcParameter, "pause10", xParameterStringLength) == 0)
	{
		// 暂停10分钟

		snprintf(pcWriteBuffer, xWriteBufferLen, "System pause 10 minutes.\r\n");
	}
	else
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "Wrong system command!\r\n");
	}

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_sys =
{
	"sys",
	"\r\nsys <stop|pause5|pause10|start>\r\n System control command.\r\n",
	cmd_sys, /* The function to run. */
	1
};

static BaseType_t cmd_backuppower( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *	pcParameter;
	BaseType_t 		xParameterStringLength;

	// 分析第一个参数
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	configASSERT( pcParameter );
	if (strncasecmp(pcParameter, "on", xParameterStringLength) == 0)
	{
		// 切换到备用电源
		Swtich_BackupPower(POWER_ON);
		Switch_ToBackupPower(POWER_ON);

		snprintf(pcWriteBuffer, xWriteBufferLen, "Backup power switch on.\r\n");
	}
	else if (strncasecmp(pcParameter, "off", xParameterStringLength) == 0)
	{
		// 关闭备用电源
		Swtich_BackupPower(POWER_OFF);
		Switch_ToBackupPower(POWER_OFF);

		snprintf(pcWriteBuffer, xWriteBufferLen, "Backup power switch off.\r\n");
	}
	else
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "Wrong backuppower command!\r\n");
	}

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_backuppower =
{
	"backuppower",
	"\r\nbackuppower <on|off>\r\n Switch on/off backup power.\r\n",
	cmd_backuppower, /* The function to run. */
	1
};

static BaseType_t cmd_uptime( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	(void)pcCommandString;
	uint32_t		uptime, day, hour, minute, second;

	uptime = GetUptime(&day, &hour, &minute, &second);

	// 输出当前运行时间长度
	snprintf(pcWriteBuffer, xWriteBufferLen, "System uptime:%lu Day %lu Hour %lu Minute %lu Second\r\n", day, hour, minute, second);

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_uptime =
{
	"uptime",
	"\r\nuptime\r\n Show system uptime.\r\n",
	cmd_uptime, /* The function to run. */
	0
};
