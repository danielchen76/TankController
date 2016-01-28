/*
 * util.c
 *
 *  Created on: Jan 6, 2016
 *      Author: daniel
 */

#include "util.h"

BaseType_t IsTimeout(TickType_t tickNow, TickType_t tickLast, TickType_t tickTimeout)
{
	if (tickNow > tickLast)
	{
		return (tickNow - tickLast) >= tickTimeout;
	}
	else
	{
		return (portMAX_DELAY - tickLast + tickNow) >= tickTimeout;
	}
}


