/*
 * WaterLevelTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */


#include "WaterLevelTask.h"
#include "Msg.h"
#include "util.h"
#include "tc_serial.h"
#include "tc_gpio.h"
#include "logTask.h"
#include "setting.h"
#include "TimerQueue.h"
#include "StateMachine.h"

#include <queue.h>
#include <string.h>

#include <task.h>

// 定义队列大小（水位控制用）
QueueHandle_t		waterlevel_queue;
const UBaseType_t 	uxWaterLevelQueueSize = 10;			// 考虑到会手工控制抽水和补水，所以将消息队列加大

// 函数定义
void ProcessWaterLevel(void* pvParameters);

static struStateMachine			s_WLStateMachine;


void InitWaterLevelMsgQueue( void )
{
	waterlevel_queue = xQueueCreate(uxWaterLevelQueueSize, sizeof(Msg*));
}

void MsgRoPumpSwitch(Msg* msg)
{
	Switch_RoPump(msg->Param.Switch.bOn);
}

void MsgRoBackupPumpSwitch(Msg* msg)
{
	Switch_BackupRoPump(msg->Param.Switch.bOn);
}

static struEntry	Entries[] =
{
	{MSG_RO_WATER_PUMP,		MsgRoPumpSwitch},
	{MSG_RO_BACKUP_PUMP,	MsgRoBackupPumpSwitch},
};

void MsgProcess_entry(void)
{
	Msg* msg;
	unsigned int i;
	BaseType_t ret;

	ret = xQueueReceive(waterlevel_queue, &msg, pdMS_TO_TICKS(100));

	if (ret == pdPASS)
	{
		// 对消息进行处理
		for (i = 0; i < (sizeof(Entries) / sizeof(Entries[0])); i++)
		{
			if (Entries[i].MsgId == ((Msg*) msg)->Id)
			{
				Entries[i].Entry((Msg*) msg);
				break;
			}
		}

		// 完成处理后，将Msg设置为空闲
		FreeMsg(msg);
	}
}

// 当前水位
#define WL_BUFFER_SIZE			10
typedef struct
{
	uint16_t WaterLevelBuffers[WL_BUFFER_SIZE];		// 每500ms采样一个数据，5秒数据至少5个（暂定1秒一个数据）
	uint16_t BufferPos;					// 当前Buffer位置

	uint16_t WaterLevel;				// 当前瞬间值
	uint16_t WaterLevel_N;				// Ns采样的平均值
} struWaterLevelData;

typedef struct
{
	uint16_t	usHeight;				// 高度是由超声波探头到缸底的高度（并不一定是实际的高度，和超声波探头安装位置有关）
	uint16_t	usLength;
	uint16_t	usWidth;
} struTankSize;

#define MAIN_TANK			0
#define SUB_TANK			1
#define RO_TANK				2

#define ULTRASOUND_NUM		3

static struWaterLevelData			s_WaterLevel[ULTRASOUND_NUM];			// 主缸/底缸/RO缸

// 缸的尺寸（静态，不可变）
const struTankSize					c_TankSize[ULTRASOUND_NUM] =
{
		{MAIN_TANK_HEIGHT, MAIN_TANK_LENGTH, MAIN_TANK_WIDTH},
		{SUB_TANK_HEIGHT, SUB_TANK_LENGTH, SUB_TANK_WIDTH},
		{RO_TANK_HEIGHT, RO_TANK_LENGTH, RO_TANK_WIDTH}
};

static TickType_t			s_NowTick;

static const TickType_t	c_DataInterval		= pdMS_TO_TICKS(1000);		// 水位数据每1秒采集一次（后续再看看是否提升到500ms）
static const uint8_t		c_NSeconds			= 5;						// N秒平均值是多少秒

static struMyTimer			s_WLTimerArray[5];
static struMyTimerQueue		s_WLTimerQueue = {s_WLTimerArray, sizeof(s_WLTimerArray) / sizeof(s_WLTimerArray[0])};

// 临时补水量（临时目标水位）

void InitWaterLevelControlTask()
{
	memset(s_WaterLevel, 0, sizeof(s_WaterLevel));

	// 初始化Timer队列
	InitTimerQueue(&s_WLTimerQueue);

	// 添加水位定时检测回调函数
	AddTimer(&s_WLTimerQueue, xTaskGetTickCount(), c_DataInterval, pdTRUE, ProcessWaterLevel, NULL);
}

//
void WaterLevelControlTask( void * pvParameters)
{
	(void)pvParameters;

	// 初始化水位控制任务的所有数据
	InitWaterLevelControlTask();

	while (1)
	{
		// 处理消息队列（暂定一次一条）
		MsgProcess_entry();

		s_NowTick = xTaskGetTickCount();

		// 定时器队列检查
		CheckTimerQueue(&s_WLTimerQueue, s_NowTick);
	}
}

// 检测、计算水位数据，然后根据水位数据和当前状态进行处理
void ProcessWaterLevel(void* pvParameters)
{
	uint8_t			i;
	int16_t			pos;
	uint32_t		total;
	uint16_t		wlAvg;
	uint8_t			count;
	BaseType_t		bRet;
	uint16_t		usDistance;
	struWaterLevelData*		pData;
	Msg*			pMsg;
	BaseType_t		bChanged = pdFALSE;			// 任何一缸水位变化，都需要做一次处理

	for (i = 0; i < ULTRASOUND_NUM; i++)
	{
		pData = &s_WaterLevel[i];

		// 检测水位
		bRet = GetDistance(i, &usDistance);
		if (bRet != pdTRUE)
		{
			// 水位检测失败（告警）
			LogOutput(LOG_ERROR, "Port %d water level failed.", i);
			usDistance = 0;
		}

		pMsg = NULL;
		// 无论是否检测到水位，本次数据都会更新。没有检测到的会变成0，并重新计算平均值。有变更或异常，还是会通知和告警的

		// 和上次的数据比较，用于判断是否有变化，如果有变化需要发送通知到主消息队列
		if ((pData->WaterLevel != usDistance) && (!pMsg))
		{
			pMsg = MallocMsg();
			bChanged = pdTRUE;
		}

		pData->WaterLevelBuffers[pData->BufferPos++] = usDistance;
		pData->WaterLevel = usDistance;
		if (pData->BufferPos == WL_BUFFER_SIZE)
		{
			// 缓冲区指针溢出，回到0
			pData->BufferPos = 0;
		}

		// 计算水位平均值
		pos = (int16_t)pData->BufferPos;
		total = 0;
		count = 0;

		// 累加
		for (i = 0; i < c_NSeconds; i++)
		{
			if (--pos < 0)
			{
				pos = WL_BUFFER_SIZE - 1;
			}

			if (pData->WaterLevelBuffers[i] > 0)
			{
				total += pData->WaterLevelBuffers[i];

				// 有效数值+1
				count++;
			}
		}

		// 计算平均值
		wlAvg = 0;
		if (count > 0)
		{
			wlAvg = (uint16_t)(total / count);
			// 比较平均值和前一次计算是否有变化
			if (!pMsg && (wlAvg != pData->WaterLevel_N))
			{
				pMsg = MallocMsg();
				bChanged = pdTRUE;
			}
			pData->WaterLevel_N = wlAvg;
		}
		else
		{
			// 告警，长时间没有检测到水位数据（根据当前检测的探头通道，需要立即停止淡水补水，或备用淡水补水）
			LogOutput(LOG_ERROR, "Port %d water level lost connection too long.", i);
		}

		if (pMsg)
		{
			// 有数据变更，需要通知
			pMsg->Id = MSG_MAIN_WATER_LEVEL + i;
			pMsg->Param.WaterLevel.level = usDistance;
			pMsg->Param.WaterLevel.levelAgv = wlAvg;

			if (MSG_SEND(pMsg) != pdPASS)
			{
				// 消息队列异常
				FreeMsg(pMsg);
			}
		}
	}

	// 状态机执行
	StateMachineRun(&s_WLStateMachine);
}



// 正常运作（补充淡水）

// 停机（排出海水）

// 停机（加入海水）

// 启动过程（中间态）
// TODO: 如果启动稳定后（例如：10分钟后），底缸水位和EEPROM存储的上次底缸应有的水位误差太大（例如：大于2CM），那么需要更新EEPROM上的数据，并以最新的为基准。
// 需要告警，日志（需要记录上次的值，以便可以手动恢复），远程告警

// 中间态到稳定态（时间+水位稳定程度，例如：1分钟内，没有超过1mm的水位变化。如果溢流槽内有水位检测，则效果更好）
// 蛋分的工作，底缸造浪泵的工作，也会影响。（建议关闭造浪泵，等系统判断为稳定，才重新启动底缸造浪泵）


// 另一条补水线路：RO水缸缺水（配置值），就检测外部RO水缸是否有水，如果有水就启动补充RO水的流程。


// 停止状态
void* WL_StopState()
{
	return NULL;
}

// 主泵开启
void* WL_Init_1_State()
{
	return NULL;
}

// 正常状态
void* WL_NormalState()
{
	// 检查底缸水位，检查是否补水

	// 检查RO缸水位，检测是否需要从备用RO桶补RO水

	//

	return NULL;
}

// 补水状态
void* WL_RORefillState()
{
	return NULL;
}

// RO备用补水状态
void* WL_ROBackupRefillState()
{
	return NULL;
}

// 从初始状态切换到正常状态需要执行的动作
void WL_Init_Normal_Change(StateMachineFunc pOld)
{

}

// 从正常切换到补充淡水
void WL_Normal_RoRefill_Change(StateMachineFunc pOld)
{

}

// 从补水切换到正常
void WL_Refill_Normal_Change(StateMachineFunc pOld)
{

}

// 从正常切换到备用RO水补充
void WL_Normal_RoBackupRefill_Change(StateMachineFunc pOld)
{

}

// 从备用补水切换到正常
void WL_BackupRefill_Normal_Change(StateMachineFunc pOld)
{

}

// 从所有状态切换到停止
void WL_All_Stop_Change(StateMachineFunc pOld)
{
	// 关闭蛋分
	Switch_ProteinSkimmer(POWER_OFF);

	// 关闭主泵
	Switch_MainPump(POWER_OFF);

	// 关闭所有补水水泵
	Switch_RoPump(POWER_OFF);
	Switch_BackupRoPump(POWER_OFF);
	Switch_SeaPumpOut(POWER_OFF);
	Switch_SeaPumpIn(POWER_OFF);
}

// 补充RO水到底缸，有一个最长约束时间，避免加水时间太长？
void RefillROWaterTimer(void* pvParameters)
{
	// 补水时间太长了，强制恢复到正常状态，等待下次符合补水条件后再加水
	//StateMachineSwitch(&s_WLStateMachine, WL_NormalState);

	// 每隔一段时间，检查水位是否和上次比较有变化（例如：升高至少多少），如果没有达到预期，可能是水泵异常或者老化或阻塞，需要提醒注意
	// 完全无变化，则可能需要切换状态
}

// 备用RO水补水定时检查
void RefillBackupROWaterTimer(void* pvParameters)
{

}

// 延迟启动蛋分
void ProteinSkimmerOnTimer(void* pvParameters)
{

}


// 水位控制任务的状态机配置数据
const struStateChangeEntry		c_WLStateMachineChangeEntries[] =
{
		{WL_StopState, WL_NormalState, WL_Init_Normal_Change},
};

static struStateMachine			s_WLStateMachine =
{
		c_WLStateMachineChangeEntries,
		sizeof(c_WLStateMachineChangeEntries) / sizeof(c_WLStateMachineChangeEntries[0]),
		WL_StopState
};

