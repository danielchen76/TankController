/*
 * setting.c
 *
 *  Created on: Dec 1, 2015
 *      Author: daniel
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "setting.h"
#include "../i2c/tc_i2c.h"

// 注意：因为I2C DMA的问题，只能读取至少2个字节数据，所有所有的配置项必须大于等于2字节
typedef struct
{
	// 温度配置
	uint16_t		usLowTemperature;		// 最低温度，单位：0.01℃
	uint16_t		usHighTemperature;		// 最高温：0.01℃
	uint16_t		usTemperatureOffset;	// 温度调整量：0.01℃

	// 水位高度配置（每次根据换水启动后自动计算获得，如果是意外停机或暂停而重启的，则从该配置项读取，也可以人工控制该配置项）
	// 配置是指实际超声波探头测量的水位（如果要显示，需要进行换算），单位全部都是“毫米”
	uint16_t		usSubTankWaterLevel;

	uint16_t		usMainTankHeigh;
	uint16_t		usSubTankHeigh;
	uint16_t		usROTankHeigh;
} TC_Setting;

static TC_Setting		s_Setting;

static TC_Setting*		s_TempSetting = NULL;

typedef struct
{
	uint8_t		blockA_ok 			: 1;		// block A is ok
	uint8_t		blockA_needRewrite	: 1;		// block A need rewrite(update)

	uint8_t		blockB_ok			: 1;
	uint8_t		blockB_needRewrite	: 1;

	uint8_t		setting_loaded		: 1;		// 内存配置是否已经成功加载
} TC_BlockState;

static TC_BlockState	s_BlockState;

#define			SETTING_BLOCK_SIZE		sizeof(TC_Setting)

#define			FLAG_MAGIC				0x5555
#define			FLAG_WRITTING			0xAAAA

#define			BLOCK_A_OFFSET			0				// 0
#define			BLOCK_B_OFFSET			0x400			// 1K

#define			SETTING_OFFSET			4				// Setting在E2PROM的起始位置（4字节对齐，前两个字节是Magic，34字节是长度）
#define			SETTING_LENGTH_OFFSET	2

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

// 给内存的配置数据进行初始化默认值
void DefaultSettings()
{
	s_Setting.usHighTemperature		= 2800;				// 28.00℃ 最高温度
	s_Setting.usLowTemperature		= 2300;				// 23.00℃ 最低温度
	s_Setting.usTemperatureOffset	= 100;				// 1.00℃
}

// 装载E2PROM中的数据到内存中
int32_t LoadSettings(uint16_t block_offset)
{
	uint16_t		length = 0;
	BaseType_t		res;

	res = EEPROM_Read(block_offset + SETTING_LENGTH_OFFSET, (uint8_t*)&length, 2);

	if (!res)
	{
		return -1;
	}

	// 拷贝短的部分
	length = MIN(length, SETTING_BLOCK_SIZE);

	if (s_BlockState.setting_loaded == 1)
	{
		// 配置已经加载成功，不需要再加载（通常在Block B部分）
		s_TempSetting = (TC_Setting*)pvPortMalloc(sizeof(TC_Setting));

		// 但需要加载到TempSetting中，用于后续特殊异常的判断
		EEPROM_Read(block_offset + SETTING_OFFSET, (uint8_t*)s_TempSetting, length);
		return length;
	}
	res = EEPROM_Read(block_offset + SETTING_OFFSET, (uint8_t*)&s_Setting, length);

	if (!res)
	{
		return -1;
	}

	return length;
}


void SaveSettingToBlock(uint16_t block, uint16_t offset, uint16_t length, BaseType_t bAll)
{
	uint16_t		data;

	// 开始写入Block
	data = FLAG_WRITTING;
	EEPROM_Write(block, (uint8_t*)&data, 2);

	// 如果有需要写入长度
	if (bAll)
	{
		data = SETTING_BLOCK_SIZE;
		EEPROM_Write(block + SETTING_LENGTH_OFFSET, (uint8_t*)&data, 2);
	}

	// 数据写入Block
	EEPROM_Write(block + SETTING_OFFSET + offset, (uint8_t*)(&s_Setting) + offset, length);

	// 结束写入Block
	data = FLAG_MAGIC;
	EEPROM_Write(block, (uint8_t*)&data, 2);
}

BaseType_t SaveSetting(uint16_t offset, uint16_t length, BaseType_t bAll)
{
	// 保存配置到Block A
	SaveSettingToBlock(BLOCK_A_OFFSET, offset, length, bAll);

	// 保存配置到Block B
	SaveSettingToBlock(BLOCK_B_OFFSET, offset, length, bAll);

	return pdTRUE;
}

BaseType_t InitSetting()
{
	//uint16_t	length = 0;
	uint16_t	magic = 0;
	int32_t		length = -1;

	BaseType_t	res;

	// 初始化Block状态
	s_BlockState.blockA_ok 			= 0;
	s_BlockState.blockA_needRewrite = 1;
	s_BlockState.blockB_ok 			= 0;
	s_BlockState.blockB_needRewrite = 1;
	s_BlockState.setting_loaded 	= 0;

	// 初始化默认值
	DefaultSettings();

	// 判断block A的Magic
	res = EEPROM_Read(BLOCK_A_OFFSET, (uint8_t*)&magic, 2);
	if (res && (magic == FLAG_MAGIC))
	{
		// Block A的配置状态正确
		s_BlockState.blockA_ok = 1;

		length = LoadSettings(BLOCK_A_OFFSET);

		if (length > 0)
		{
			if (length != SETTING_BLOCK_SIZE)
			{
				// 内存的配置大小和E2PROM的不一致，需要重写E2PROM（无论是升级还是降级）
				s_BlockState.blockA_needRewrite = 1;
			}
			else
			{
				s_BlockState.blockA_needRewrite = 0;
			}

			// 配置从Block A加载成功
			s_BlockState.setting_loaded = 1;
		}
		else
		{
			// TODO:配置加载失败
		}
	}

	// 判断block B的Magic
	res = EEPROM_Read(BLOCK_B_OFFSET, (uint8_t*)&magic, 2);
	if (res && (magic == FLAG_MAGIC))
	{
		// Block B的配置状态正确
		s_BlockState.blockB_ok = 1;

		length = LoadSettings(BLOCK_B_OFFSET);

		if (length > 0)
		{
			if (length != SETTING_BLOCK_SIZE)
			{
				// 内存的配置大小和E2PROM的不一致，需要重写E2PROM（无论是升级还是降级）
				s_BlockState.blockB_needRewrite = 1;
			}
			else
			{
				s_BlockState.blockB_needRewrite = 0;
			}

			// 配置从Block B加载成功（也有可能已经从A加载成功，这里只是跳过）
			s_BlockState.setting_loaded = 1;
		}
		else
		{
			// TODO:配置加载失败
		}
	}

	// 对Block进行后续处理
	if ( (s_BlockState.blockA_ok == 1) && (s_BlockState.blockB_ok == 1) )
	{
		// 特殊异常（当Block A正确写入，但是B还没有改变Magic时，此时掉电，会出现B正常，但是实际B的配置和A不同）
		if (s_BlockState.blockB_needRewrite == 0)
		{
			// 和内存的配置进行逐字节比较
			if (memcmp(&s_Setting, s_TempSetting, sizeof(s_Setting)) != 0)
			{
				// Block B数据和内存的不同，需要重写
				s_BlockState.blockB_needRewrite = 1;
			}
		}
	}

	// Free Block B setting temp copy
	if (s_TempSetting)
	{
		vPortFree(s_TempSetting);
	}

	// 对需要重写的Block进行重写（Block B先重写，然后才是A）
	if (s_BlockState.blockB_needRewrite == 1)
	{
		SaveSettingToBlock(BLOCK_B_OFFSET, 0, SETTING_BLOCK_SIZE, pdTRUE);
	}

	if (s_BlockState.blockA_needRewrite == 1)
	{
		SaveSettingToBlock(BLOCK_A_OFFSET, 0, SETTING_BLOCK_SIZE, pdTRUE);
	}

	return pdTRUE;
}

BaseType_t Set_LowTemperature(uint16_t usTemp)
{
	s_Setting.usLowTemperature = usTemp;

	// 保存配置到E2PROM
	return SaveSetting(offsetof(TC_Setting, usLowTemperature), sizeof(s_Setting.usLowTemperature), pdFALSE);
}

uint16_t Get_LowTemperature()
{
	return s_Setting.usLowTemperature;
}


// 读取E2PROM中指定地址的数据（至少读取2个字节）
static BaseType_t cmd_eeRead( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *	pcParameter;
	BaseType_t 		xParameterStringLength;
	int32_t			offset;
	int32_t			length, sublength;
	uint8_t			Buffer[8];		// 一行8个字节
	BaseType_t		res;

	// offset
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	configASSERT( pcParameter );
	offset = atoi(pcParameter);

	// length
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
	configASSERT( pcParameter );
	length = atoi(pcParameter);

	while (length > 0)
	{
		sublength = (length >= 8) ? 8 : length;
		length -= sublength;

		if (sublength < 2)
		{
			sublength = 2;
		}

		res = EEPROM_Read(offset, Buffer, sublength);

		if (res == pdTRUE)
		{
			// 输出数据
			snprintf(pcWriteBuffer, xWriteBufferLen, "%02X %02X %02X %02X %02X %02X %02X %02X \r\n",
					Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7]);
		}
		else
		{
			// 读取EEPROM失败
			snprintf(pcWriteBuffer, xWriteBufferLen, "Read failed!\r\n");
		}
	}



	return pdFALSE;
}


const CLI_Command_Definition_t cmd_def_eeRead =
{
	"eeRead",
	"\r\neeRead <offset> <length>\r\n Read E2PROM data. Length at least 2 bytes\r\n",
	cmd_eeRead, /* The function to run. */
	2
};
