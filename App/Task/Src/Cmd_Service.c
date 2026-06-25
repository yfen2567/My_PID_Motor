#include "App_Cmd.h"
#include "Cmd_Service.h"
#include "App_Rtos.h"

#include "cmsis_os2.h"
#include "portable.h"

Cmd_ServiceResult_t Cmd_Service_PostControlCommand(const App_Cmd_t *cmd)
{
    App_Cmd_t *queued_cmd = NULL;

    if (cmd == NULL)
    {
        return CMD_SERVICE_BAD_ARG;
    }

    if (CmdQueueHandle == NULL)
    {
        return CMD_SERVICE_QUEUE_NULL;
    }

    queued_cmd = (App_Cmd_t *)pvPortMalloc(sizeof(App_Cmd_t));
    if (queued_cmd == NULL)
    {
        return CMD_SERVICE_ALLOC_FAIL;
    }

    *queued_cmd = *cmd;

    if (osMessageQueuePut(CmdQueueHandle, &queued_cmd, 0U, 0U) != osOK)
    {
        vPortFree(queued_cmd);
        return CMD_SERVICE_QUEUE_FULL;
    }

    return CMD_SERVICE_OK;
}

bool Cmd_Service_TryGetControlCommand(App_Cmd_t *cmd_out)
{
    App_Cmd_t *cmd_ptr = NULL;

    if ((cmd_out == NULL) || (CmdQueueHandle == NULL))
    {
        return false;
    }

    if (osMessageQueueGet(CmdQueueHandle, &cmd_ptr, NULL, 0U) != osOK)
    {
        return false;
    }

    if (cmd_ptr == NULL)
    {
        return false;
    }

    *cmd_out = *cmd_ptr;
    vPortFree(cmd_ptr);
    return true;
}
