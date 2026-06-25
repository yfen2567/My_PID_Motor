/*电机控制模块（底层模块提供能力）
1.初始化电机：本质是启动“输出电机所需的PWM”的定时器
2.判断正反转
3.把接收到的PWM值转换为compare（设置占空比）*/

#include "Motor.h"
#include "app_config.h"
#include "main.h"
#include "tim.h"
//电机初始化及其PWM
void Motor_Init(){
	if(HAL_TIM_Base_Start(&htim1)!=HAL_OK){
		Error_Handler();
	};

	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1)!=HAL_OK){
			Error_Handler();
	};
}

static int16_t Motor_LimitPWM(int16_t PWM){

	if(PWM>MOTOR_PWM_ABS_MAX){
		PWM=MOTOR_PWM_ABS_MAX;
	}
	if(PWM<(-MOTOR_PWM_ABS_MAX)){
			PWM=(-MOTOR_PWM_ABS_MAX);
		}

	return PWM;
}

void Motor_SetPWM(int16_t PWM){
	int16_t abs_PWM;
	int16_t limit_PWM;
	limit_PWM=Motor_LimitPWM(PWM);
	if(limit_PWM>0){
		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin,GPIO_PIN_RESET);
		abs_PWM=limit_PWM;
	}
	else if(limit_PWM<0){

		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin,GPIO_PIN_SET);
		abs_PWM=(-limit_PWM);
	}
	else{
		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin,GPIO_PIN_RESET);
		abs_PWM = 0;
	}

	uint16_t compare;
	uint16_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
	compare = ((uint32_t)abs_PWM * period) / MOTOR_PWM_ABS_MAX;
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,compare);
}
//========================================================================

//停止电机
void Motor_Stop(){
	HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin,GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,0);
}
