#include "App_Cmd.h"
#include "CmdTask.h"
#include "Cmd_Service.h"
#include "LogTask.h"
#include "Uart.h"
#include "app_config.h"
#include "cmsis_os.h"

#define CMD_TASK_MAX_LINES_PER_CYCLE  4U

static uint8_t CmdTask_ProcessLine(void);
static void CmdTask_PrintHelp(void);

static void CmdTask_PostUartEvents(uint32_t events)
{
	if((events&UART_EVENT_RX_OVERFLOW)!=0U)
	{
		(void)Cmd_Service_PostNotice(UART_PROTOCOL_NOTICE_RX_OVERFLOW);
	}

	if((events&UART_EVENT_LINE_QUEUE_FULL)!=0U)
	{
		(void)Cmd_Service_PostNotice(UART_PROTOCOL_NOTICE_CMD_LINE_QUEUE_FULL);
	}

	if((events&UART_EVENT_CMD_TOO_LONG)!=0U)
	{
		(void)Cmd_Service_PostParseError(APP_CMD_PARSE_TOO_LONG);
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
    App_Cmd_t cmd;
    Cmd_ServiceResult_t result;
    App_Cmd_ParseResult_t parse_result;

    if (Uart_ReadLine(line, sizeof(line)) == 0U)
    {
        return;
    }

    parse_result=App_Cmd_Parse(line, &cmd);
    if (!App_Cmd_Parse(line, &cmd))
    {
        Uart_TxText("ERR:BAD_CMD\r\n");
        return;
    }

    if(cmd.type==APP_CMD_SET_TARGET&&
    		cmd.value>(-APP_TARGET_SPEED_MIN_RUN)&&
			cmd.value<APP_TARGET_SPEED_MIN_RUN){
        Uart_TxText("ERR:TARGET_BELOW_MIN\r\n");
        return;
    }

    if (cmd.type == APP_CMD_STATUS)
    {
        LogTask_PrintPeriodicStatus();
        return;
    }

    if (cmd.type == APP_CMD_HELP)
    {
        CmdTask_PrintHelp();
        return;
    }

    if (cmd.type == APP_CMD_GET_FAULT)
    {
        LogTask_PrintFaultSnapshot();
        return;
    }


    result = Cmd_Service_PostControlCommand(&cmd);
    switch (result)
    {
        case CMD_SERVICE_OK:
            Uart_TxText("OK:CMD_QUEUED\r\n");
            break;

        case CMD_SERVICE_ALLOC_FAIL:
            Uart_TxText("ERR:CMD_ALLOC_FAIL\r\n");
            break;

        case CMD_SERVICE_QUEUE_FULL:
            Uart_TxText("ERR:CMD_QUEUE_FULL\r\n");
            break;

        case CMD_SERVICE_QUEUE_NULL:
            Uart_TxText("ERR:CMD_QUEUE_NULL\r\n");
            break;

        case CMD_SERVICE_BAD_ARG:
        default:
            Uart_TxText("ERR:BAD_CMD\r\n");
            break;
    }
}

static void CmdTask_PrintHelp(void)
{
    Uart_TxText(
        "cmd:\r\n"
        "  run=1:start\r\n"
        "  stop\r\n"
        "  t=num:set target\r\n"
        "  set target adc\r\n"
        "  set target uart\r\n"
        "  kp=num:set kp 0.5\r\n"
        "  ki=num:set ki 0.1\r\n"
        "  kd=num:set kd 0\r\n"
        "  rst:reset\r\n"
        "  status\r\n"
        "  help\r\n"
        "  get fault:return shot\r\n");
}
