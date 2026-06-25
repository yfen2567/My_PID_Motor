#include "Cmd_Service.h"
#include "Control_Task.h"
#include "Control.h"

#include "cmsis_os.h"

void StartControlTask(void *argument)
{
    App_Cmd_t cmd;

    (void)argument;

    for (;;)
    {
        while (Cmd_Service_TryGetControlCommand(&cmd))
        {
            Control_ApplyCommand(&cmd);
        }

        Control_Tick10ms();
        osDelay(10U);
    }
}
