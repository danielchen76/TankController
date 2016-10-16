/*
 * shell_serial.c
 *
 *	Bluetooth module on UART for shell
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */


#include <stdio.h>
#include <WaterLevelTask.h>
#include <TemperatureTask.h>
#include "tc_serial.h"
#include "setting.h"

#include "UARTAdapter.h"

// 各个模块的头文件，用来包含shell命令函数入口
#include "tc_rtc.h"
#include "logTask.h"
#include "LEDTask.h"
#include "controller.h"

#define mainUART_COMMAND_CONSOLE_STACK_SIZE		( 500 )
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY  	1


extern void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );

static BaseType_t prvQueryHeapCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	sprintf( pcWriteBuffer, "Current free heap %d bytes, minimum ever free heap %d bytes\r\n", ( int ) xPortGetFreeHeapSize(), ( int ) xPortGetMinimumEverFreeHeapSize() );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/* Structure that defines the "query_heap" command line command. */
static const CLI_Command_Definition_t xQueryHeap =
{
	"query-heap",
	"\r\nquery-heap:\r\n Displays the free heap space, and minimum ever free heap space.\r\n",
	prvQueryHeapCommand, /* The function to run. */
	0 /* The user can enter any number of commands. */
};

// reboot system
static BaseType_t cmd_reboot( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)pcCommandString;

	__disable_irq();

	NVIC_SystemReset();

	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_reboot =
{
	"reboot",
	"\r\nReboot system.\r\n",
	cmd_reboot, /* The function to run. */
	0
};

// Command entries
static const CLI_Command_Definition_t* commands[] =
{
	&xQueryHeap,
	&cmd_def_eeRead,
	&cmd_def_time,
	&cmd_def_log,
	&cmd_def_clearErr,
	&cmd_def_wlstatus,
	&cmd_def_wlset,
	&cmd_def_wlpump,
	&cmd_def_reboot,
	&cmd_def_gettemp,
	&cmd_def_sys,
	&cmd_def_backuppower,
	&cmd_def_uptime,
};

void vRegisterCLICommands( void )
{
	uint16_t 	i;

	// 注册命令
	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		FreeRTOS_CLIRegisterCommand(commands[i]);
	}
}

void InitShell( void )
{
	// REgister shell commands
	vRegisterCLICommands();

	// Start shell thread
	vUARTCommandConsoleStart(mainUART_COMMAND_CONSOLE_STACK_SIZE, mainUART_COMMAND_CONSOLE_TASK_PRIORITY);
}

void EnableBluetooth(BaseType_t bEnable)
{
	if (bEnable)
	{
		// 开启蓝牙（CarStation的串口1通过蓝牙模块输出，蓝牙模块的速率默认是115200）
		GPIO_SetBits(USARTsh_BT_GPIO, USARTsh_BT_Pin);
	}
	else
	{
		GPIO_ResetBits(USARTsh_BT_GPIO, USARTsh_BT_Pin);
	}
}


