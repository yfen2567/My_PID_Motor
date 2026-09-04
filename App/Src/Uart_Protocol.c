#include "Uart_Protocol.h"
#include "stdio.h"
#include "App_Cmd.h"
#include "Control.h"

static uint16_t Uart_Protocol_FinalizeLength(int written, uint16_t buffer_size)
{
    if ((written <= 0) || (buffer_size == 0U))
    {
        return 0U;
    }

    if ((uint32_t)written >= buffer_size)
    {
        return 0U;
    }

    return (uint16_t)written;
}


static uint16_t Uart_Protocol_CopyText(const char *text,
                                       char *buffer,
                                       uint16_t buffer_size)
{
    int written;

    if ((text == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    written = snprintf(buffer, buffer_size, "%s", text);
    return Uart_Protocol_FinalizeLength(written, buffer_size);
}

uint16_t Uart_Protocol_FormatNotice(Uart_ProtocolNotice_t notice,
                                    char *buffer,
                                    uint16_t buffer_size)
{
    const char *text;

    switch (notice)
    {
        case UART_PROTOCOL_NOTICE_RX_OVERFLOW:
            text = "ERR:RX_OVERFLOW\r\n";
            break;
        case UART_PROTOCOL_NOTICE_CMD_LINE_QUEUE_FULL:
            text = "ERR:CMD_LINE_QUEUE_FULL\r\n";
            break;
        default:
            text = "ERR:BAD_CMD\r\n";
            break;
    }

    return Uart_Protocol_CopyText(text, buffer, buffer_size);
}


uint16_t Uart_Protocol_FormatParseError(App_Cmd_ParseResult_t result,
                                        char *buffer,
                                        uint16_t buffer_size)
{
    const char *text;

    switch (result)
    {
        case APP_CMD_PARSE_EMPTY: text = "ERR:EMPTY_CMD\r\n"; break;
        case APP_CMD_PARSE_OUT_OF_RANGE: text = "ERR:OUT_OF_RANGE\r\n"; break;
        case APP_CMD_PARSE_BAD_INT: text = "ERR:BAD_INT\r\n"; break;
        case APP_CMD_PARSE_BAD_FLOAT: text = "ERR:BAD_FLOAT\r\n"; break;
        case APP_CMD_PARSE_TOO_LONG: text = "ERR:CMD_TOO_LONG\r\n"; break;
        case APP_CMD_PARSE_INVALID:
        default: text = "ERR:BAD_CMD\r\n"; break;
    }
    return Uart_Protocol_CopyText(text, buffer, buffer_size);
}

uint16_t Uart_Protocol_FormatCommandResult(App_Cmd_Type_t command,
                                           App_Cmd_ExecResult_t result,
                                           char *buffer,
                                           uint16_t buffer_size)
{
    const char *text;

    if (result != APP_CMD_EXEC_OK)
    {
        switch (result)
        {
            case APP_CMD_EXEC_NOT_IDLE: text = "ERR:NOT_IDLE\r\n"; break;
            case APP_CMD_EXEC_BUSY: text = "ERR:BUSY\r\n"; break;
            case APP_CMD_EXEC_INVALID_STATE: text = "ERR:INVALID_STATE\r\n"; break;
            case APP_CMD_EXEC_OUT_OF_RANGE: text = "ERR:OUT_OF_RANGE\r\n"; break;
            case APP_CMD_EXEC_QUEUE_FULL: text = "ERR:QUEUE_FULL\r\n"; break;
            case APP_CMD_EXEC_POOL_EMPTY: text = "ERR:CMD_POOL_EMPTY\r\n"; break;
            case APP_CMD_EXEC_STORAGE_READ_FAILED:
                text = "ERR:PARAMS_LOAD_FAILED\r\n"; break;
            case APP_CMD_EXEC_STORAGE_WRITE_FAILED:
                text = "ERR:PARAMS_SAVE_FAILED\r\n"; break;
            case APP_CMD_EXEC_BAD_ARG:
            case APP_CMD_EXEC_UNSUPPORTED:
            default: text = "ERR:BAD_CMD\r\n"; break;
        }
    }
    else
    {
        switch (command)
        {
            case APP_CMD_RUN: text = "OK:RUN\r\n"; break;
            case APP_CMD_STOP: text = "OK:STOPPED\r\n"; break;
            case APP_CMD_SET_TARGET: text = "OK:TARGET_SET\r\n"; break;
            case APP_CMD_SET_TARGET_ADC: text = "OK:TARGET_SOURCE_ADC\r\n"; break;
            case APP_CMD_SET_TARGET_UART: text = "OK:TARGET_SOURCE_UART\r\n"; break;
            case APP_CMD_SET_KP: text = "OK:KP_SET\r\n"; break;
            case APP_CMD_SET_KI: text = "OK:KI_SET\r\n"; break;
            case APP_CMD_SET_KD: text = "OK:KD_SET\r\n"; break;
            case APP_CMD_SAVE_PARAMS: text = "OK:PARAMS_SAVED\r\n"; break;
            case APP_CMD_LOAD_PARAMS: text = "OK:PARAMS_LOADED\r\n"; break;
            case APP_CMD_RESET_PARAMS: text = "OK:PARAMS_RESET\r\n"; break;
            case APP_CMD_RESET: text = "OK:RESET\r\n"; break;
            default: text = "OK\r\n"; break;
        }
    }

    return Uart_Protocol_CopyText(text, buffer, buffer_size);
}


static void Uart_Protocol_FormatFloat3(char *buffer,
                                       uint16_t buffer_size,
                                       float value)
{
    uint32_t scaled;
    uint32_t integer;
    uint32_t fraction;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (value >= 0.0f)
    {
        scaled = (uint32_t)(value * 1000.0f + 0.5f);
        integer = scaled / 1000U;
        fraction = scaled % 1000U;
        (void)snprintf(buffer, buffer_size, "%lu.%03lu",
                       (unsigned long)integer,
                       (unsigned long)fraction);
    }
    else
    {
        scaled = (uint32_t)((-value) * 1000.0f + 0.5f);
        integer = scaled / 1000U;
        fraction = scaled % 1000U;
        (void)snprintf(buffer, buffer_size, "-%lu.%03lu",
                       (unsigned long)integer,
                       (unsigned long)fraction);
    }
}

static const char *Uart_Protocol_StateName(SystemState_t state)
{
    switch (state)
    {
        case SYS_IDLE:
            return "IDLE";
        case SYS_RUN:
            return "RUN";
        case SYS_FAULT:
            return "FAULT";
        case SYS_CALIB:
            return "CALIB";
        default:
            return "UNKNOWN";
    }
}



static const char *Uart_Protocol_FaultName(FaultCode_t fault)
{
    switch (fault)
    {
        case FAULT_NONE:
            return "NONE";
        case FAULT_SPEED_OVER_LIMIT:
            return "SPEED_OVER_LIMIT";
        case FAULT_ENCODER_LOST:
            return "ENCODER_LOST";
        case FAULT_PWM_SATURATION:
            return "PWM_SATURATION";
        default:
            return "UNKNOWN";
    }
}



uint16_t Uart_Protocol_FormatStatus(const Motor_Status_t *status,
                                    float kp,
                                    float ki,
                                    float kd,
                                    uint8_t adc_target_enabled,
                                    uint32_t tick_ms,
                                    char *buffer,
                                    uint16_t buffer_size)
{
    char kp_text[16];
    char ki_text[16];
    char kd_text[16];
    int written;

    if ((status == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    Uart_Protocol_FormatFloat3(kp_text, sizeof(kp_text), kp);
    Uart_Protocol_FormatFloat3(ki_text, sizeof(ki_text), ki);
    Uart_Protocol_FormatFloat3(kd_text, sizeof(kd_text), kd);

    written = snprintf(
        buffer,
        buffer_size,
        "ms:%lu,enable:%d,State:%s,source:%s,target:%ld,actual:%ld,delta:%ld,PWM:%d,adc1:%u,adc2:%u,fault:%s,kp:%s,ki:%s,kd:%s\r\n",
        (unsigned long)tick_ms,
        status->enable,
        Uart_Protocol_StateName(status->state),
        (adc_target_enabled != 0U) ? "ADC" : "Uart",
        (long)status->target_speed,
        (long)status->actual_speed,
        (long)status->encoder_delta,
        status->PWM,
        (unsigned int)status->adc_raw,
        (unsigned int)status->adc_aux_raw,
        Uart_Protocol_FaultName(status->fault),
        kp_text,
        ki_text,
        kd_text);

    return Uart_Protocol_FinalizeLength(written, buffer_size);
}

uint16_t Uart_Protocol_FormatFaultSnapshot(uint8_t valid,
                                           const FaultSnapshot_t *snapshot,
                                           char *buffer,
                                           uint16_t buffer_size)
{
    int written;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    if ((valid == 0U) || (snapshot == NULL))
    {
        return Uart_Protocol_CopyText(
            "FAULT_SNAPSHOT_NONE\r\n", buffer, buffer_size);
    }

    written = snprintf(
        buffer,
        buffer_size,
        "valid:%d,time:%lu,state:%s,fault:%s,T_speed:%ld,A_speed:%ld,pwm=%d,adc_target=%u,adc_aux=%u\r\n",
        snapshot->valid,
        (unsigned long)snapshot->tick_ms,
        Uart_Protocol_StateName(snapshot->state),
        Uart_Protocol_FaultName(snapshot->fault),
        (long)snapshot->target_speed,
        (long)snapshot->actual_speed,
        snapshot->pwm,
        (unsigned int)snapshot->adc_target,
        (unsigned int)snapshot->adc_aux);

    return Uart_Protocol_FinalizeLength(written, buffer_size);
}

uint16_t Uart_Protocol_FormatHelp(char *buffer, uint16_t buffer_size)
{
    return Uart_Protocol_CopyText(
        "cmd:\r\n"
        "  run=1:start\r\n"
        "  stop\r\n"
        "  t=num:set target\r\n"
        "  set target adc\r\n"
        "  set target uart\r\n"
        "  kp=num:set kp 0.5\r\n"
        "  ki=num:set ki 0.1\r\n"
        "  kd=num:set kd 0\r\n"
        "  save params\r\n"
        "  load params\r\n"
        "  reset params\r\n"
        "  rst:reset\r\n"
        "  status\r\n"
        "  help | get fault | comm stats\r\n",
        buffer,
        buffer_size);
}


uint16_t Uart_Protocol_FormatStats(const Comm_StatsSnapshot_t *s,
                                   char *buffer,
                                   uint16_t buffer_size)
{
    int written;

    if ((s == NULL) || (buffer == NULL) || (buffer_size == 0U)) { return 0U; }
    written = snprintf(buffer, buffer_size,
        "COMM_STATS rx_overflow:%lu,line_q_full:%lu,cmd_q_full:%lu,pool_fail:%lu,pool_max:%lu,tx_drop:%lu,tx_error:%lu,rx_restart_fail:%lu,max_line:%lu,comm_q:%lu/%lu,cmd_q:%lu/%lu\r\n",
        (unsigned long)s->rx_overflow_count,
        (unsigned long)s->line_queue_full_count,
        (unsigned long)s->cmd_queue_full_count,
        (unsigned long)s->cmd_pool_alloc_fail_count,
        (unsigned long)s->cmd_pool_max_used,
        (unsigned long)s->tx_dropped_count,
        (unsigned long)s->tx_error_count,
        (unsigned long)s->uart_rx_restart_fail_count,
        (unsigned long)s->max_line_len,
        (unsigned long)s->comm_tx_queue_used,
        (unsigned long)s->comm_tx_queue_max,
        (unsigned long)s->cmd_queue_used,
        (unsigned long)s->cmd_queue_max);
    return Uart_Protocol_FinalizeLength(written, buffer_size);
}

uint16_t Uart_Protocol_FormatControlTimingStats(
    const Control_TimingStats_t *stats,
    char *buffer,
    uint16_t buffer_size)
{
    /*检查参数*/
    if ((stats == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return 0U;
    }

    /*格式化信息*/
    int written = snprintf(
        buffer,
        buffer_size,
        "TIMING_STATS period_us:%lu,exec_us:%lu,"
        "tick_exec_us:%lu,timeout_count:%u\r\n",
        (unsigned long)stats->period_us,
        (unsigned long)stats->execution_us,
        (unsigned long)stats->control_tick_execution_us,
        (unsigned int)stats->timeout_count);

    return Uart_Protocol_FinalizeLength(written, buffer_size);
}
