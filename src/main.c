//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

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

#include "main/controller.h"
#include "main/TemperatureTask.h"
#include "msg.h"
#include "i2c/stm32_eval_i2c_ee.h"
#include "tc_rtc.h"
#include "logTask.h"
#include "tc_spi.h"

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
#define WATERLEVEL_TASK_STACK_SIZE	500
#define LED_TASK_STACK_SIZE			configMINIMAL_STACK_SIZE
#define LOG_TASK_STACK_SIZE			500

// 主线程，初始化使用，以及所有的主消息处理
static void MainTask( void * pvParameters)
{
	// RTC
	InitRTC();

	// SPI flash和文件系统
	InitSpiffs();

	// 初始化消息数组和所有消息队列
	InitMsgArray();
	InitMainMsgQueue();

	InitLEDTask();

	// 初始化LED指示灯线程
	xTaskCreate(LEDTask, "LEDTask", LED_TASK_STACK_SIZE, NULL, 1, NULL);

	// E2PROM，加载默认配置，和从E2PROM中读取保存的配置
	/* Initialize the I2C EEPROM driver ----------------------------------------*/
	sEE_Init();
	InitSetting();

	// 初始化Shell
	InitShell();
	EnableBluetooth(pdTRUE);		// TODO: 需要最后再确定是否默认开启蓝牙模块，还是通过手动方式开启。

	// 初始化日志
	InitLogTask();
	xTaskCreate(LogTask, "LogTask", LOG_TASK_STACK_SIZE, NULL, 1, NULL);

	// 初始化所有GPIO口
	InitPowerGPIO();
	InitSensorsGPIO();

	// 初始化温度控制任务
	InitTempMsgQueue();

	// 初始化水位控制任务
	InitWaterLevelMsgQueue();

	// 初始化GUI

	// 启动任务
	//xTaskCreate(TempControlTask, "TempTask", TEMP_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(WaterLevelControlTask, "WaterLevelTask", WATERLEVEL_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

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

