/*
 * WaterLevelTask.c
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */


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
#include <stdlib.h>

#include <task.h>
#include <WaterLevelTask.h>
#include <LogTask.h>

#include "stm32f10x.h"

// 定义队列大小（水位控制用）
QueueHandle_t		waterlevel_queue;
const UBaseType_t 	uxWaterLevelQueueSize = 10;			// 考虑到会手工控制抽水和补水，所以将消息队列加大

// 函数定义
void ProcessWaterLevel(void* pvParameters);

static struStateMachine			s_WLStateMachine;

// 状态机函数定义（同时也是给状态值使用）
void* WL_StopState();
void* WL_Init1State();
void* WL_NormalState();
void* WL_RORefillState();
void* WL_ROBackupRefillState();

void RefillROWaterTimer(void* pvParameters);
void RefillBackupROWaterTimer(void* pvParameters);
void ProteinSkimmerOnTimer(void* pvParameters);


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

void MsgACPowerChange(Msg* msg)
{
	if (msg->Param.Power.bOk)
	{
		// AC电源恢复，延迟一段时间后进入启动状态

	}
	else
	{
		// AC电源停电（关闭所有的设备）
	}
}

void MsgBackupPowerChange(Msg* msg)
{
	if (msg->Param.Power.bOk)
	{
		// 备用电源启动正常，延迟一段时间后进入备用电源启动状态（部分设备保持关闭或进入低功耗模式）
	}
	else
	{
		// 备用电源关闭（关闭所有的设备）
	}
}

void MsgPauseSystem(Msg* msg)
{
	if (msg->Param.Pause.seconds > 0)
	{
		// 设定一个定时器，触发重启系统
	}

	// 控制所有泵进入停机状态
	// TODO:造浪泵怎么控制？（造浪泵也在这个水位控制任务中控制了？）
}

static struEntry	Entries[] =
{
	{MSG_RO_WATER_PUMP,		MsgRoPumpSwitch},
	{MSG_RO_BACKUP_PUMP,	MsgRoBackupPumpSwitch},
	{MSG_AC_POWER,			MsgACPowerChange},
	{MSG_BACKUP_POWER,		MsgBackupPowerChange},
	{MSG_PAUSE_SYS,			MsgPauseSystem}
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

	// 测试用值，用命令行模拟水位数据来验证（如果测试数据为0，则表示获取实际数据）
	uint16_t FakeData;
} struWaterLevelData;

typedef struct
{
	uint16_t	usHeight;				// 高度是由超声波探头到缸底的高度（并不一定是实际的高度，和超声波探头安装位置有关）
	uint16_t	usLength;
	uint16_t	usWidth;
} struTankSize;


#define SUB_TANK			SUB_US_PORT
#define RO_TANK				RO_US_PORT
#define MAIN_TANK			MAIN_US_PORT

#define ULTRASOUND_NUM		3

static struWaterLevelData			s_WaterLevel[ULTRASOUND_NUM];			// 主缸/底缸/RO缸
static BaseType_t					s_HasBackupRO = pdFALSE;				// 备用RO水箱是否有水

// 缸的尺寸（静态，不可变）
const struTankSize					c_TankSize[ULTRASOUND_NUM] =
{
		{RO_TANK_HEIGHT, RO_TANK_LENGTH, RO_TANK_WIDTH},
		{SUB_TANK_HEIGHT, SUB_TANK_LENGTH, SUB_TANK_WIDTH},
		{MAIN_TANK_HEIGHT, MAIN_TANK_LENGTH, MAIN_TANK_WIDTH},
};

static TickType_t			s_NowTick;

static const TickType_t	c_DataInterval		= pdMS_TO_TICKS(1000);		// 水位数据每1秒采集一次（后续再看看是否提升到500ms）
static const uint8_t		c_NSeconds			= 7;						// N秒平均值是多少秒 （需要考虑最大值和最小值会被删除）

static struMyTimer			s_WLTimerArray[5];
static struMyTimerQueue		s_WLTimerQueue = {s_WLTimerArray, sizeof(s_WLTimerArray) / sizeof(s_WLTimerArray[0])};

// 蛋分延迟启动定时器
static int16_t				s_ProteinSkimmerTimer = -1;

// 电源切换延迟启动定时器
static int16_t				s_PowerChangeTimer	= -1;

// 备用RO水补水定时检查（避免陷入备用RO水补水状态，而无法返回正常的补水检测）
static int16_t				s_BackupRORefillTimer = -1;
#define BACKUPROREFILL_TIMER_TIMEOUT			(30 * 1000)					// 至少每隔30秒提升10mm水位。如果低于这个标准，就退回正常状态
#define BACKUPROREFILL_MIN_HEIGHT				10
static uint16_t				s_usLastROWL = 0;								// 上次定时检查的水位

// 临时补水量（临时目标水位）

void InitWaterLevelControlTask()
{
	memset(s_WaterLevel, 0, sizeof(s_WaterLevel));

	// 初始化Timer队列
	InitTimerQueue(&s_WLTimerQueue);

	// 添加水位定时检测回调函数
	AddTimer(&s_WLTimerQueue, xTaskGetTickCount(), c_DataInterval, pdTRUE, ProcessWaterLevel, NULL);
	LogOutput(LOG_INFO, "1 second timer, for check water level and run state machine.");
}

//
void WaterLevelControlTask( void * pvParameters)
{
	(void)pvParameters;

	// 初始化水位控制任务的所有数据
	InitWaterLevelControlTask();

	// 从停止状态切换到Init1，重启后，默认直接进入正常状态
	StateMachineSwitch(&s_WLStateMachine, WL_Init1State);

	while (1)
	{
		// 处理消息队列（暂定一次一条）
		MsgProcess_entry();

		s_NowTick = xTaskGetTickCount();

		// 定时器队列检查
		CheckTimerQueue(&s_WLTimerQueue, s_NowTick);
	}
}

// 计算平均值（去除最大值、最小值，计算平均值）
uint8_t GetAverage(struWaterLevelData* pData, int16_t pos, uint16_t* pAvg)
{
	uint8_t		count = 0;			// 有多少个有效值
	uint32_t	total = 0;
	uint8_t		i;
	uint16_t	usMax, usMin;		// 记录最大最小值

	usMax = 0;
	usMin = 0Xffff;

	// 从pos开始循环，判断最大值和最小值，并进行累加
	for (i = 0; i < c_NSeconds; i++)
	{
		if (--pos < 0)
		{
			pos = WL_BUFFER_SIZE - 1;
		}

		if (pData->WaterLevelBuffers[pos] > 0)
		{
			uint16_t	cur = pData->WaterLevelBuffers[pos];
			total += cur;

			// 有效数值+1
			count++;

			// 判断是否是最大/最小值
			if (cur > usMax)
			{
				usMax = cur;
			}

			if (cur < usMin)
			{
				usMin = cur;
			}
		}
	}

	// 至少5个有效值（包含最大最小值）才进行计算，否则就认为计算失败，仍保持上次的平均值
	if (count > 5)
	{
		// 删除最大值
		total -= usMax;

		// 删除最小值
		total -= usMin;

		// 计算平均值
		*pAvg = (uint16_t)(total / (count - 2));
	}
	else
	{
		count = 0;
	}

	return count;
}


// 检测、计算水位数据，然后根据水位数据和当前状态进行处理
void ProcessWaterLevel(void* pvParameters)
{
	uint8_t			i;
	int16_t			pos;
	uint16_t		wlAvg;
	uint8_t			count;
	BaseType_t		bRet;
	uint16_t		usDistance;
	struWaterLevelData*		pData;
	Msg*			pMsg;

	// TODO:后续需要修改这个写死的常量
	for (i = 0; i < 2; i++)
	{
		pData = &s_WaterLevel[i];

		// 检测水位
		if (pData->FakeData == 0)
		{
			// 获取实际水位数据
			bRet = GetDistance(i, &usDistance);
			if (bRet != pdTRUE)
			{
				// 水位检测失败（告警）
				LogOutput(LOG_ERROR, "Port %d water level failed.", i);
				usDistance = 0;
			}
			else
			{
				// 换算为实际水位高度
				LogOutput(LOG_INFO, "Port %d water level = %d.", i, usDistance);
				usDistance = c_TankSize[i].usHeight - usDistance;
			}
		}
		else
		{
			// 使用命令行模拟的数据
			usDistance = pData->FakeData;
		}

		pMsg = NULL;
		// 无论是否检测到水位，本次数据都会更新。没有检测到的会变成0，并重新计算平均值。有变更或异常，还是会通知和告警的

		// 和上次的数据比较，用于判断是否有变化，如果有变化需要发送通知到主消息队列
		if ((pData->WaterLevel != usDistance) && (!pMsg))
		{
			pMsg = MallocMsg();
		}

		pData->WaterLevelBuffers[pData->BufferPos++] = usDistance;
		pData->WaterLevel = usDistance;
		if (pData->BufferPos == WL_BUFFER_SIZE)
		{
			// 缓冲区指针溢出，回到0
			pData->BufferPos = 0;
		}

		// 计算平均值
		wlAvg = 0;
		pos = (int16_t)pData->BufferPos;
		count = GetAverage(pData, pos, &wlAvg);
		if (count > 0)
		{
			// 比较平均值和前一次计算是否有变化
			if (!pMsg && (wlAvg != pData->WaterLevel_N))
			{
				pMsg = MallocMsg();
			}
			pData->WaterLevel_N = wlAvg;
		}
		else
		{
			// 告警，长时间没有检测到水位数据（根据当前检测的探头通道，需要立即停止淡水补水，或备用淡水补水）
			// TODO:怎么解决开机后的初始化值都是0？（理论上，第一次如果成功获取到水位，则至少有一个数据，所以不会告警）
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

	// 检查备用RO水箱的水位状态（红外线检测，开关量）
	s_HasBackupRO = BackupROHasWater();

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
void* WL_Init1State()
{
	return NULL;
}

// 正常状态
void* WL_NormalState()
{
	// 检查底缸水位，检查是否补水（水位低于“参考值 - offset”后，同时检查RO水缸水位是否大于最小值）
	if ((s_WaterLevel[SUB_TANK].WaterLevel_N < (Get_usSubTankWaterLevelRef() - Get_usRORefillOffset()))
			&& (s_WaterLevel[RO_TANK].WaterLevel_N > Get_usRoTankWaterLevel_Min()))
	{
		LogOutput(LOG_INFO, "Begin to refill RO water to sub tank.");
		return WL_RORefillState;
	}

	// 检查RO缸水位，检测是否需要从备用RO桶补RO水（备用RO桶还要判断是否有水，有水才能切换状态）
	if ((s_WaterLevel[RO_TANK].WaterLevel_N < Get_usRoTankWaterLevel_Refill())
			&& (s_HasBackupRO))
	{
		LogOutput(LOG_INFO, "Begin to refill RO tank.");
		return WL_ROBackupRefillState;
	}

	// 没有变化，保持当前状态
	return NULL;
}

// 补水状态
void* WL_RORefillState()
{
	// 检查底缸水位 和 RO水缸水位（底缸需要使用即时水位检查，避免延迟）
	if ((s_WaterLevel[SUB_TANK].WaterLevel >= Get_usSubTankWaterLevelRef())
			|| (s_WaterLevel[RO_TANK].WaterLevel <= Get_usRoTankWaterLevel_Min()))
	{
		LogOutput(LOG_INFO, "Sub tank:%dmm, RO tank:%dmm", s_WaterLevel[SUB_TANK].WaterLevel, s_WaterLevel[RO_TANK].WaterLevel);
		LogOutput(LOG_INFO, "Finish refill RO water to sub tank.");
		return WL_NormalState;
	}

	return NULL;
}

// RO备用补水状态
void* WL_ROBackupRefillState()
{
	// 检查备用RO缸水位状态（红外线探测，开关量） 和 RO水缸水位
	if ((s_WaterLevel[RO_TANK].WaterLevel >= Get_usRoTankWaterLevel_Max())
			|| (!s_HasBackupRO))
	{
		LogOutput(LOG_INFO, "RO tank:%dmm, backup RO:%s", s_WaterLevel[RO_TANK].WaterLevel, s_HasBackupRO ? "Yes" : "No");
		LogOutput(LOG_INFO, "Finish refill RO tank");
		return WL_ROBackupRefillState;
	}

	return NULL;
}

// 从停止状态切换到初始启动状态
void WL_Stop_Init1_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 开启主泵
	Switch_MainPump(POWER_ON);

	// 设置定时器，延迟启动蛋分（防止水位太高或者因停机太久，水质变差，导致蛋分暴冲）
	if (s_ProteinSkimmerTimer)
	{
		RemoveTimer(&s_WLTimerQueue, s_ProteinSkimmerTimer);
	}
	s_ProteinSkimmerTimer = AddTimer(&s_WLTimerQueue, xTaskGetTickCount(), Get_ulProteinSkimmerTimer(), pdFALSE, ProteinSkimmerOnTimer, NULL);

	LogOutput(LOG_INFO, "Start timer for delay turn on protein skimmer.");
}

// 从初始状态切换到正常状态需要执行的动作
void WL_Init1_Normal_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 这个状态通常有定时器启动切换的
	// 启动蛋分
	Switch_ProteinSkimmer(POWER_ON);

	LogOutput(LOG_INFO, "Init1 mode switch to normal mode.");
}

// 从正常切换到补充淡水
void WL_Normal_RoRefill_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 启动RO水泵
	Switch_RoPump(POWER_ON);

	LogOutput(LOG_INFO, "Normal mode switch to RO refill mode.");
}

// 从补水切换到正常
void WL_Refill_Normal_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 关闭RO水泵
	Switch_RoPump(POWER_OFF);

	LogOutput(LOG_INFO, "RO refill mode switch to normal mode.");
}

// 从正常切换到备用RO水补充
void WL_Normal_RoBackupRefill_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 启动备用RO水泵，向RO缸补充淡水
	Switch_BackupRoPump(POWER_ON);

	// 启动一个保护定时器，确保备用RO水补充状态不会因为意外错误导致状态机无法恢复到正常检测状态
	if (s_BackupRORefillTimer > 0)
	{
		RemoveTimer(&s_WLTimerQueue, s_BackupRORefillTimer);
	}
	s_BackupRORefillTimer = AddTimer(&s_WLTimerQueue, xTaskGetTickCount(), BACKUPROREFILL_TIMER_TIMEOUT, pdTRUE, RefillBackupROWaterTimer, NULL);

	LogOutput(LOG_INFO, "Normal mode switch to backup RO refill mode.");
}

// 从备用补水切换到正常
void WL_BackupRefill_Normal_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 关闭备用RO水泵
	Switch_BackupRoPump(POWER_OFF);

	LogOutput(LOG_INFO, "Backup RO refill mode switch to normal mode.");
}

// 从所有状态切换到停止
void WL_All_Stop_Change(StateMachineFunc pOld)
{
	(void)pOld;

	// 删除部分可能正在进行的定时器（例如：Init时的蛋分启动延迟，或者电源切换过程使用的延迟）
	if (s_ProteinSkimmerTimer)
	{
		RemoveTimer(&s_WLTimerQueue, s_ProteinSkimmerTimer);
		s_ProteinSkimmerTimer = -1;
	}

	// 从备用RO补充过程的定时器关闭
	if (s_BackupRORefillTimer > 0)
	{
		RemoveTimer(&s_WLTimerQueue, s_BackupRORefillTimer);
	}

	// 关闭蛋分
	Switch_ProteinSkimmer(POWER_OFF);

	// 关闭主泵
	Switch_MainPump(POWER_OFF);

	// 关闭所有补水水泵
	Switch_RoPump(POWER_OFF);
	Switch_BackupRoPump(POWER_OFF);
	Switch_SeaPumpOut(POWER_OFF);
	Switch_SeaPumpIn(POWER_OFF);

	LogOutput(LOG_INFO, "Stop mode.");
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
	// 比较当前RO缸平均水位和上次水位的比较，如果小于预定值，则将切换回正常状态
	if (s_usLastROWL == 0)
	{
		s_usLastROWL = s_WaterLevel[RO_TANK].WaterLevel_N;
		return;
	}

	if (s_WaterLevel[RO_TANK].WaterLevel_N < (s_usLastROWL + BACKUPROREFILL_MIN_HEIGHT))
	{
		// 水位增加太慢，有问题，切换回正常状态
		StateMachineSwitch(&s_WLStateMachine, WL_NormalState);
		s_usLastROWL = 0;
		LogOutput(LOG_ERROR, "Backup RO refill error, found too less water refill to RO tank.");
	}
	else
	{
		s_usLastROWL = s_WaterLevel[RO_TANK].WaterLevel_N;
	}
}

// 延迟启动蛋分
void ProteinSkimmerOnTimer(void* pvParameters)
{
	(void)pvParameters;

	StateMachineSwitch(&s_WLStateMachine, WL_NormalState);
}


// 水位控制任务的状态机配置数据
const struStateChangeEntry		c_WLStateMachineChangeEntries[] =
{
		{WL_StopState, WL_Init1State, WL_Stop_Init1_Change},
		{WL_Init1State, WL_NormalState, WL_Init1_Normal_Change},
};

static struStateMachine			s_WLStateMachine =
{
		c_WLStateMachineChangeEntries,
		sizeof(c_WLStateMachineChangeEntries) / sizeof(c_WLStateMachineChangeEntries[0]),
		WL_StopState
};

// -------------------------------Command line--------------------------------------------
static BaseType_t cmd_wlstatus( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	char			WriteBuffer[30];
	(void)pcCommandString;

	// 输出每个缸当前水位
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Sub Tank:\r\n");
	strncpy(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Current:%dmm\r\n", s_WaterLevel[SUB_TANK].WaterLevel);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Average:%dmm\r\n", s_WaterLevel[SUB_TANK].WaterLevel_N);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "!!!Fake:%dmm\r\n", s_WaterLevel[SUB_TANK].FakeData);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);


	snprintf(WriteBuffer, sizeof(WriteBuffer), "\r\nRO Tank:\r\n");
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Current:%dmm\r\n", s_WaterLevel[RO_TANK].WaterLevel);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Average:%dmm\r\n", s_WaterLevel[RO_TANK].WaterLevel_N);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "!!!Fake:%dmm\r\n", s_WaterLevel[RO_TANK].FakeData);
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);

	snprintf(WriteBuffer, sizeof(WriteBuffer), "\r\nBackup RO Tank:\r\n");
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);
	snprintf(WriteBuffer, sizeof(WriteBuffer), "Has water:%s\r\n", s_HasBackupRO ? "Yes" : "No");
	strncat(pcWriteBuffer, WriteBuffer, xWriteBufferLen);

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_wlstatus =
{
	"wlstatus",
	"\r\nwlstatus\r\n Show water level datas.\r\n",
	cmd_wlstatus, /* The function to run. */
	0
};

static BaseType_t cmd_wlset( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *	pcParameter;
	BaseType_t 		xParameterStringLength;
	uint16_t		usDistance;
	int8_t			tankIndex = -1;

	// 分析第二个参数，水位数据，单位mm
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
	configASSERT( pcParameter );
	usDistance = atoi(pcParameter);

	// 判断哪个超声波距离传感器
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	configASSERT( pcParameter );
	if (strncasecmp(pcParameter, "ro", xParameterStringLength) == 0)
	{
		tankIndex = RO_TANK;
	}
	else if (strncasecmp(pcParameter, "sub", xParameterStringLength) == 0)
	{
		tankIndex = SUB_TANK;
	}
	else if (strncasecmp(pcParameter, "main", xParameterStringLength) == 0)
	{
		tankIndex = MAIN_TANK;
	}

	if (tankIndex >= 0)
	{
		// 设置虚拟值
		s_WaterLevel[tankIndex].FakeData = usDistance;
		snprintf(pcWriteBuffer, xWriteBufferLen, "Tank (%d) set %dmm.\r\n", tankIndex, usDistance);
	}
	else
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "Wrong tank!\r\n");
	}

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_wlset =
{
	"wlset",
	"\r\nwlset <sub|ro|main> <water level(mm)>\r\n Set fake water level datas.\r\n",
	cmd_wlset, /* The function to run. */
	2
};





