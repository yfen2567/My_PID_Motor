/*
 * PID.h
 *
 *  Created on: May 26, 2026
 *      Author: yuan
 */

#ifndef INC_PID_H_
#define INC_PID_H_

#include "app_config.h"
#include <stddef.h>
typedef struct{
	float kp;
	float ki;
	float kd;

	float dt;

	float error;
	float integral;
	float last_error;
	float derivative;

	float p_term;
	float i_term;
	float d_term;

	float output;
	float output_abs_MAX;
	float integral_abs_MAX;

}PID_t;

void PID_Init(
		PID_t* pid,
		float kp,
		float ki,
		float kd,
		float dt,
		float output_MAX);
void PID_Reset(PID_t* pid);
float PID_Update(PID_t* s_pid,float target_speed,float actual_speed);
void PID_SetGains(PID_t* s_pid,float kp,float ki,float kd);
#endif /* INC_PID_H_ */
