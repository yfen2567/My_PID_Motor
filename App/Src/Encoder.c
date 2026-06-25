/*编码器获取速度（底层模块提供能力）
1.编码器初始化
2.获取速度*/
#include "Encoder.h"
#include "tim.h"
#include "app_config.h"
#include "string.h"
#define Encoder_MAX 65535
#define Encoder_MAX_Half 32768
static uint16_t s_encoder_last=0;
static int16_t s_ecoder_last_delta=0;
static int16_t s_delta_buf[APP_ENCODER_SPEED_AVG_N];
static uint8_t s_delta_index = 0;
static int32_t s_delta_sum = 0;
//编码器
void Encoder_Init(){
	__HAL_TIM_SET_COUNTER(&htim2,Encoder_MAX_Half);
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	s_encoder_last = __HAL_TIM_GET_COUNTER(&htim2);
	memset(s_delta_buf,0,sizeof(s_delta_buf));
	s_delta_index=0;
	s_delta_sum=0;
}

int16_t Encoder_GetDelta(){
	uint16_t encoder_now=__HAL_TIM_GET_COUNTER(&htim2);
	s_ecoder_last_delta=encoder_now-s_encoder_last;
	s_encoder_last=encoder_now;
	return s_ecoder_last_delta;
}

int32_t Encoder_Getspeed(){
	//延长测速窗口可以让delta更平滑
	int16_t delta=Encoder_GetDelta();
	s_delta_sum-=s_delta_buf[s_delta_index];
	s_delta_buf[s_delta_index]=delta;
	s_delta_sum+=delta;
	s_delta_index++;
	if(s_delta_index>=APP_ENCODER_SPEED_AVG_N){
		s_delta_index=0;
	}

	float window_time=APP_CONTROL_DT_SEC*APP_ENCODER_SPEED_AVG_N;
	float counts_per_second=(float)s_delta_sum/window_time;
	float rpm=counts_per_second*60.0f/APP_ENCODER_COUNTER_PER_REV;
	return rpm;
}

int16_t Encoder_GetLastDelta(){
	return s_ecoder_last_delta;
}
