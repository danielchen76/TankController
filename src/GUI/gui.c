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
//	gdispClear(Black);
//	for (int j = 0; j < 84; j++)
//	{
//
//		for (int i = 0; i < 48; i++)
//		{
//			gdispDrawPixel(j, i, White);
//			gdispFlush();
//			vTaskDelay(50 / portTICK_RATE_MS);
//		}
//		//gdispFlush();
//	}
	gdispFillArea(0, 0, 83, 47, White);
	gdispFlush();

	//gdispDrawLine(82, 0, 82, 47, White);

	// 循环处理GUI消息
	while (1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}



