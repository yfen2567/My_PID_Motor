#include "LogTask.h"
#include "Comm_Service.h"
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


bool LogTask_PostPeriodicStatus(void)
{
    Motor_Status_t motor_status;
    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;

    Control_GetStatusSnapshot(&motor_status);
    Control_GetPID(&kp, &ki, &kd);

    return Comm_Service_PostStatus(
        &motor_status,
        kp,
        ki,
        kd,
        Control_IsAdcTargetEnabled(),
        HAL_GetTick());
}

//处理获取错误状态快照命令
bool LogTask_PostFaultSnapshot(void)
{
    FaultSnapshot_t snapshot;

    if (Control_HasFaultShot() == 0U)
    {
        return Comm_Service_PostFaultSnapshot(0U, NULL);
    }

    snapshot = Control_GetFaultShot();
    return Comm_Service_PostFaultSnapshot(1U, &snapshot);
}

void StartLogTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        (void)LogTask_PostPeriodicStatus();
        osDelay(APP_LOG_TICK);
    }
}
