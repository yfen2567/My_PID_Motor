#include "NvTask.h"
#include "ParamStore.h"
#include "Nv_Service.h"
#include "App_Cmd.h"
#include "CmdPool.h"
#include <stddef.h>
#include "Cmd_Service.h"
#include "cmsis_os2.h"
#include "Comm_Service.h"
#define NV_INTERNAL_COMMAND_RETRY_MS  100U

static uint8_t NvTask_PostApplyCommand(App_Cmd_Type_t origin,
                                       const ParamStore_Param_t *params)
{
    App_Cmd_t *cmd;
    uint32_t elapsed = 0U;

    while (elapsed < NV_INTERNAL_COMMAND_RETRY_MS)
    {
        cmd = CmdPool_Alloc();
        if (cmd != NULL)
        {
            cmd->type = APP_CMD_APPLY_STORED_PARAMS;
            cmd->origin_type = origin;
            cmd->kp = params->kp;
            cmd->ki = params->ki;
            cmd->kd = params->kd;
            if (Cmd_Service_PostControlCommand(cmd) == CMD_SERVICE_OK)
            {
                return 1U;
            }
            CmdPool_Free(cmd);
        }
        osDelay(1U);
        elapsed++;
    }
    return 0U;
}


void StartNvTask(void *argument)
{
    NvRequest_t request;
    ParamStore_Param_t params;
    bool success;

    (void)argument;
    for (;;)
    {
        if (!Nv_Service_Wait(&request)) { continue; }

        if (request.operation == APP_CMD_SAVE_PARAMS)
        {
            success = ParamStore_Save(&request.params);
            Nv_Service_Complete();
            (void)Comm_Service_PostCommandResult(
                request.operation, success ? APP_CMD_EXEC_OK :
                APP_CMD_EXEC_STORAGE_WRITE_FAILED);
            continue;
        }

        if (request.operation == APP_CMD_LOAD_PARAMS)
        {
            success = ParamStore_Load(&params);
        }
        else if (request.operation == APP_CMD_RESET_PARAMS)
        {
            success = ParamStore_Reset(&params);
        }
        else
        {
            success = false;
        }

        if (!success)
        {
            Nv_Service_Complete();
            (void)Comm_Service_PostCommandResult(
                request.operation,
                (request.operation == APP_CMD_LOAD_PARAMS) ?
                APP_CMD_EXEC_STORAGE_READ_FAILED :
                APP_CMD_EXEC_STORAGE_WRITE_FAILED);
        }
        else if (NvTask_PostApplyCommand(request.operation, &params) == 0U)
        {
            Nv_Service_Complete();
            (void)Comm_Service_PostCommandResult(
                request.operation, APP_CMD_EXEC_POOL_EMPTY);
        }
        /* On success, ControlTask applies gains, replies, then clears busy. */
    }
}
