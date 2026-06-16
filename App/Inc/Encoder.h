/*
 * Encoder.h
 *
 *  Created on: May 27, 2026
 *      Author: yuan
 */

#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

#include <stdint.h>

void Encoder_Init();
int16_t Encoder_GetDelta();
int32_t Encoder_Getspeed();

int16_t Encoder_GetLastDelta();
#endif /* INC_ENCODER_H_ */
