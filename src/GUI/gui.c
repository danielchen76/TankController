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
	//gdispFillArea(0, 0, 83, 47, White);


	font_t		font1;

	font1 = gdispOpenFont("fixed_10x20");
	gdispDrawString(10, 10, "Display", font1, 0x3);
	gdispDrawString(10, 30, "Display", font1, 0x2);
	gdispDrawString(10, 50, "Display", font1, 0x1);

	gdispDrawLine(0, 90, 127, 90, 0x3);
	gdispDrawLine(0, 91, 127, 91, 0x2);
	gdispDrawLine(0, 92, 127, 92, 0x1);

	gdispFlush();

	gdispSetContrast(27);
//	for (int i = 0; i < 100; i++)
//	{
//		gdispSetContrast(i);
//		vTaskDelay(200 / portTICK_RATE_MS);
//	}

	// 循环处理GUI消息
	while (1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}



