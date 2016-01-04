/*
 * logTask.h
 *
 *  Created on: Jan 2, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_LOGTASK_H_
#define INCLUDE_LOGTASK_H_

#include <FreeRTOS.h>
#include "FreeRTOS_CLI.h"

typedef enum
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
} enumLogLevel;

void InitLogTask();
void LogTask( void * pvParameters);

void LogOutput(enumLogLevel level, const char* pLog, ...);
void LogTemperature(int16_t sTemperature, int16_t sHumidity, int16_t sMainT, int16_t sSubT);
void LogWaterLevel(uint16_t usMainWL, uint16_t usSubWL, uint16_t usRoWL);


extern const CLI_Command_Definition_t cmd_def_log;

#endif /* INCLUDE_LOGTASK_H_ */
