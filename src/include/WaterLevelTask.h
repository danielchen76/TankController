/*
 * WaterLevelTask.h
 *
 *监控所有缸的水位的任务，并控制各个水泵启动和停止，将水位数据，泵的状态和启动、停止事件传递给主处理线程
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_WATERLEVELTASK_H_
#define INCLUDE_WATERLEVELTASK_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>

void InitWaterLevelMsgQueue( void );
void WaterLevelControlTask( void * pvParameters);


extern const CLI_Command_Definition_t cmd_def_wlstatus;
extern const CLI_Command_Definition_t cmd_def_wlset;
extern const CLI_Command_Definition_t cmd_def_wlpump;


#endif /* INCLUDE_WATERLEVELTASK_H_ */
