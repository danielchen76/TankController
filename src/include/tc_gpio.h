/*
 * tc_gpio.h
 *
 *  Created on: Dec 28, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_TC_GPIO_H_
#define INCLUDE_TC_GPIO_H_

#include <FreeRTOS.h>
#include "stm32f10x.h"

#define	 POWER_ON		pdTRUE
#define POWER_OFF		pdFALSE

void InitPowerGPIO();
void InitSensorsGPIO();

// 24v/12v低压设备开关
void Switch_MainPump(BaseType_t bOn);
void Switch_ProteinSkimmer(BaseType_t bOn);
void Switch_WaveMaker(BaseType_t bOn);
void Switch_WaveMakerNightMode(BaseType_t bOn);
void Switch_SubWaveMaker(BaseType_t bOn);
void Switch_RoPump(BaseType_t bOn);
void Switch_BackupRoPump(BaseType_t bOn);
void Switch_SeaPumpOut(BaseType_t bOn);
void Switch_SeaPumpIn(BaseType_t bOn);

// 220V设备
void Switch_Chiller(BaseType_t bOn);
void Switch_MainLight(BaseType_t bOn);
void Switch_SubLight(BaseType_t bOn);

void Switch_T5H0(BaseType_t bOn);
void Switch_Heater1(BaseType_t bOn);
void Switch_Heater2(BaseType_t bOn);

// 输入引脚状态
BaseType_t BackupROHasWater();


#endif /* INCLUDE_TC_GPIO_H_ */
