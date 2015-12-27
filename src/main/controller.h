/*
 * controller.h
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#ifndef MAIN_CONTROLLER_H_
#define MAIN_CONTROLLER_H_

#include <FreeRTOS.h>

void InitMainMsgQueue( void );
void controller_entry(void);

#endif /* MAIN_CONTROLLER_H_ */
