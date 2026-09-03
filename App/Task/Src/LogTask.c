#include "LogTask.h"
#include "Comm_Service.h"
#include "Control.h"
#include "app_config.h"
#include "main.h"
#include "cmsis_os2.h"

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

bool LogTask_PostControlTimingStats(void)
{

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
