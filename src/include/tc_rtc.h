/*
 * tc_rtc.h
 *
 *  Created on: Jan 3, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_TC_RTC_H_
#define INCLUDE_TC_RTC_H_

#include <FreeRTOS.h>
#include "FreeRTOS_CLI.h"
#include "stm32f10x.h"

void InitRTC(void);
void GetRTC(uint32_t* pYear, uint32_t* pMonth, uint32_t* pDate, uint32_t* pWday, uint32_t* pHour, uint32_t* pMinute, uint32_t* pSecond);

extern const CLI_Command_Definition_t cmd_def_time;

#endif /* INCLUDE_TC_RTC_H_ */
