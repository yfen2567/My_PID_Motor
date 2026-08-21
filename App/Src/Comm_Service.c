#include "Comm_Service.h"

#include "App_Rtos.h"
#include "Uart.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "Uart_Protocol.h"
#include "app_config.h"

static uint32_t s_dropped_count;
static uint32_t s_tx_error_count;
static uint32_t s_queue_used_max;

static void Comm_Service_Increment(uint32_t *counter)
{
    taskENTER_CRITICAL();
    (*counter)++;
    taskEXIT_CRITICAL();
}

static bool Comm_Service_PostMessage(const Comm_Message_t *message)
{
    uint32_t used;

    if ((message == NULL) || (CommTxQueueHandle == NULL)) { return false; }
    if (osMessageQueuePut(CommTxQueueHandle, message, 0U, 0U) != osOK)
    {
        Comm_Service_Increment(&s_dropped_count);
        return false;
    }

    used = osMessageQueueGetCount(CommTxQueueHandle);
    taskENTER_CRITICAL();
    if (used > s_queue_used_max) { s_queue_used_max = used; }
    taskEXIT_CRITICAL();
    return true;
}

bool Comm_Service_PostNotice(Uart_ProtocolNotice_t notice)
{
    Comm_Message_t message = {0};

    message.type = COMM_MESSAGE_NOTICE;
    message.payload.notice = notice;
    return Comm_Service_PostMessage(&message);
}

bool Comm_Service_PostParseError(App_Cmd_ParseResult_t result)
{
    Comm_Message_t message = {0};

    message.type = COMM_MESSAGE_PARSE_ERROR;
    message.payload.parse_result = result;
    return Comm_Service_PostMessage(&message);
}


bool Comm_Service_PostStatus(const Motor_Status_t *status,
                             float kp, float ki, float kd,
                             uint8_t adc_target_enabled,
                             uint32_t tick_ms)
{
    Comm_Message_t message = {0};

    if (status == NULL) { return false; }

    message.type = COMM_MESSAGE_STATUS;
    message.payload.status.status = *status;
    message.payload.status.kp = kp;
    message.payload.status.ki = ki;
    message.payload.status.kd = kd;
    message.payload.status.adc_target_enabled = adc_target_enabled;
    message.payload.status.tick_ms = tick_ms;

    return Comm_Service_PostMessage(&message);
}


bool Comm_Service_PostFaultSnapshot(uint8_t valid,
                                    const FaultSnapshot_t *snapshot)
{
    Comm_Message_t message = {0};
    message.type = COMM_MESSAGE_FAULT_SNAPSHOT;
    message.payload.fault.valid = (valid != 0U) ? 1U : 0U;
    if ((valid != 0U) && (snapshot != NULL))
    {
        message.payload.fault.snapshot = *snapshot;
    }
    return Comm_Service_PostMessage(&message);
}



static uint16_t Comm_Service_Format(const Comm_Message_t *message,
                                    char *buffer, uint16_t size)
{
    switch (message->type)
    {
        case COMM_MESSAGE_NOTICE:
            return Uart_Protocol_FormatNotice(
                message->payload.notice, buffer, size);

        case COMM_MESSAGE_PARSE_ERROR:
            return Uart_Protocol_FormatParseError(
                message->payload.parse_result, buffer, size);

        case COMM_MESSAGE_STATUS:
            return Uart_Protocol_FormatStatus(
                &message->payload.status.status,
                message->payload.status.kp,
                message->payload.status.ki,
                message->payload.status.kd,
                message->payload.status.adc_target_enabled,
                message->payload.status.tick_ms,
                buffer, size);

        case COMM_MESSAGE_FAULT_SNAPSHOT:
            return Uart_Protocol_FormatFaultSnapshot(
                message->payload.fault.valid,
                &message->payload.fault.snapshot,
                buffer, size);

        case COMM_MESSAGE_HELP:
            return Uart_Protocol_FormatHelp(buffer, size);
        default:
            return 0U;
    }
}


bool Comm_Service_ProcessTx(void)
{
    Comm_Message_t message;
    char buffer[APP_UART_TX_SIZE];
    uint16_t length;

    if ((CommTxQueueHandle == NULL) ||
        (osMessageQueueGet(CommTxQueueHandle, &message, NULL, osWaitForever) != osOK))
    {
        return false;
    }
    length = Comm_Service_Format(&message, buffer, sizeof(buffer));
    if ((length == 0U) || !Uart_WriteAsync((const uint8_t *)buffer, length) ||
        !Uart_WaitTxComplete(APP_UART_TX_TIMEOUT_MS))
    {
        Comm_Service_Increment(&s_tx_error_count);
    }
    return true;
}


bool Comm_Service_PostHelp(void)
{
    Comm_Message_t message = {0};

    message.type = COMM_MESSAGE_HELP;
    return Comm_Service_PostMessage(&message);
}

bool Comm_Service_PostCommandResult(App_Cmd_Type_t command,
                                    App_Cmd_ExecResult_t result)
{
    Comm_Message_t message = {0};
    message.type = COMM_MESSAGE_COMMAND_RESULT;
    message.payload.command_result.command = command;
    message.payload.command_result.result = result;
    return Comm_Service_PostMessage(&message);
}
