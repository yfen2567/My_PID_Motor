#include "App_Cmd.h"
#include "Cmd_Service.h"
#include "App_Rtos.h"
#include "Control.h"
#include "cmsis_os2.h"
#include "portable.h"
#include "Uart_Protocol.h"

static uint32_t s_dropped_count;
static uint32_t s_queue_used_max;



//控制命令_队列
Cmd_ServiceResult_t Cmd_Service_PostControlCommand(const App_Cmd_t *cmd)
{
    App_Cmd_t *queued_cmd = NULL;

    if (cmd == NULL)
    {
        return CMD_SERVICE_BAD_ARG;
    }

    if (ControlCmdQueueHandle == NULL)
    {
        return CMD_SERVICE_QUEUE_NULL;
    }

    queued_cmd = (App_Cmd_t *)pvPortMalloc(sizeof(App_Cmd_t));
    if (queued_cmd == NULL)
    {
        return CMD_SERVICE_ALLOC_FAIL;
    }

    *queued_cmd = *cmd;

    if (osMessageQueuePut(ControlCmdQueueHandle, &queued_cmd, 0U, 0U) != osOK)
    {
        vPortFree(queued_cmd);
        return CMD_SERVICE_QUEUE_FULL;
    }

    return CMD_SERVICE_OK;
}

bool Cmd_Service_TryGetControlCommand(App_Cmd_t *cmd_out)
{
    App_Cmd_t *cmd_ptr = NULL;

    if ((cmd_out == NULL) || (ControlCmdQueueHandle == NULL))
    {
        return false;
    }

    if (osMessageQueueGet(ControlCmdQueueHandle, &cmd_ptr, NULL, 0U) != osOK)
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

static void Comm_Service_Increment(uint32_t *counter)
{
	taskENTER_CRITICAL();
	(*counter)++;
	taskEXIT_CRITICAL();
}

static bool Cmd_Service_PostMessage(const Cmd_Message_t *message)
{
	uint32_t used;

	if(message==NULL||UartTxMsgQueueHandle==NULL){return false;}
	if(osMessageQueuePut(UartTxMsgQueueHandle, message, 0, 0)!=osOK)
	{
		Comm_Service_Increment(&s_dropped_count);
		return false;
	}

	used=osMessageQueueGetCount(UartTxMsgQueueHandle);
	taskENTER_CRITICAL();
	if(used>s_queue_used_max)
	{
		s_queue_used_max=used;
	}
	taskEXIT_CRITICAL();
	return true;
}

bool Cmd_Service_PostNotice(Uart_ProtocolNotice_t notice)
{
	Cmd_Message_t message={0};
	message.type=CMD_MESSAGE_NOTICE;
	message.payload.notice=notice;
	return Cmd_Service_PostMessage(&message);
}

bool Comm_Service_PostParseError(App_Cmd_ParseResult_t result)
{
	Cmd_Message_t message={0};
	message.type=CMD_MESSAGE_PARSE_ERROR;
	message.payload.parse_result=result;
	return Cmd_Service_PostMessage(&message);
}
