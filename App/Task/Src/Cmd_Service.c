#include "App_Cmd.h"
#include "Cmd_Service.h"
#include "App_Rtos.h"
#include "Control.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "Uart_Protocol.h"

typedef enum
{
    CMD_MESSAGE_NOTICE = 0,
    CMD_MESSAGE_PARSE_ERROR,
    CMD_MESSAGE_CMD_RESULT,
	CMD_MESSAGE_STATUS,
	CMD_MESSAGE_FAULT_SNAPSHOT,
    CMD_MESSAGE_HELP,
    CMD_MESSAGE_STATS
} Cmd_MessageType_t;

typedef struct
{
    App_Cmd_Type_t command;
    App_Cmd_ExecResult_t result;
} Comm_CommandResult_t;

typedef struct
{
    Motor_Status_t status;
    float kp;
    float ki;
    float kd;
    uint8_t adc_target_enabled;
    uint32_t tick_ms;
} Comm_StatusMessage_t;

typedef struct
{
    uint8_t valid;
    FaultSnapshot_t snapshot;
} Comm_FaultMessage_t;

typedef struct
{
    Cmd_MessageType_t type;
    union
    {
        Uart_ProtocolNotice_t notice;
        App_Cmd_ParseResult_t parse_result;
        Comm_CommandResult_t command_result;
        Comm_StatusMessage_t status;
        Comm_FaultMessage_t fault;
        Comm_StatsSnapshot_t stats;
    } payload;
} Cmd_Message_t;



//控制命令_队列
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


//串口消息_队列
bool Cmd_Service_PostNotice(Uart_ProtocolNotice_t notice)
{
	return true;
}
