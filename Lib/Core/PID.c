/* PID算法模块（底层模块提供能力）
1.PID初始化：设置一些控制参数
2.积分和输出限制：避免PWM过大导致占空比超出100%，超过的首先是没有物理意义，其次不设置范围容易引起误判
3.PID数据更新：接收新数据后，计算结果，然后对旧数据进行覆盖
4.PID控制参数置0：用于run=0，或者run=1刚开始时清楚之前的数据残留
5.设置控制参数*/

#include "PID.h"


//PID
static void PID_integral_Limit(PID_t* s_pid){
	if(s_pid->ki>0){
		s_pid->integral_abs_MAX=s_pid->output_abs_MAX/s_pid->ki;
	}
	else if(s_pid->ki==0){
		s_pid->integral_abs_MAX=s_pid->output_abs_MAX;
	}
	else{
		s_pid->integral_abs_MAX=-(s_pid->output_abs_MAX/s_pid->ki);
	}
	if((s_pid->integral)>(s_pid->integral_abs_MAX)){
			(s_pid->integral)=s_pid->integral_abs_MAX;
		}
		if((s_pid->integral)<-(s_pid->integral_abs_MAX)){
			(s_pid->integral)=-(s_pid->integral_abs_MAX);
		}

}
static void PID_output_integral_Limit(PID_t* s_pid){
	if((s_pid->output)>(s_pid->output_abs_MAX)){
		(s_pid->output)=s_pid->output_abs_MAX;
	}
	if((s_pid->output)<-(s_pid->output_abs_MAX)){
		(s_pid->output)=-(s_pid->output_abs_MAX);
	}
}



//PID初始化
void PID_Init(
		PID_t* pid,
		float kp,
		float ki,
		float kd,
		float dt,
		float output_abs_MAX
)
{
	if (pid == NULL) return;
	PID_Reset(pid);
	pid->kp=kp;
	pid->ki=ki;
	pid->kd=kd;
	pid->dt=dt;
	pid->integral_abs_MAX=output_abs_MAX;
	pid->output_abs_MAX=output_abs_MAX;

}
//========================================================================================

//PID更新数据
float PID_Update(PID_t* s_pid,float target_speed,float actual_speed){
	if (s_pid == NULL||s_pid->dt <= 0.0f) return -1.0;
	s_pid->error=target_speed-actual_speed;
	s_pid->integral+=s_pid->error*s_pid->dt;
	s_pid->derivative=(s_pid->error-s_pid->last_error)/s_pid->dt;
	PID_integral_Limit(s_pid);

	s_pid->p_term=(s_pid->kp)*(s_pid->error);
    s_pid->i_term=(s_pid->ki)*(s_pid->integral);
    s_pid->d_term=(s_pid->kd)*(s_pid->derivative);
    s_pid->output=s_pid->p_term+s_pid->i_term+s_pid->d_term;
    PID_output_integral_Limit(s_pid);
    s_pid->last_error = s_pid->error;
    return s_pid->output;
}
//========================================================================================


//PID数据置0
void PID_Reset(PID_t* pid)
{
	if (pid == NULL) return;
	pid->error=0;
	pid->last_error=0;
	pid->derivative=0;
	pid->integral=0;
	pid->output=0;

}

void PID_SetGains(PID_t* s_pid,float kp,float ki,float kd){
	if (s_pid == NULL) return;
	s_pid->kp=kp;
	s_pid->ki=ki;
	s_pid->kd=kd;
}
