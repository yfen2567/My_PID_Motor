/*
 * Control.h
 *
 *  Created on: May 27, 2026
 *      Author: yuan
 */

#ifndef INC_CONTROL_H_
#define INC_CONTROL_H_
#include<stdint.h>

extern volatile uint8_t flag_control_tick;

void Control_Tick10ms(void);


typedef enum {
    FAULT_NONE = 0,
    FAULT_SPEED_OVER_LIMIT,
    FAULT_ENCODER_LOST,
    FAULT_PWM_SATURATION,
} FaultCode_t;

typedef enum{
	SYS_IDLE=0,
	SYS_RUN,
	SYS_FAULT,
	SYS_CALIB,
}SystemState_t;

typedef struct{
	 int16_t PWM;
	 int32_t encoder_delta;
	 int32_t actual_speed;
	 int32_t target_speed;
	 uint16_t adc_raw;
	 float adc_filtered;
	 FaultCode_t fault;
	 SystemState_t state;
	 uint8_t enable;
}Motor_Status_t;


typedef struct
{
    uint8_t valid;
    uint32_t tick_ms;
    FaultCode_t fault;
    SystemState_t state;
    int32_t target_speed;
    int32_t actual_speed;
    int16_t pwm;
    uint16_t adc_target;
    uint16_t adc_aux;
} FaultSnapshot_t;


void Control_Init();
void Control_SetEnable(uint8_t enable);
void Control_SetTargetSpeed(int32_t target_speed);
void Control_GetStatusSnapshot(Motor_Status_t *status);
void Control_GetPID(float *kp, float *ki, float *kd);
void Control_SetPID(float kp, float ki, float kd);
void Control_SetKp(float kp);
void Control_SetKi(float ki);
void Control_SetKd(float kd);
void Control_UseAdcTarget(uint8_t enable);
uint8_t Control_IsAdcTargetEnabled(void);
const char* Control_StateName(SystemState_t state);
const char* Control_FaultName(FaultCode_t fault);
void Control_PID_Rst();
void Control_ResetFault();
void Control_ReportFault(FaultCode_t fault);
FaultSnapshot_t Control_GetFaultShot();
uint8_t Control_HasFaultShot();
#endif /* INC_CONTROL_H_ */

