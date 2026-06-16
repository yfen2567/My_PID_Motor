/*
 * Motor.h
 *
 *  Created on: May 26, 2026
 *      Author: yuan
 */

#ifndef CORE_MOTOR_H_
#define CORE_MOTOR_H_
#include <stdint.h>



void Motor_SetPWM(int16_t PWM);
void Motor_Init();
void Motor_Stop();
#endif /* CORE_MOTOR_H_ */
