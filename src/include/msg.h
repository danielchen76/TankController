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
	MSG_MAIN_WATER_LEVEL,
	MSG_SUB_WATER_LEVEL,
	MSG_RO_WATER_LEVEL,
	MSG_SUB_WATER_LEVEL_REF,	// 底缸稳定水位参考值，记录到EEPROM，掉电重启，或者暂停重启后，使用这个作为补水（RO）参考

	// 开关控制
	MSG_RO_WATER_PUMP,			// RO补水泵
	MSG_MAIN_PUMP,				// 主循环水泵
	MSG_RO_BACKUP_PUMP,			// 向RO水缸补水泵
	MSG_SEA_INOUT_PUMP,			// 换水/加注抽水泵
	MSG_STOP_ALL_PUMP,			// 停止所有的水泵

	MSG_HEATER,					// 加热棒（在消息中控制一根还是两个工作）
	MSG_CHILLER,				// 冷水机电源
	MSG_HEATER_MODE,			// 加热棒工作模式（自动还是手动）
	MSG_CHILLER_MODE,			// 冷水机工作模式（自动还是手动）

	// 电源状态变更消息
	MSG_ADC_FINISHED,			// ADC采样完成
	MSG_AC_POWER,				// 交流电源状态
	MSG_BACKUP_POWER,			// 备用电源状态

	// 组合控制消息
	MSG_PAUSE_SYS,				// 系统暂停（主要控制所有泵停机一段时间，加热棒和冷水机则不需要暂停）
	MSG_START_SYS,
	MSG_STOP_SYS,				// 根据需要停止系统（组合，停止水泵，也停止加热棒。冷水机可以考虑是否不停止。默认冷水机不停）

	// 其他控制消息

	// LED、Button、数码管、蜂鸣器
	MSG_BUTTON_DOWN,			// 按键按下
	MSG_BUTTON_UP,				// 按键松开
	MSG_BUTTON_FWD,				// 旋钮向前转
	MSG_BUTTON_REV,				// 旋钮向后转
	MSG_BUTTON_CLICK,
	MSG_BUTTON_LONG_CLICK,

	MSG_EX_RO_WATER,			// 有RO备用水
	MSG_EX_RO_WATER_EMPTY,		// RO备用水空

	MSG_ALARM_ERROR,			// 告警音（错误，连续）
	MSG_ALARM_WARN,				// 告警音（提示，间歇，短暂提示）


} enumMsg;

// 供电状态
typedef enum
{
	POWER_AC,					// 220v供电
	POWER_BATTERY,				// 后备电池供电
	POWER_TOBACKUP,				// 切换到备用电池的中间状态
	POWER_AC_RESUME,			// 切换会220v供电中间态（没有什么动作，只是检测到220v恢复后，等待一段时间才恢复）
} enumPowerMode;

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

	struct Msg_DeviceEnable
	{
		BaseType_t	bAuto;			// 是否允许自动控制（这个和Switch有差别）(TRUE:auto, FALSE:manual)
	}	Mode;

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
	struct Msg_DisplayBackLightOnOff
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
Msg* MallocMsgFromISR();
void FreeMsg(Msg* pMsg);
void FreeMsgFromISR(Msg* pMsg, portBASE_TYPE* pWoken);

#define MSG_SEND(p)					xQueueSendToBack(main_queue, (void*)&p, portMAX_DELAY)
#define MSG_SEND_I(p, woken)		xQueueSendToBackFromISR(main_queue, (void*)&p, woken)

#define LED_MSG_SEND(p)				xQueueSendToBack(LEDButton_queue, (void*)&p, portMAX_DELAY)
#define LED_MSG_SEND_I(p, woken)	xQueueSendToBackFromISR(LEDButton_queue, (void*)&p, woken)

#define WL_MSG_SEND(p)				xQueueSendToBack(waterlevel_queue, (void*)&p, portMAX_DELAY)
#define WL_MSG_SEND_I(p, woken)		xQueueSendToBackFromISR(waterlevel_queue, (void*)&p, woken)

#define TEMP_MSG_SEND(p)			xQueueSendToBack(temp_queue, (void*)&p, portMAX_DELAY)
#define TEMP_MSG_SEND_I(p, woken)	xQueueSendToBackFromISR(temp_queue, (void*)&p, woken)

// 每个queue外部定义
extern QueueHandle_t		main_queue;
extern QueueHandle_t		waterlevel_queue;
extern QueueHandle_t		temp_queue;
extern QueueHandle_t		LEDButton_queue;

//extern QueueHandle_t		sensor_queue;



#endif /* MSG_MSG_H_ */
