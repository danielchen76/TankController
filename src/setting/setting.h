/*
 * setting.h
 *
 *  Created on: Dec 1, 2015
 *      Author: daniel
 */

#ifndef SETTING_SETTING_H_
#define SETTING_SETTING_H_

#include <FreeRTOS.h>
#include "..\uart\FreeRTOS_CLI.h"


BaseType_t InitSetting();


BaseType_t Set_LowTemperature(uint16_t usTemp);
uint16_t Get_LowTemperature();

extern const CLI_Command_Definition_t cmd_def_eeRead;

#endif /* SETTING_SETTING_H_ */
