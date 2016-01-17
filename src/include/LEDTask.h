/*
 * LEDTask.h
 *
 *  Created on: Dec 6, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_LEDTASK_H_
#define INCLUDE_LEDTASK_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>

void InitLEDTask( void );
void LEDTask( void * pvParameters);

void SetErrorLED(BaseType_t bError);

// For Debug use only
void SetDebugLED(BaseType_t bOn);


extern const CLI_Command_Definition_t cmd_def_clearErr;

#endif /* INCLUDE_LEDTASK_H_ */
