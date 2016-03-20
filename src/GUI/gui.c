/*
 * gui.c
 *
 *  Created on: Mar 20, 2016
 *      Author: daniel
 */


#include <gui.h>
#include <task.h>

void GUITask( void * pvParameters)
{
	// 初始化gfx
	gfxInit();

	// test
	gdispDrawLine(82, 0, 82, 47, White);
	gdispFlush();


	// 循环处理GUI消息
	while (1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}



