/*
 * DS18B20.h
 *
 *  Created on: Apr 28, 2015
 *      Author: Daniel
 */

#ifndef TESTDS18B20_DS18B20_H_
#define TESTDS18B20_DS18B20_H_

#include <stdio.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"

typedef struct struDS18B20_GPIO
{
	uint16_t		pin;
	GPIO_TypeDef*	port;
	uint32_t		portclk;
} struDS18B20_GPIO_t;

#define DS18B20_MAIN						(0)				// 主缸
#define DS18B20_UNDER						(1)				// 底缸

#define DS18B20_H(id)						GPIO_SetBits(s_DS18B20s[id].port, s_DS18B20s[id].pin)
#define DS18B20_L(id)						GPIO_ResetBits(s_DS18B20s[id].port, s_DS18B20s[id].pin)

#define DS18B20_COMMAND_SEARCHROM			(0xF0)
#define DS18B20_COMMAND_READROM				(0x33)
#define DS18B20_COMMAND_MARCHROM			(0x55)
#define DS18B20_COMMAND_SKIPROM				(0xCC)
#define DS18B20_COMMAND_ALARMSEARCH			(0xEC)
#define DS18B20_COMMAND_CONVERTT			(0x44)
#define DS18B20_COMMAND_WRITESCRATCHPAD		(0x4E)
#define DS18B20_COMMAND_READSCRATCHPAD		(0xBE)
#define DS18B20_COMMAND_COPYSCRATCHPAD		(0x48)
#define DS18B20_COMMAND_RECALLE2			(0xB8)
#define DS18B20_COMMAND_READPOWERSUPPLY		(0xB4)


BaseType_t InitDS18B20();
int16_t GetTemperature(uint8_t id);


#endif /* TESTDS18B20_DS18B20_H_ */
