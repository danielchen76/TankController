/*
 * TM1637.h
 *
 *  Created on: May 7, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_TM1637_H_
#define INCLUDE_TM1637_H_

#include <stdio.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"

#define SEG_A   0b00000001
#define SEG_B   0b00000010
#define SEG_C   0b00000100
#define SEG_D   0b00001000
#define SEG_E   0b00010000
#define SEG_F   0b00100000
#define SEG_G   0b01000000

void tm1637Init(void);
void tm1637SetBrightness(uint8_t brightness);
void tm1637DisplayDecimal(int v, int displaySeparator);


#endif /* INCLUDE_TM1637_H_ */
