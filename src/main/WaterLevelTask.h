/*
 * WaterLevelTask.h
 *
 *监控所有缸的水位的任务，并控制各个水泵启动和停止，将水位数据，泵的状态和启动、停止事件传递给主处理线程
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#ifndef MAIN_WATERLEVELTASK_H_
#define MAIN_WATERLEVELTASK_H_

#include <FreeRTOS.h>

void InitWaterLevelMsgQueue( void );
void WaterLevelControlTask( void * pvParameters);


#endif /* MAIN_WATERLEVELTASK_H_ */
