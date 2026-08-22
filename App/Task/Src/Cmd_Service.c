#include "Cmd_Service.h"
#include "App_Rtos.h"
#include "cmsis_os2.h"
#include "portable.h"

#include "FreeRTOS.h"
#include "task.h"

static uint32_t s_queue_full_count;
static uint32_t s_queue_used_max;

//控制命令_队列
Cmd_ServiceResult_t Cmd_Service_PostControlCommand(App_Cmd_t *cmd)
{
    uint32_t used;

    if (cmd == NULL) { return CMD_SERVICE_BAD_ARG; }
    if (ControlCmdQueueHandle == NULL) { return CMD_SERVICE_QUEUE_NULL; }

    if (osMessageQueuePut(ControlCmdQueueHandle, &cmd, 0U, 0U) != osOK)
    {
        taskENTER_CRITICAL();
        s_queue_full_count++;
        taskEXIT_CRITICAL();
        return CMD_SERVICE_QUEUE_FULL;
    }

    used = osMessageQueueGetCount(ControlCmdQueueHandle);
    taskENTER_CRITICAL();
    if (used > s_queue_used_max) { s_queue_used_max = used; }
    taskEXIT_CRITICAL();
    return CMD_SERVICE_OK;
}



bool Cmd_Service_TryGetControlCommand(App_Cmd_t **cmd_out)
{
    if ((cmd_out == NULL) || (ControlCmdQueueHandle == NULL)) { return false; }
    *cmd_out = NULL;
    if (osMessageQueueGet(ControlCmdQueueHandle, cmd_out, NULL, 0U) != osOK)
    {
        return false;
    }
    return (*cmd_out != NULL);
}

uint32_t Cmd_Service_GetQueueFullCount(void)
{
    return s_queue_full_count;
}

uint32_t Cmd_Service_GetQueueUsed(void)
{
    return (ControlCmdQueueHandle != NULL) ?
           osMessageQueueGetCount(ControlCmdQueueHandle) :
           0U;
}

uint32_t Cmd_Service_GetQueueUsedMax(void)
{
    return s_queue_used_max;
}

uint32_t Cmd_Service_GetQueueCapacity(void)
{
    return 16U;
}
