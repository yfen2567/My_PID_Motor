//控制模块（上层模块组合能力）
/*1.定义所需变量：因为能力组合时需要清楚系统此时的状态
2.控制初始化:使用底层模块的功能函数进行初始化，符合“谁拥有变量，谁提供操作接口”的逻辑
3.电机控制：根据一定逻辑，对底层函数进行组合，实现所需功能
4.周期控制
5.外部接口：这些最核心的状态不能被别人随意修改，最好的办法就是直接不让别人知道（static），所
以外部想要修改这些，就必须通过外部接口函数*/
#include "Control.h"
#include "app_config.h"
#include "PID.h"
#include "Motor.h"
#include "Encoder.h"
#include "ADC_Sensor.h"
#include "tim.h"
static volatile uint8_t s_adc_target_enable = 1;
static volatile uint8_t s_pwm_start_boost_tick=0;
static volatile uint8_t s_pwm_start_boost_active=0;
static  PID_t s_pid;


static volatile Motor_Status_t s_motor_status;

static volatile uint8_t s_encoder_lost_count=0;
static volatile uint8_t s_motor_pwm_saturation_count=0;
static FaultSnapshot_t s_fault_snapshot;
//初始化
void Control_Init(){
	PID_Init(&s_pid, APP_PID_DEFAULT_KP, APP_PID_DEFAULT_KI, APP_PID_DEFAULT_KD, APP_CONTROL_DT_SEC, APP_PID_OUTPUT_MAX);
	ADC_Init();
	s_motor_status=(Motor_Status_t){0};

	s_adc_target_enable = 1;
	s_encoder_lost_count = 0;
	s_motor_pwm_saturation_count = 0;
	s_pwm_start_boost_tick=0;
	s_pwm_start_boost_active=0;

}
//==================================================================================
//保存故障状态快照
static void Control_SaveFaultSnapShot(FaultCode_t fault){
	if(s_fault_snapshot.valid!=0){
		return;
	}
	s_fault_snapshot.valid=1;
	s_fault_snapshot.tick_ms=HAL_GetTick();
	s_fault_snapshot.fault=fault;
	s_fault_snapshot.state=s_motor_status.state;
	s_fault_snapshot.actual_speed=s_motor_status.actual_speed;
	s_fault_snapshot.target_speed=s_motor_status.target_speed;
	s_fault_snapshot.pwm=s_motor_status.PWM;
	s_fault_snapshot.adc_target = ADC_Get_Raw();
	//s_fault_snapshot.adc_aux=
}


//====================================================================================
//保护系统
void Control_EnterFault(FaultCode_t fault){
	Control_SaveFaultSnapShot(fault);
	s_motor_status.fault=fault;
	s_motor_status.state=SYS_FAULT;
	s_motor_status.enable=0;
	Motor_Stop();
}

void Control_ReportFault(FaultCode_t fault){
	if(fault!=FAULT_NONE){
		Control_EnterFault(fault);
	}
}

static uint32_t Control_Abs(int32_t value){
	return (value>0)?value:(-value);
}

static FaultCode_t Control_CheckFault(){
	if(s_motor_status.state!=SYS_RUN){
		s_encoder_lost_count=0;
		s_motor_pwm_saturation_count=0;
		return FAULT_NONE;
	}

	if(Control_Abs(s_motor_status.actual_speed)>APP_SPEED_ABS_LIMIT){
		Control_EnterFault(FAULT_SPEED_OVER_LIMIT);
		return FAULT_SPEED_OVER_LIMIT;
	}

	if(Control_Abs(s_motor_status.target_speed)>APP_ENCODER_LOST_CHECK_TARGET&&
			Control_Abs(s_motor_status.PWM)>MOTOR_PWM_ABS_MAX/2&&
			Control_Abs(s_motor_status.actual_speed)<APP_ENCODER_LOST_CHECK_EPS){
		if(s_encoder_lost_count<APP_ENCODER_LOST_TICK){
			s_encoder_lost_count++;

		}
		else{
			s_encoder_lost_count=0;
			Control_EnterFault(FAULT_ENCODER_LOST);
			return FAULT_ENCODER_LOST;
		}
	}
	else{
		s_encoder_lost_count=0;
	}

	if(Control_Abs(s_motor_status.PWM)>=MOTOR_PWM_THRESHOLD){
		if(s_motor_pwm_saturation_count<APP_MOTOR_PWM_SATURATION_TICK){
			s_motor_pwm_saturation_count++;
		}
		else{
			s_motor_pwm_saturation_count=0;
			Control_EnterFault(FAULT_PWM_SATURATION);
			return FAULT_PWM_SATURATION;
		}
	}
	else{
		s_motor_pwm_saturation_count=0;
	}
	return FAULT_NONE;
}
//====================================================================================


void Control_ResetFault(){
	__disable_irq();
	s_motor_status.fault=FAULT_NONE;
	s_motor_status.state=SYS_IDLE;
	s_motor_status.PWM=0;
	s_motor_status.enable=0;
	s_encoder_lost_count=0;
	s_motor_pwm_saturation_count=0;
	PID_Reset(&s_pid);
	__enable_irq();
	Motor_Stop();
	s_fault_snapshot = (FaultSnapshot_t){0};
}
//====================================================================================

//目标速度限幅
static int32_t Control_TargetSpeed_Limit(int32_t target_speed){
	uint32_t abs_target_speed=0;
	abs_target_speed=target_speed>0?target_speed:(-target_speed);
	if(abs_target_speed<APP_TARGET_SPEED_MIN){
		abs_target_speed=APP_TARGET_SPEED_MIN;
		return abs_target_speed;
	}
	if(abs_target_speed>APP_TARGET_SPEED_MAX){
		abs_target_speed=APP_TARGET_SPEED_MAX;
	}

	if(target_speed>0){
		return abs_target_speed;
	}
	else{
		return -abs_target_speed;
	}
}
//=========================================================================================

//PWM前馈
int16_t Control_CalcFeedForward(int16_t target_speed){
	float abs_pwm_ff=0;
	float abs_target=0;

	if(target_speed==0){
		abs_pwm_ff=0;
		return abs_pwm_ff;
	}

	abs_target=(float)(target_speed>0?target_speed:(-target_speed));
	abs_pwm_ff=APP_PWM_FF_KS+APP_PWM_FF_KV*abs_target;

	if(abs_pwm_ff>APP_PID_OUTPUT_MAX){
		abs_pwm_ff=APP_PID_OUTPUT_MAX;
	}

	if(target_speed>0){
		return (uint16_t)abs_pwm_ff;
	}
	else{
		return (int16_t)(-abs_pwm_ff);
	}
}
//======================================================================================

//获取速度方向
static int8_t Control_GetSpeedSign(int32_t target_speed){
	int8_t sign=1;
	if(target_speed>0){
		sign=1;
		return sign;
	}
	else if(target_speed<0){
		sign=-1;
		return sign;
	}
	return 0;
}
//===========================================================================

//计算输出PWM
int16_t Control_CalcOutputPWM(PID_t* pid,int32_t target_speed,int32_t actual_speed){
	int16_t temp_PWM=0;
	if(s_pwm_start_boost_active!=0
			&&s_pwm_start_boost_tick<APP_PWM_START_BOOST_TICK
			&&target_speed!=0
			&&s_motor_status.encoder_delta==0){

		temp_PWM=APP_PWM_START_BOOST;

		PID_Reset(&s_pid);
		s_pwm_start_boost_tick++;

	}
	else{
		temp_PWM=Control_CalcFeedForward(target_speed)+PID_Update(pid,target_speed,actual_speed);
		s_pwm_start_boost_tick=0;
		s_pwm_start_boost_active=0;
	}
	if(temp_PWM>APP_PID_OUTPUT_MAX){
		temp_PWM=APP_PID_OUTPUT_MAX;
		}
	if(temp_PWM<(-APP_PID_OUTPUT_MAX)){
		temp_PWM=(-APP_PID_OUTPUT_MAX);
	}
	return temp_PWM;
}

//======================================================================================

//电机控制
static void Control_Update(){

	ADC_Update();
	s_motor_status.adc_raw=ADC_Get_Raw();
	s_motor_status.adc_aux_raw=ADC_Get_Aux_Raw();
	s_motor_status.adc_filtered=ADC_Get_Filtered();
	if(s_adc_target_enable){
	s_motor_status.target_speed=Control_TargetSpeed_Limit(ADC_MapTargetSpeed());
	}

	s_motor_status.actual_speed=Encoder_Getspeed();

	s_motor_status.encoder_delta=Encoder_GetLastDelta();

	if(s_motor_status.state==SYS_FAULT){
		Motor_Stop();
		return;
	}
	if(s_motor_status.state == SYS_CALIB){
	    s_motor_status.PWM = 0;
	    Motor_Stop();
	    return;
	}

	if(s_motor_status.enable==0){
		s_motor_status.state = SYS_IDLE;
		s_motor_status.PWM=0;
		Motor_Stop();
		return;
	}
	s_motor_status.state=SYS_RUN;
	int16_t temp_PWM=Control_CalcOutputPWM(&s_pid,s_motor_status.target_speed,s_motor_status.actual_speed);
	s_motor_status.PWM=temp_PWM;
	if(Control_CheckFault()!=FAULT_NONE){
		return;
	}
	Motor_SetPWM((int16_t)s_motor_status.PWM);

}



void Control_Tick10ms(void){
	Control_Update();
//	telemetry_due = 1;
}
//================================================================================

//获取电机状态
void Control_GetStatusSnapshot(Motor_Status_t *status){
	if(status==0){
		return;
	}
	__disable_irq();
	status->target_speed=s_motor_status.target_speed;
	status->actual_speed=s_motor_status.actual_speed;
	status->PWM=s_motor_status.PWM;
	status->state=s_motor_status.state;
	status->adc_raw=s_motor_status.adc_raw;
	status->adc_aux_raw=s_motor_status.adc_aux_raw;
	status->adc_filtered=s_motor_status.adc_filtered;
	status->encoder_delta=s_motor_status.encoder_delta;
	status->fault=s_motor_status.fault;
	status->enable=s_motor_status.enable;
	__enable_irq();
}

void Control_GetPID(float *kp, float *ki, float *kd){
	__disable_irq();
	if(kp!=0){
		*kp=s_pid.kp;
	}
	if(ki!=0){
		*ki=s_pid.ki;
	}
	if(kd!=0){
		*kd=s_pid.kd;
	}
	__enable_irq();
}


//设置数据


void Control_PID_Rst(){
	__disable_irq();
	PID_Reset(&s_pid);
	s_pwm_start_boost_active=0;
	s_pwm_start_boost_tick=0;
	__enable_irq();
}

//
void Control_SetEnable(uint8_t enable){
	__disable_irq();
	if(enable!=0){
		if(s_motor_status.fault==FAULT_NONE&&s_motor_status.state!=SYS_CALIB){
		s_motor_status.state=SYS_RUN;
		s_motor_status.enable=1;
		PID_Reset(&s_pid);
		}
		s_pwm_start_boost_active=1;
		s_pwm_start_boost_tick=0;
	}
	else{
		s_motor_status.enable=0;
		s_motor_status.PWM=0;
		PID_Reset(&s_pid);
		s_pwm_start_boost_active=0;
		s_pwm_start_boost_tick=0;
		if(s_motor_status.fault==FAULT_NONE){
			s_motor_status.state=SYS_IDLE;
		}
		else{
			s_motor_status.state=SYS_FAULT;
		}
	}
	__enable_irq();
	if(enable==0){
		Motor_Stop();
	}

}

void Control_SetTargetSpeed(int32_t target_speed){
	target_speed=Control_TargetSpeed_Limit(target_speed);
	__disable_irq();
	int8_t old_sign=Control_GetSpeedSign(s_motor_status.target_speed);
	int8_t new_sign=Control_GetSpeedSign(target_speed);
	s_adc_target_enable=0;
	s_motor_status.target_speed=target_speed;
	if(old_sign!=new_sign){
		PID_Reset(&s_pid);
		if(new_sign!=0){
			s_pwm_start_boost_active = 1;
			s_pwm_start_boost_tick = 0;
		}
		else{
			s_pwm_start_boost_active = 0;
			s_pwm_start_boost_tick = 0;
		}
	}
	__enable_irq();

}

void Control_SetPID(float kp, float ki, float kd){
	__disable_irq();
	PID_SetGains(&s_pid,kp,ki,kd);
	PID_Reset(&s_pid);
	__enable_irq();
}

void Control_SetKp(float kp){
	Control_SetPID(kp,s_pid.ki,s_pid.kd);
}

void Control_SetKi(float ki){
	Control_SetPID(s_pid.kp,ki,s_pid.kd);
}

void Control_SetKd(float kd){
	Control_SetPID(s_pid.kp,s_pid.ki,kd);
}

void Control_UseAdcTarget(uint8_t enable){
	s_adc_target_enable=enable?1:0;
}

uint8_t Control_IsAdcTargetEnabled(void){
	return s_adc_target_enable;
}

const char* Control_StateName(SystemState_t state){
	switch(state){
	case SYS_IDLE:
		return "IDLE";
	case SYS_RUN:
		return "RUN";
	case SYS_FAULT:
		return "FAULT";
	case SYS_CALIB:
		return "CALIB";
	default:
		return "UNKNOWN";
	}
}

const char* Control_FaultName(FaultCode_t fault){
	switch(fault){
	case FAULT_NONE:
		return "NONE";
	case FAULT_SPEED_OVER_LIMIT:
		return "SPEED_OVER_LIMIT";
	case FAULT_ENCODER_LOST:
		return "ENCODER_LOST";
	case FAULT_PWM_SATURATION:
		return "PWM_SATURATION";
	default:
		return "UNKNOWN";
	}
}



//获取错误状态快照对外接口
uint8_t Control_HasFaultShot(){
	return s_fault_snapshot.valid;
}

FaultSnapshot_t Control_GetFaultShot(){
		return s_fault_snapshot;

}


