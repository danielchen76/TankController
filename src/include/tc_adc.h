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

int16_t GetDCVoltage();
int16_t GetBackupDCVoltage();

int16_t GetMainPowerCurrent();
int16_t GetMainPumpCurrent();
int16_t GetProteinSkimmerCurrent();


#endif /* INCLUDE_TC_ADC_H_ */
