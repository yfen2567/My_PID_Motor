#include "App_Cmd.h"
#include "CmdTask.h"
#include "Cmd_Service.h"
#include "LogTask.h"
#include "Uart.h"
#include "app_config.h"

#include "cmsis_os.h"

static void CmdTask_ProcessLine(void);
static void CmdTask_PrintHelp(void);

void StartCmdTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        CmdTask_ProcessLine();
        osDelay(5U);
    }
}

static void CmdTask_ProcessLine(void)
{
    char line[APP_UART_LINE_SIZE];
    App_Cmd_t cmd;
    Cmd_ServiceResult_t result;
    Uart_Task();

    if (Uart_ReadLine(line, sizeof(line)) == 0U)
    {
        return;
    }

    if (!App_Cmd_Parse(line, &cmd))
    {
        Uart_TxText("ERR:BAD_CMD\r\n");
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
