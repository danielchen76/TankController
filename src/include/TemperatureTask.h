/*
 * TemperatureTask.h
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_TEMPERATURETASK_H_
#define INCLUDE_TEMPERATURETASK_H_

#include <FreeRTOS.h>

void InitTempMsgQueue( void );
void TempControlTask( void * pvParameters);

extern const CLI_Command_Definition_t cmd_def_gettemp;


#endif /* INCLUDE_TEMPERATURETASK_H_ */
