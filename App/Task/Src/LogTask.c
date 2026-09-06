#include "LogTask.h"
#include "Comm_Service.h"
#include "Control.h"
#include "app_config.h"
#include "main.h"
#include "cmsis_os2.h"

bool LogTask_PostPeriodicStatus(void)
{
	Control_TelemetrySnapshot_t snapshot;

    Control_GetTelemetrySnapshot(&snapshot);

    return Comm_Service_PostStatus(
        snapshot,
        HAL_GetTick());
}

//处理获取错误状态快照命令
bool LogTask_PostFaultSnapshot(void)
{
    FaultSnapshot_t snapshot;
    if(Control_GetFaultShot(&snapshot))
    {
        return Comm_Service_PostFaultSnapshot(1U, &snapshot);
    }
    return Comm_Service_PostFaultSnapshot(0U, NULL);
}

//统计控制时序
bool LogTask_PostControlTimingStats(void)
{
	/*获取stats*/
    Control_TimingStats_t stats;
    Control_Timing_GetStats(&stats);

    /*上报stats*/
    return Comm_Service_PostControlTiming(stats);
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
