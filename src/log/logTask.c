/*
 * logTask.c
 *
 *  Created on: Jan 2, 2016
 *      Author: daniel
 */

#include "logTask.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <queue.h>
#include <semphr.h>

#include "stm32f10x.h"

#include <spiffs.h>
#include "tc_serial.h"

// 定义队列大小（日志任务用）
QueueHandle_t		log_queue;

#define LOG_RECORD_SIZE		100				// 每条记录长度
#define LOG_BUFFER_NUM		10				// 总共缓存记录数（太多了就需要丢弃，并记录一条丢弃记录）

typedef enum
{
	LOG_UNUSED,
	LOG_USED,

	LOG_LOG_TEXT,
	LOG_WATERLEVEL,
	LOG_TEMPERATURE,
} enumLogMsg;

typedef union
{
	struct Log_Text
	{
		enumLogLevel	level;
		char		cLog[LOG_RECORD_SIZE];
	} LogText;

	struct Log_WaterLevel
	{
		uint16_t	usMainWaterLevel;
		uint16_t	usSubWaterLevel;
		uint16_t	usRoWaterLevel;
	} LogWaterLevel;

	struct Log_Temperature
	{
		int16_t		sTemperature;			// 环境温度
		int16_t		sHumidity;				// 环境湿度

		int16_t		sMainTemperature;
		int16_t		sSubTemperature;
	} LogTemperature;
} Log_Param;

typedef struct
{
	enumLogMsg		LogType;
	uint32_t		TimeStamp;				// UTC时间（需要注意处理当前为北京时间，东八区）
	Log_Param		LogParam;
} LogMsg;

// 缓存日志消息（包括文本日志和其他需要记录的数据日志）
static LogMsg		s_LogMsgs[LOG_BUFFER_NUM];
static LogMsg		s_FullLog;				// 固定用来生成溢出Log

const uint8_t		s_LogMsgNum = sizeof(s_LogMsgs) / sizeof(s_LogMsgs[0]);

static SemaphoreHandle_t		s_LogMutex;

static uint8_t		s_logout = 0;

void InitLogTask()
{
	// 多预留一条，用于写入溢出的消息
	log_queue = xQueueCreate(LOG_BUFFER_NUM + 1, sizeof(LogMsg*));

	// 保护日志消息数组的互斥量
	s_LogMutex = xSemaphoreCreateMutex();

	for (uint8_t i = 0; i < s_LogMsgNum; i++)
	{
		s_LogMsgs[i].LogType = LOG_UNUSED;
	}
}

// 内部函数，在互斥量内使用，不得外用
void LogMsgFull()
{
	LogMsg*		pLog;
	
	// 向队列插入full（如果full消息没有在使用）
	// 这个只是记录有过full，如果一直很满，那么也无法准确记录。这个只是记录曾经full过，需要寻找问题根源
	if (s_FullLog.LogType == LOG_UNUSED)
	{
		s_FullLog.LogType = LOG_LOG_TEXT;

		// timestamp
		s_FullLog.TimeStamp = RTC_GetCounter();

		// level
		s_FullLog.LogParam.LogText.level = LOG_ERROR;

		// text
		snprintf(s_FullLog.LogParam.LogText.cLog, LOG_RECORD_SIZE, "Log full!");

		// 放入消息队列
		pLog = &s_FullLog;
		xQueueSendToBack(log_queue, (void*)&pLog, portMAX_DELAY);
	}
}

LogMsg* MallocLogMsg()
{
	LogMsg*		pLog	= NULL;

	xSemaphoreTake(s_LogMutex, portMAX_DELAY);

	for (uint8_t i = 0; i < s_LogMsgNum; i++)
	{
		if (s_LogMsgs[i].LogType == LOG_UNUSED)
		{
			pLog = &s_LogMsgs[i];
			pLog->LogType = LOG_USED;
			break;
		}
	}

	if (!pLog)
	{
		// 没有空闲空间，说明Log消息满了
		// 判断Full专用消息是否在使用，没有用，就记录一条full
		LogMsgFull();
	}

	xSemaphoreGive(s_LogMutex);

	return pLog;
}

void FreeLogMsg(LogMsg* pMsg)
{
	configASSERT(pMsg);

	xSemaphoreTake(s_LogMutex, portMAX_DELAY);

	pMsg->LogType = LOG_UNUSED;

	xSemaphoreGive(s_LogMutex);
}

void LogOutput(enumLogLevel level, const char* pLog, ...)
{
	LogMsg*		pLogMsg;

	pLogMsg = MallocLogMsg();

	if (pLogMsg)
	{
		// timestamp
		pLogMsg->TimeStamp = RTC_GetCounter();

		// Log level
		pLogMsg->LogParam.LogText.level = level;

		// text
		va_list		args;
		va_start(args, pLog);
		vsnprintf(pLogMsg->LogParam.LogText.cLog, LOG_RECORD_SIZE, pLog, args);
		va_end(args);

		pLogMsg->LogType = LOG_LOG_TEXT;

		// 放入队列中
		xQueueSendToBack(log_queue, (void*)&pLogMsg, portMAX_DELAY);
	}
}

// 记录温度和湿度
void LogTemperature(int16_t sTemperature, int16_t sHumidity, int16_t sMainT, int16_t sSubT)
{

}

// 记录水位数据
void LogWaterLevel(uint16_t usMainWL, uint16_t usSubWL, uint16_t usRoWL)
{

}

void OutputLogs(const char* pLog)
{
	if (s_logout != 0)
	{
		vOutputString(pLog);
	}
}

static char		s_LogText[LOG_RECORD_SIZE + 23 + 2];		// 包含“\r\n”
static int			s_FileNo = -1;
static spiffs_file	s_LogFile = 0;

static char		s_TempDate[11];

extern	spiffs fs;

void GetLogFile(int mon, int mday)
{
	char	szFileName[4];

	if ((s_FileNo < 0) || (mday != s_FileNo))
	{
		// 日期变化，需要更换文件
		if (s_LogFile > 0)
		{
			// 先关闭文件
			SPIFFS_close(&fs, s_LogFile);
		}

		s_FileNo = mday;

		// 打开新文件
		sprintf(szFileName, "L%02d", mday);
		s_LogFile = SPIFFS_open(&fs, szFileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);

		// 读取文件前面的时间，判断该文件是否是今天的，如果不是则清除（关闭，然后SPIFFS_TRUNC打开）
		int ret = SPIFFS_read(&fs, s_LogFile, (uint8_t*)s_TempDate, 10);

		if (ret == 10)
		{
			// 分析月份和日期，年份不管（日志存储一个月）
		}
	}
}

void WriteLogs(LogMsg* pLog)
{
	struct tm		ts;
	time_t			t_sec;
	char*			pLevel;

	// 下面是Log的例子，时间和级别分别使用固定长度字符串
//2016-01-01 12:36:30 I Ro water full!
//2016-01-01 12:36:31 I Ro backup pum switch off.
//2016-01-01 12:36:31 W
//2016-01-01 12:36:31 E WaveMaker stop, will restart!

	// 转换时间，并判断是否需要切换文件
	t_sec = pLog->TimeStamp;
	localtime_r(&t_sec, &ts);
	switch (pLog->LogParam.LogText.level)
	{
	case LOG_INFO:
		pLevel = "I";
		break;
	case LOG_WARN:
		pLevel = "W";
		break;
	case LOG_ERROR:
		pLevel = "E";
		break;
	default:
		configASSERT(pdFALSE);
		pLevel = "U";
		break;
	}
	sprintf(s_LogText, "%d-%02d-%02d %02d:%02d:%02d %s ",
			ts.tm_year + 1900,
			ts.tm_mon + 1,
			ts.tm_mday,
			ts.tm_hour,
			ts.tm_min,
			ts.tm_sec,
			pLevel);

	snprintf(s_LogText + 22, LOG_RECORD_SIZE, "%s\r\n", pLog->LogParam.LogText.cLog);
	s_LogText[LOG_RECORD_SIZE + 24] = 0;

	// 写入日志文件

	// 发送到shell
	OutputLogs(s_LogText);
}

void ReadLogs(uint16_t year, uint8_t month, uint8_t date)
{
	// 如果正在输出，则需要先停止

	// 等待停止成功

	// 记录日志

	// 触发输出
}


void LogTask( void * pvParameters)
{
	( void )pvParameters;

	LogMsg*			log;
	BaseType_t		ret;
	TickType_t		waitMS;

	while (1)
	{
		waitMS = 100;		// TODO:根据是否有请求显示日志文件的事务正在处理。如果有显示任务，则等待为0

		// 循环从队列中获取缓存，写入到文件中
		ret = xQueueReceive(log_queue, &log, pdMS_TO_TICKS(waitMS));

		if (ret == pdPASS)
		{
			switch (log->LogType)
			{
			case LOG_LOG_TEXT:
				WriteLogs(log);
				break;

			case LOG_WATERLEVEL:
				break;

			case LOG_TEMPERATURE:
				break;

			default:
				configASSERT(0);
				break;
			}

			// 释放消息
			FreeLogMsg(log);
		}

		// 有请求时，从文件中读取，然后返回

	}
}



static BaseType_t cmd_log( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *	pcParameter;
	BaseType_t 		xParameterStringLength;

	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	configASSERT( pcParameter );
	if (strncasecmp(pcParameter, "out", xParameterStringLength) == 0)
	{
		// 允许shell输出
		s_logout = 1;
		snprintf(pcWriteBuffer, xWriteBufferLen, "Log output on. \r\n");
	}
	else if (strncasecmp(pcParameter, "off", xParameterStringLength) == 0)
	{
		// 关闭shell输出
		s_logout = 0;
		snprintf(pcWriteBuffer, xWriteBufferLen, "Log output off. \r\n");
	}
	else
	{
		// 按日期格式分析

	}

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_log =
{
	"log",
	"\r\nlog out|off|<YYYYMMDD> \r\n Log output control.\r\n",
	cmd_log, /* The function to run. */
	1
};
