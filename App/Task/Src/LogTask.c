#include "LogTask.h"
#include "Control.h"
#include "Uart.h"
#include "app_config.h"
#include "main.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include <stdio.h>
#include "usart.h"
#include "string.h"

static void LogTask_FormatFloat3(char *buf, uint8_t size, float num)
{
    uint32_t scaled = 0U;
    uint32_t integer = 0U;
    uint32_t fraction = 0U;

    if (num >= 0.0f)
    {
        scaled = (uint32_t)(num * 1000.0f + 0.5f);
        integer = scaled / 1000U;
        fraction = scaled % 1000U;
        snprintf(buf, size, "%lu.%03lu", (unsigned long)integer, (unsigned long)fraction);
    }
    else
    {
        scaled = (uint32_t)((-num) * 1000.0f + 0.5f);
        integer = scaled / 1000U;
        fraction = scaled % 1000U;
        snprintf(buf, size, "-%lu.%03lu", (unsigned long)integer, (unsigned long)fraction);
    }
}

void LogTask_PrintPeriodicStatus(void)
{
    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
    Motor_Status_t motor_status;
    char kp_text[16];
    char ki_text[16];
    char kd_text[16];
    char message[APP_UART_TX_SIZE] = {0};

    Control_GetStatusSnapshot(&motor_status);
    Control_GetPID(&kp, &ki, &kd);
    LogTask_FormatFloat3(kp_text, sizeof(kp_text), kp);
    LogTask_FormatFloat3(ki_text, sizeof(ki_text), ki);
    LogTask_FormatFloat3(kd_text, sizeof(kd_text), kd);

     snprintf(message, sizeof(message),
              "ms:%lu,enable:%d,State:%s,source:%s,target:%ld,actual:%ld,delta:%ld,PWM:%d,adc1:%u,adc2:%u,fault:%s,kp:%s,ki:%s,kd:%s\r\n",
             (unsigned long)HAL_GetTick(),
             motor_status.enable,
             Control_StateName(motor_status.state),
             Control_IsAdcTargetEnabled() ? "ADC" : "Uart",
             (long)motor_status.target_speed,
             (long)motor_status.actual_speed,
             (long)motor_status.encoder_delta,
             motor_status.PWM,
             motor_status.adc_raw,
             motor_status.adc_aux_raw,
             Control_FaultName(motor_status.fault),
             kp_text,
             ki_text,
             kd_text);

    Uart_TxText(message);
}

//处理获取错误状态快照命令
void LogTask_PrintFaultSnapshot(void){

		if (Control_HasFaultShot() == 0) {
			Uart_TxText("FAULT_SNAPSHOT_NONE\r\n");
			return;
		}
		FaultSnapshot_t shot = Control_GetFaultShot();
		char Message[APP_UART_TX_SIZE] = {0};
		snprintf(Message, sizeof(Message),
		         "valid:%d,time:%lu,state:%s,fault:%s,T_speed:%ld,A_speed:%ld,pwm=%d,adc_target=%u,adc_aux=%u\r\n",
		         shot.valid,
		         shot.tick_ms,
		         Control_StateName(shot.state),
		         Control_FaultName(shot.fault),
		         (long)shot.target_speed,
		         (long)shot.actual_speed,
		         shot.pwm,
		         shot.adc_target,
		         shot.adc_aux);
				 Uart_TxText(Message);

}

void StartLogTask(void *argument){
	for(;;){
		LogTask_PrintPeriodicStatus();
		osDelay(3000);
	}
}
