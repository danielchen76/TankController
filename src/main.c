//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <controller.h>
#include <LEDTask.h>
#include <setting.h>
#include <stdio.h>
#include <stdlib.h>
#include <tc_gpio.h>
#include <tc_i2c.h>
#include <tc_serial.h>
#include <WaterLevelTask.h>
#include "diag/Trace.h"

#include "FreeRTOS.h"
#include "task.h"

#include <TemperatureTask.h>

#include "msg.h"
#include "i2c/stm32_eval_i2c_ee.h"
#include "tc_rtc.h"
#include "logTask.h"
#include "tc_spi.h"
#include "tc_adc.h"
#include "ds18b20/DS18B20.h"
#include <DisplayButtonTask.h>

// ----------------------------------------------------------------------------
//
// Semihosting STM32F1 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

// 堆栈大小
#define MAIN_TASK_STACK_SIZE		1000
#define TEMP_TASK_STACK_SIZE		500
#define WATERLEVEL_TASK_STACK_SIZE	1000
#define GUI_TASK_STACK_SIZE			1000
#define LEDBUTTON_TASK_STACK_SIZE	1000
#define LOG_TASK_STACK_SIZE			500

// 主线程，初始化使用，以及所有的主消息处理
static void MainTask( void * pvParameters)
{
	// RTC
	InitRTC();

	// TODO:（暂时不用）SPI flash和文件系统
	//InitSpiffs();

	// 初始化消息数组和所有消息队列
	InitMsgArray();
	InitMainMsgQueue();

	// 初始化日志
	InitLogTask();
	xTaskCreate(LogTask, "Log", LOG_TASK_STACK_SIZE, NULL, 1, NULL);

	// 初始化数码管和旋转编码器相关的接口
	InitDisplayButtonTask();

	// 初始化LED按键灯线程
	xTaskCreate(DisplayButtonTask, "LED", LEDBUTTON_TASK_STACK_SIZE, NULL, 1, NULL);

	// E2PROM，加载默认配置，和从E2PROM中读取保存的配置
	/* Initialize the I2C EEPROM driver ----------------------------------------*/
	sEE_Init();
	InitSetting();

	// 初始化Shell
	InitShell();
	EnableBluetooth(pdTRUE);		// TODO: 需要最后再确定是否默认开启蓝牙模块，还是通过手动方式开启。

	// 初始化所有GPIO口
	InitPowerGPIO();
	InitSensorsGPIO();

	// 初始化水位用超声波探头
	InitUltraSoundSensors();

	// 初始化ADC（电压监控和切换在Main主任务中执行）
	InitADC();

	// TODO:TEST
	GetDCVoltage();

	// 初始化温度探头
	InitDS18B20();

	// 初始化温度控制任务
	InitTempMsgQueue();

	// 初始化水位控制任务
	InitWaterLevelMsgQueue();

	// 启动任务
	xTaskCreate(TempControlTask, "TempControl", TEMP_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(WaterLevelControlTask, "WaterLevel", WATERLEVEL_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

	// TODO: TEST gpio
	// 24v/12v低压设备开关
//	Switch_MainPump(pdTRUE);
//	Switch_ProteinSkimmer(pdTRUE);
//	Switch_WaveMaker(pdTRUE);
//	Switch_WaveMakerNightMode(pdTRUE);
//	Switch_SubWaveMaker(pdTRUE);
//	Switch_RoPump(pdTRUE);
//	Switch_BackupRoPump(pdTRUE);
//	Switch_SeaPumpOut(pdTRUE);
//	Switch_SeaPumpIn(pdTRUE);

	// 220V设备
	Switch_Chiller(pdTRUE);
	Switch_MainLight(pdTRUE);
	Switch_SubLight(pdTRUE);

	Switch_T5H0(pdTRUE);
	Switch_Heater1(pdTRUE);
	Switch_Heater2(pdTRUE);



	// 进入主任务的消息处理函数（不再返回）
	controller_entry();
}


int main(int argc, char* argv[])
{
	// At this stage the system clock should have already been configured
	// at high speed.

	// 初始化主线程
	xTaskCreate(MainTask, "MainTask", MAIN_TASK_STACK_SIZE, NULL, 1, NULL);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// start FreeRTOS
	vTaskStartScheduler();

	// Would not go here!

	return 0;
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	/* This function will get called if a task overflows its stack.   If the
	parameters are corrupt then inspect pxCurrentTCB to find which was the
	offending task. */

	( void ) xTask;
	( void ) pcTaskName;

	for( ;; );
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


