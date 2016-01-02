/*
 * shell_serial.c
 *
 *	Bluetooth module on UART for shell
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#include <stdio.h>

#include "tc_serial.h"
#include "UARTAdapter.h"

// 各个模块的头文件，用来包含shell命令函数入口
#include "setting/setting.h"

#define mainUART_COMMAND_CONSOLE_STACK_SIZE		( configMINIMAL_STACK_SIZE * 10 )
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY  	1


extern void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );

// Test command
static BaseType_t cmd_test( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	( void )pcCommandString;

	snprintf(pcWriteBuffer, xWriteBufferLen, "Test echo.\r\n");
	return pdFALSE;
}

const CLI_Command_Definition_t cmd_def_test =
{
	"test",
	"\r\ntest \r\n Test command, output test.\r\n",
	cmd_test, /* The function to run. */
	2
};

// Command entries
static const CLI_Command_Definition_t* commands[] =
{
	&cmd_def_test,
	&cmd_def_eeRead,
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


