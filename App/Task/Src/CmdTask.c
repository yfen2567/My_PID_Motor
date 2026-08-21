#include "App_Cmd.h"
#include "CmdPool.h"
#include "CmdTask.h"
#include "Cmd_Service.h"
#include "Comm_Service.h"
#include "LogTask.h"
#include "Uart.h"
#include "app_config.h"
#include "cmsis_os.h"

#define CMD_TASK_MAX_LINES_PER_CYCLE  4U

static uint8_t CmdTask_ProcessLine(void);


static void CmdTask_PostUartEvents(uint32_t events)
{
	if((events&UART_EVENT_RX_OVERFLOW)!=0U)
	{
		(void)Comm_Service_PostNotice(UART_PROTOCOL_NOTICE_RX_OVERFLOW);
	}

	if((events&UART_EVENT_LINE_QUEUE_FULL)!=0U)
	{
		(void)Comm_Service_PostNotice(UART_PROTOCOL_NOTICE_CMD_LINE_QUEUE_FULL);
	}

	if((events&UART_EVENT_CMD_TOO_LONG)!=0U)
	{
		(void)Comm_Service_PostParseError(APP_CMD_PARSE_TOO_LONG);
	}
}

void StartCmdTask(void *argument)
{
	uint16_t count;
    (void)argument;

    for (;;)
    {
        CmdTask_PostUartEvents(Uart_Task());
        for(count=0U;count<CMD_TASK_MAX_LINES_PER_CYCLE;count++)
        {
        	if(CmdTask_ProcessLine()==0U){break;}
        }
        osDelay(5U);
    }
}

static uint8_t CmdTask_ProcessLine(void)
{
    char line[APP_UART_LINE_SIZE];
    App_Cmd_t parsed;
    App_Cmd_t *queued;
    App_Cmd_ParseResult_t parse_result;
    Cmd_ServiceResult_t queue_result;

    if (Uart_ReadLine(line, sizeof(line)) == 0U) { return 0U; }
    parse_result = App_Cmd_Parse(line, &parsed);
    if (parse_result != APP_CMD_PARSE_OK)
    {
        (void)Comm_Service_PostParseError(parse_result);
        return 1U;
    }

    if (parsed.type == APP_CMD_STATUS)
    {
        (void)LogTask_PostPeriodicStatus();
        return 1U;
    }
    if (parsed.type == APP_CMD_HELP)
    {
        (void)Comm_Service_PostHelp();
        return 1U;
    }
    if (parsed.type == APP_CMD_GET_FAULT)
    {
        (void)LogTask_PostFaultSnapshot();
        return 1U;
    }
    if (parsed.type == APP_CMD_COMM_STATS)
    {
        (void)Comm_Service_PostStats();
        return 1U;
    }

    queued = CmdPool_Alloc();
    if (queued == NULL)
    {
        (void)Comm_Service_PostCommandResult(parsed.type, APP_CMD_EXEC_POOL_EMPTY);
        return 1U;
    }
    *queued = parsed;
    queue_result = Cmd_Service_PostControlCommand(queued);
    if (queue_result != CMD_SERVICE_OK)
    {
        CmdPool_Free(queued);
        (void)Comm_Service_PostCommandResult(parsed.type,
            (queue_result == CMD_SERVICE_QUEUE_FULL) ?
            APP_CMD_EXEC_QUEUE_FULL : APP_CMD_EXEC_BAD_ARG);
    }
    return 1U;
}

