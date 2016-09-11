/*
 * util.h
 *
 *  Created on: Jan 6, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_UTIL_H_
#define INCLUDE_UTIL_H_

#include <FreeRTOS.h>

BaseType_t IsTimeout(TickType_t tickNow, TickType_t tickLast, TickType_t tickTimeout);

uint8_t crc8( uint8_t* data, uint16_t number_of_bytes_in_data );


#endif /* INCLUDE_UTIL_H_ */
