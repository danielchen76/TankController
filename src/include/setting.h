/*
 * setting.h
 *
 *  Created on: Dec 1, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_SETTING_H_
#define INCLUDE_SETTING_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>


BaseType_t InitSetting();

// 注意：因为I2C DMA的问题，只能读取至少2个字节数据，所有所有的配置项必须大于等于2字节
typedef struct
{
	// 温度配置
	uint16_t		usLowTemperature;		// 最低温度，单位：0.01℃
	uint16_t		usHighTemperature;		// 最高温：0.01℃
	uint16_t		usTemperatureOffset;	// 温度调整量：0.01℃

	// 水位高度配置（每次根据换水启动后自动计算获得，如果是意外停机或暂停而重启的，则从该配置项读取，也可以人工控制该配置项）
	// 配置是指实际超声波探头测量的水位（实际水位，是经过换算后的），单位全部都是“毫米”
	uint16_t		usSubTankWaterLevelRef;	// 自动计算，由WaterLevel task进行更新（shell也要做到那里来做强制指定）
	uint16_t		usRORefillOffset;		// 当水位低于参考值多少时才启动补水
	uint16_t		usRORefillEnable;		// 是否启动自动补水

	uint16_t		usChangeWater;			// （单位：ml）最大换水量（通过主缸/底缸的水位降低总和来计算，以停机后几分钟作为参考）

	// RO补水缸的配置
	uint16_t		usRoTankWaterLevel_Max;
	uint16_t		usRoTankWaterLevel_Min;			// RO补水缸最低水位，达到和低于这个数值，就不再启动补水（到底缸）的动作，并要进行告警
	uint16_t		usRoTankWaterLevel_Refill;		// RO补水缸自动启动再补水（外部备用RO水）的水位线（这个水位之上也可以通过手动方式启动再补水）
	uint16_t		usRoTankWaterLevel_Warn;		// RO补水缸提示再补水的水位

	// 延迟时间配置（单位：ms）
	uint32_t		ulProteinSkimmerTimer;			// 蛋分延迟启动时间
	uint32_t		ulBackupPowerOnlineTimer;		// AC电源掉电后，备用电源上线后，延迟多长时间启动到正常状态
	uint32_t		ulACPowerOnlineTimer;			// AC电源重新来电后，延迟多长才正式启用AC电源

} TC_Setting;

// 这个extern是为下面的宏准备的，不应该直接访问s_Setting这个全局变量
extern TC_Setting		s_Setting;

#define DEFINE_SETTING_SET_GET(setting)		\
	BaseType_t Set_##setting(typeof(s_Setting.setting) value); \
	typeof(s_Setting.setting) Get_##setting();


// 温度配置
DEFINE_SETTING_SET_GET(usLowTemperature)
DEFINE_SETTING_SET_GET(usHighTemperature)
DEFINE_SETTING_SET_GET(usTemperatureOffset)

// 水位高度配置（每次根据换水启动后自动计算获得，如果是意外停机或暂停而重启的，则从该配置项读取，也可以人工控制该配置项）
// 配置是指实际超声波探头测量的水位（实际水位，是经过换算后的），单位全部都是“毫米”
DEFINE_SETTING_SET_GET(usSubTankWaterLevelRef)
DEFINE_SETTING_SET_GET(usRORefillOffset)
DEFINE_SETTING_SET_GET(usRORefillEnable)

DEFINE_SETTING_SET_GET(usChangeWater)

// RO补水缸的配置
DEFINE_SETTING_SET_GET(usRoTankWaterLevel_Max)
DEFINE_SETTING_SET_GET(usRoTankWaterLevel_Min)
DEFINE_SETTING_SET_GET(usRoTankWaterLevel_Refill)
DEFINE_SETTING_SET_GET(usRoTankWaterLevel_Warn)

// 延迟时间配置（单位：ms）
DEFINE_SETTING_SET_GET(ulProteinSkimmerTimer)
DEFINE_SETTING_SET_GET(ulBackupPowerOnlineTimer)
DEFINE_SETTING_SET_GET(ulACPowerOnlineTimer)


extern const CLI_Command_Definition_t cmd_def_eeRead;

#endif /* INCLUDE_SETTING_H_ */
