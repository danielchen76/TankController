/*
 * TemperatureTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#include "Msg.h"
#include "util.h"
#include "tc_serial.h"
#include "tc_gpio.h"
#include "logTask.h"
#include "setting.h"
#include "TimerQueue.h"
#include "StateMachine.h"

#include <queue.h>
#include <string.h>
#include <stdlib.h>
#include <task.h>
#include <TemperatureTask.h>
#include "ds18b20/DS18B20.h"

#include "stm32f10x.h"

// 当前记录的温度数据和简单状态
static int16_t				s_MainTemperature = 0;
static int16_t				s_SubTemperature = 0;

// 设备是否自动由程序控制
static uint8_t				s_bHeaterAuto = 0;			// 加热棒默认不自动控制
static uint8_t				s_bChillerAuto = 0;

// 设备是否开启
static uint8_t				s_bHeaterOn = 0;				// 加热棒默认不开启
static uint8_t				s_bChillerOn = 0;

// 定义队列大小（温度控制用）
QueueHandle_t		temperature_queue;
const UBaseType_t 	uxTempQueueSize = 5;

void ProcessTemperature(void* pvParameters);

// 初始化消息队列
void InitTempMsgQueue( void )
{
	temperature_queue = xQueueCreate(uxTempQueueSize, sizeof(Msg*));
}

void MsgHeaterCtrl( Msg* msg )
{
	// Msg_Switch，加热棒还是用扩展参数做独立控制
	switch (msg->Param.Switch.param)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
	if (msg->Param.Switch.bOn)
	{
	}
}

void MsgChillerCtrl( Msg* msg )
{
	// Msg_Switch
}

void MsgHeaterMode( Msg* msg)
{
	// Msg_DeviceEnable
	s_bHeaterAuto = (uint8_t)msg->Param.Mode.bAuto;
}

void MsgChillerMode( Msg* msg)
{
	// Msg_DeviceEnable
	s_bChillerAuto = (uint8_t)msg->Param.Mode.bAuto;
}

static struEntry	Entries[] =
{
	{MSG_HEATER,		MsgHeaterCtrl},
	{MSG_CHILLER,		MsgChillerCtrl},
	{MSG_HEATER_MODE,	MsgHeaterMode},
	{MSG_CHILLER_MODE,	MsgChillerMode}
};

static void MsgProcess_entry( void )
{
	Msg* msg;
	unsigned int i;
	BaseType_t ret;

	ret = xQueueReceive(temperature_queue, &msg, pdMS_TO_TICKS(100));

	if (ret == pdPASS)
	{
		// 对消息进行处理
		for (i = 0; i < (sizeof(Entries) / sizeof(Entries[0])); i++)
		{
			if (Entries[i].MsgId == ((Msg*) msg)->Id)
			{
				Entries[i].Entry((Msg*) msg);
				break;
			}
		}

		// 完成处理后，将Msg设置为空闲
		FreeMsg(msg);
	}
}

// 温度监控任务使用的定时器
static struMyTimer			s_TempTimerArray[5];
static struMyTimerQueue		s_TempTimerQueue = {s_TempTimerArray, sizeof(s_TempTimerArray) / sizeof(s_TempTimerArray[0])};

static const TickType_t	c_TempCheckInterval		= pdMS_TO_TICKS(1000);		// 温度检测，每1秒一次

static TickType_t			s_NowTick;


void InitTempControlTask( void )
{
	// 初始化Timer队列
	InitTimerQueue(&s_TempTimerQueue);

	// 添加Timer Callback（定时检查水温，并控制加热棒或冷水机是否启动）
	AddTimer(&s_TempTimerQueue, xTaskGetTickCount(), c_TempCheckInterval, pdTRUE, ProcessTemperature, NULL);
	LogOutput(LOG_INFO, "1 second timer, for check temperature and run state machine.");
}

// 温度监视和控制
void TempControlTask( void * pvParameters )
{
	(void)pvParameters;

	// 初始化温度监控任务的数据
	InitTempControlTask();

	while (1)
	{
		// 处理消息
		MsgProcess_entry();

		s_NowTick = xTaskGetTickCount();

		// 定时器队列检查
		CheckTimerQueue(&s_TempTimerQueue, s_NowTick);
	}
}

// 定时器回调，用于处理定时任务
void ProcessTemperature(void* pvParameters)
{
	int16_t		mainTemp, subTemp;
	BaseType_t	bChanged = pdFALSE;

	uint16_t	usHighTemp 	= Get_usHighTemperature();
	uint16_t	usLowTemp	= Get_usLowTemperature();
	uint16_t	usOffset	= Get_usTemperatureOffset();

	// 检测主缸和底缸温度
	if (!GetTemperature(MAIN_TANK_TEMP, &mainTemp))
	{
		mainTemp = 0;
	}

	if (!GetTemperature(SUB_TANK_TEMP, &subTemp))
	{
		subTemp = 0;
	}

	// 判断是否温度是否有变化。有变化，更新数据，同时发送通知消息
	if ( s_MainTemperature != mainTemp )
	{
		bChanged = pdTRUE;
		s_MainTemperature = mainTemp;
	}

	if ( s_SubTemperature != subTemp )
	{
		bChanged = pdTRUE;
		s_SubTemperature = subTemp;
	}

	if ( bChanged )
	{
		// 通知温度变更
	}

	// 判断主缸和底缸的温度差异
	// 如果差异太大，则告警。
	// 因爲加熱棒在底缸,所以溫度以底缸爲主。

	// 判断是否需要启动加热棒/冷水机
	if (s_bChillerAuto)
	{
		if (s_bChillerOn)
		{
			if ( s_SubTemperature <= (int16_t)(usHighTemp - usOffset) )
			{
				// 关闭冷水机
			}
		}
		else
		{
			if ( s_SubTemperature >= (int16_t)usHighTemp )
			{
				// 启动冷水机
			}
		}
	}

	if (s_bHeaterAuto)
	{
		if (s_bHeaterOn)
		{
			if ( s_SubTemperature >= (int16_t)(usLowTemp + usOffset) )
			{
				// 关闭加热棒
			}
		}
		else
		{
			if ( s_SubTemperature <= (int16_t)usLowTemp )
			{
				// 开启加热棒
			}
		}
	}

	// 判断是否进入换水状态（抽水泵开始工作，就禁用加热棒）

}





// -------------------------------Command line--------------------------------------------
static BaseType_t cmd_gettemp( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	char			WriteBuffer[50];
	(void)pcCommandString;

	// temperature
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Main Tank:%d.%02d(%d)\r\n", s_MainTemperature / 100, s_MainTemperature % 100, s_MainTemperature);
	strncpy(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Sub Tank:%d.%02d(%d)\r\n", s_SubTemperature / 100, s_SubTemperature % 100, s_SubTemperature);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);

	// Chiller
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Chiller auto {%s}, switch {%s}\r\n",
			s_bChillerAuto ? "Auto" : "Manual", s_bChillerOn ? "On" : "Off");
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);

	// Heater
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Heater auto {%s}, switch {%s}\r\n",
			s_bHeaterAuto ? "Auto" : "Manual", s_bHeaterOn ? "On" : "Off");
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_gettemp =
{
	"temp",
	"\r\ntemp\r\n Get temp.\r\n",
	cmd_gettemp, /* The function to run. */
	0
};

static BaseType_t cmd_chiller( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_chiller =
{
	"chiller",
	"\r\nchiller <auto | manual> <on | off>\r\n Set chiller work mode.\r\n",
	cmd_chiller, /* The function to run. */
	2
};

static BaseType_t cmd_heater( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_heater =
{
	"chiller",
	"\r\ntheater <auto | manual> <on | off>\r\n Set heater work mode.\r\n",
	cmd_heater, /* The function to run. */
	2
};





