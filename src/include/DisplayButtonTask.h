/*
 * DisplayButtonTask.h
 *
 *  Created on: May 7, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_DISPLAYBUTTONTASK_H_
#define INCLUDE_DISPLAYBUTTONTASK_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>

void InitLEDButton( void );
void InitDisplayButtonTask( void );
void DisplayButtonTask( void * pvParameters);

void SetErrorLED(BaseType_t bError);

// For Debug use only
void SetDebugLED(BaseType_t bOn);


extern const CLI_Command_Definition_t cmd_def_clearErr;


#endif /* INCLUDE_DISPLAYBUTTONTASK_H_ */
