#include "Cmd_Service.h"
#include "Control_Task.h"
#include "Control.h"

#include "cmsis_os.h"

void StartControlTask(void *argument)
{
    App_Cmd_t *cmd;
    App_Cmd_ExecResult_t result;
    App_Cmd_Type_t reply_command;
    TickType_t last_time = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10);

    (void)argument;
    for (;;)
    {
        while (Cmd_Service_TryGetControlCommand(&cmd))
        {
            if (cmd->type == APP_CMD_APPLY_STORED_PARAMS)
            {
                reply_command = cmd->origin_type;
                result = Control_ApplyCommand(cmd);
                Nv_Service_Complete();
                (void)Comm_Service_PostCommandResult(reply_command, result);
            }
            else if (App_Cmd_IsParamStoreCommand(cmd->type) != 0U)
            {
                result = ControlTask_PostNvRequest(cmd);
                if (result != APP_CMD_EXEC_OK)
                {
                    (void)Comm_Service_PostCommandResult(cmd->type, result);
                }
                /* Success is deferred until NvTask has actually finished. */
            }
            else if (Nv_Service_IsBusy() && (cmd->type != APP_CMD_STOP))
            {
                (void)Comm_Service_PostCommandResult(cmd->type, APP_CMD_EXEC_BUSY);
            }
            else
            {
                result = Control_ApplyCommand(cmd);
                (void)Comm_Service_PostCommandResult(cmd->type, result);
            }
            CmdPool_Free(cmd);
        }

        Control_Tick10ms();
        vTaskDelayUntil(&last_time, period);
    }
}
