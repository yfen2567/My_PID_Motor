#include "Cmd_Service.h"
#include "Control_Task.h"
#include "Control.h"
#include "App_Cmd.h"
#include "cmsis_os.h"

#include "Nv_Service.h"
#include "Comm_Service.h"
#include "App_Cmd.h"
#include "CmdPool.h"
#include "Control_Task.h"

static App_Cmd_ExecResult_t ControlTask_PostNvRequest(const App_Cmd_t *cmd)
{
    Motor_Status_t status;
    NvRequest_t request = {0};

    Control_GetStatusSnapshot(&status);
    if ((status.enable != 0U) || (status.state != SYS_IDLE))
    {
        return APP_CMD_EXEC_NOT_IDLE;
    }
    if (Nv_Service_IsBusy()) { return APP_CMD_EXEC_BUSY; }

    request.operation = cmd->type;
    if (cmd->type == APP_CMD_SAVE_PARAMS)
    {
        Control_GetPID(&request.params.kp, &request.params.ki, &request.params.kd);
    }
    return Nv_Service_Post(&request) ? APP_CMD_EXEC_OK : APP_CMD_EXEC_QUEUE_FULL;
}

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
