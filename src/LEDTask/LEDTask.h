/*
 * LEDTask.h
 *
 *  Created on: Dec 6, 2015
 *      Author: daniel
 */

#ifndef LEDTASK_LEDTASK_H_
#define LEDTASK_LEDTASK_H_

#include <FreeRTOS.h>

void InitLEDTask( void );
void LEDTask( void * pvParameters);

void SetErrorLED(BaseType_t bError);

// For Debug use only
void SetDebugLED(BaseType_t bOn);


#endif /* LEDTASK_LEDTASK_H_ */
