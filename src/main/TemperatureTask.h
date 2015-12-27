/*
 * TemperatureTask.h
 *
 *  Created on: Dec 27, 2015
 *      Author: daniel
 */

#ifndef MAIN_TEMPERATURETASK_H_
#define MAIN_TEMPERATURETASK_H_

#include <FreeRTOS.h>

void InitTempMsgQueue( void );
void TempControlTask( void * pvParameters);


#endif /* MAIN_TEMPERATURETASK_H_ */
