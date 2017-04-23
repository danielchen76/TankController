/*
 * controller.h
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_CONTROLLER_H_
#define INCLUDE_CONTROLLER_H_

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>

void InitMainMsgQueue( void );
void controller_entry(void);

extern const CLI_Command_Definition_t cmd_def_sys;
extern const CLI_Command_Definition_t cmd_def_backuppower;
extern const CLI_Command_Definition_t cmd_def_uptime;
extern const CLI_Command_Definition_t cmd_def_power;

#endif /* INCLUDE_CONTROLLER_H_ */
