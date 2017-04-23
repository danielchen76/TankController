/*
 * tc_adc.h
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_TC_ADC_H_
#define INCLUDE_TC_ADC_H_

#include <FreeRTOS.h>

void InitADC();

void StartGetVoltage();

void transformADCValue(uint16_t* pDCVoltage,
					   uint16_t* pBatteryVoltage,
					   uint16_t* pMainCurrent,
					   uint16_t* pProteinSkimmerCurrent,
					   uint16_t* pMainPumpCurrent);

#endif /* INCLUDE_TC_ADC_H_ */
