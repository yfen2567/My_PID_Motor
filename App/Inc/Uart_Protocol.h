/*
 * Uart_Protocol.h
 *
 *  Created on: Jul 6, 2026
 *      Author: yuan
 */

#ifndef INC_UART_PROTOCOL_H_
#define INC_UART_PROTOCOL_H_

#include "App_Cmd.h"
#include "Control.h"
#include "Control_Task.h"

#include <stdint.h>

typedef enum
{
    UART_PROTOCOL_NOTICE_RX_OVERFLOW = 0,
    UART_PROTOCOL_NOTICE_CMD_LINE_QUEUE_FULL
} Uart_ProtocolNotice_t;

typedef struct
{
    uint32_t rx_overflow_count;
    uint32_t line_queue_full_count;
    uint32_t cmd_queue_full_count;
    uint32_t cmd_pool_alloc_fail_count;
    uint32_t cmd_pool_max_used;
    uint32_t tx_dropped_count;
    uint32_t tx_error_count;
    uint32_t uart_rx_restart_fail_count;
    uint32_t max_line_len;
    uint32_t comm_tx_queue_used;
    uint32_t comm_tx_queue_max;
    uint32_t cmd_queue_used;
    uint32_t cmd_queue_max;
} Comm_StatsSnapshot_t;

uint16_t Uart_Protocol_FormatNotice(Uart_ProtocolNotice_t notice,
                                    char *buffer,
                                    uint16_t buffer_size);


uint16_t Uart_Protocol_FormatParseError(App_Cmd_ParseResult_t result,
                                        char *buffer,
                                        uint16_t buffer_size);

uint16_t Uart_Protocol_FormatCommandResult(App_Cmd_Type_t command,
                                           App_Cmd_ExecResult_t result,
                                           char *buffer,
                                           uint16_t buffer_size);
uint16_t Uart_Protocol_FormatStatus(const Motor_Status_t *status,
                                    float kp,
                                    float ki,
                                    float kd,
                                    uint8_t adc_target_enabled,
                                    uint32_t tick_ms,
                                    char *buffer,
                                    uint16_t buffer_size);

uint16_t Uart_Protocol_FormatFaultSnapshot(uint8_t valid,
                                           const FaultSnapshot_t *snapshot,
                                           char *buffer,
                                           uint16_t buffer_size);

uint16_t Uart_Protocol_FormatHelp(char *buffer, uint16_t buffer_size);

uint16_t Uart_Protocol_FormatStats(const Comm_StatsSnapshot_t *s,
                                   char *buffer,
                                   uint16_t buffer_size);

uint16_t Uart_Protocol_FormatControlTimingStats(
    const Control_TimingStats_t *stats,
    char *buffer,
    uint16_t buffer_size);
#endif /* INC_UART_PROTOCOL_H_ */
