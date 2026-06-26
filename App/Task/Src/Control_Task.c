#include "Cmd_Service.h"
#include "Control_Task.h"
#include "Control.h"

#include "cmsis_os.h"

void StartControlTask(void *argument)
{
    App_Cmd_t cmd;

    (void)argument;
    TickType_t last_time=xTaskGetTickCount();
    TickType_t period=pdMS_TO_TICKS(10);
    for (;;)
    {
        while (Cmd_Service_TryGetControlCommand(&cmd))
        {
            Control_ApplyCommand(&cmd);
        }

        Control_Tick10ms();
        vTaskDelayUntil(&last_time, period);
    }
}
