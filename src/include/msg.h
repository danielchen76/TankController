/*
 * msg.h
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#ifndef MSG_MSG_H_
#define MSG_MSG_H_

#include <stdio.h>
#include "const.h"
#include <FreeRTOS.h>
#include <queue.h>

// 消息ID，主处理线程和GUI处理线程，以及其他处理线程，共用一个消息枚举，消息内存也一起合并定义
typedef enum
{
	MSG_UNUSED,					// 标记消息没有使用
	MSG_USED,					// 分配后，还没有赋值前使用

	// 水位和温度更新消息
	MSG_MAIN_TANK_TEMP,
	MSG_SUB_TANK_TEMP,
	MSG_MAIN_WATER_LEVEL,		// 这3个消息顺序依赖4052的Port顺序，所以不能随意调整
	MSG_SUB_WATER_LEVEL,
	MSG_RO_WATER_LEVEL,
	MSG_SUB_WATER_LEVEL_REF,	// 底缸稳定水位参考值，记录到EEPROM，掉电重启，或者暂停重启后，使用这个作为补水（RO）参考

	// 开关控制
	MSG_RO_WATER_PUMP,			// RO补水泵
	MSG_MAIN_PUMP,				// 主循环水泵
	MSG_RO_BACKUP_PUMP,			// 向RO水缸补水泵
	MSG_WATER_OUT_PUMP,			// 换水抽水泵
	MSG_WATER_IN_PUMP,			// 加注海水泵

	MSG_HEATER,					// 加热棒（在消息中控制一根还是两个工作）
	MSG_CHILLER,				// 冷水机电源

	// 电源状态变更消息
	MSG_AC_POWER,				// 交流电源状态
	MSG_BACKUP_POWER,			// 备用电源状态

	// 组合控制消息
	MSG_PAUSE_SYS,				// 系统暂停（主要控制所有泵停机一段时间，加热棒和冷水机则不需要暂停）

	// 其他控制消息

	// GUI



} enumMsg;

typedef union
{
	//Power voltage
	struct	Msg_Temperature
	{
		int16_t		temperature;	// Unit:℃ （单位：0.01℃）
		int16_t		tempAvg;		// 过去N秒的温度平均值
	}	TankTemperature;

	struct Msg_WaterLevel
	{
		uint16_t	level;			// Unit:mm
		uint16_t	levelAgv;		// 过去N秒的平均水位

	}	WaterLevel;

	struct	Msg_Switch
	{
		BaseType_t	bOn;			//True:开启
		uint8_t		param;			// 扩展参数（目前只有加热棒使用），0：控制所有，1：主加热棒，2：副加热棒
	} 	Switch;

	struct Msg_Power
	{
		BaseType_t	bOk;			// true：电源正常，false：电源异常
	} Power;

	struct Msg_Pause
	{
		uint16_t	seconds;		// 暂停秒数，如果为0，等同永久停机
	} Pause;

	struct Msg_RTCSecond
	{
		uint32_t	time;
	} RTCSecond;

	// 下面定义给UI使用的消息（UI和主处理模块也可能共用消息结构，共用部分在上面）
	// 实时时钟消息（每分钟更新一次）
	struct Msg_UIClock
	{
		uint16_t	Year;
		uint8_t		Month;
		uint8_t		Day;

		uint8_t		Hour;
		uint8_t		Minute;
	} UIClock;

	// 液晶屏控制消息
	struct Msg_DisplayOnOff
	{
		int16_t		IsOn : 1;
	} DisplayOnOff;

	struct Msg_DisplayContrast
	{
		uint8_t		Contrast;
	} DisplayContrast;

} Msg_Param;

typedef struct
{
	enumMsg		Id;
	Msg_Param	Param;
} Msg;

typedef void (*MsgEntry_t)(Msg* msg);

typedef struct
{
	enumMsg		MsgId;
	MsgEntry_t	Entry;
} struEntry;

void InitMsgArray();
Msg* MallocMsg();
void FreeMsg(Msg* pMsg);

#define MSG_SEND(p)		xQueueSendToBack(main_queue, (void*)&p, portMAX_DELAY)
#define MSG_SEND_I(p)	xQueueSendToBackFromISR(main_queue, (void*)&p, pdFALSE)

#define GUI_MSG_SEND(p)		xQueueSendToBack(gui_queue, (void*)&p, portMAX_DELAY)
#define GUI_MSG_SEND_I(p)	xQueueSendToBackFromISR(gui_queue, (void*)&p, pdFALSE)

// 每个queue外部定义
extern QueueHandle_t		gui_queue;
extern QueueHandle_t		main_queue;
extern QueueHandle_t		waterlevel_queue;
extern QueueHandle_t		temp_queue;

//extern QueueHandle_t		sensor_queue;



#endif /* MSG_MSG_H_ */
