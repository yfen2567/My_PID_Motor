/*
 * ADC_Sensor.h
 *
 *  Created on: May 31, 2026
 *      Author: yuan
 */

#ifndef INC_ADC_SENSOR_H_
#define INC_ADC_SENSOR_H_

#include <stdint.h>

void ADC_Init();
void ADC_Update();
int32_t ADC_MapTargetSpeed();
uint16_t ADC_Get_Raw();
uint16_t ADC_Get_Aux_Raw(void);
float ADC_Get_Filtered();
#endif /* INC_ADC_SENSOR_H_ */
