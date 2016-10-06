/*
 * DisplayButtonTask.h
 *
 *  Created on: May 7, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_DISPLAYBUTTONTASK_H_
#define INCLUDE_DISPLAYBUTTONTASK_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>

void InitLEDButton( void );
void InitDisplayButtonTask( void );
void DisplayButtonTask( void * pvParameters);

void SetErrorLED(BaseType_t bError);

// For Debug use only
void SetDebugLED(BaseType_t bOn);

// 告警和错误标识
#define WARN_RO_LOW					0x00000001
#define WARN_TEMP_LOW				0x00000002
#define WARN_TEMP_HIGH				0x00000004
#define WARN_BATTERY_LOW			0x00000008
#define WARN_CHANGE_WATER			0x00000001		// 停机换水告警，防止忘记开启
#define WARN_FEED_MODE				0x00000002		// 喂食模式（暂停主泵和蛋分一段时间，时间可配置）

#define ERROR_RO_EMPTY				0x00000001
#define ERROR_RO_HIGH				0x00000002
#define ERROR_SEA_HIGH				0x00000004
#define ERROR_MAINPUMP_STOP			0x00000008
#define ERROR_PROTEINSKIMMER_STOP	0x00000010

// LED数码管显示状态
#define LED_MODE_TIME				0				// 优先级最低
#define LED_MODE_TEMP				1				// 和时间交替显示
#define LED_MODE_WARN_ERROR			2				//
#define LED_MODE_MENU				3				// 优先级最高，菜单操作时，一直显示菜单状态（除非退出、超时退出）

void SetWarn(uint32_t flag);
void ClearWarn(uint32_t flag);
void SetError(uint32_t flag);
void ClearError(uint32_t flag);


extern const CLI_Command_Definition_t cmd_def_clearErr;


#endif /* INCLUDE_DISPLAYBUTTONTASK_H_ */
