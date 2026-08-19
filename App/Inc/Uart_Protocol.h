/*
 * Uart_Protocol.h
 *
 *  Created on: Jul 6, 2026
 *      Author: yuan
 */

#ifndef INC_UART_PROTOCOL_H_
#define INC_UART_PROTOCOL_H_

#include <stdint.h>

typedef enum
{
    UART_PROTOCOL_NOTICE_RX_OVERFLOW = 0,
    UART_PROTOCOL_NOTICE_CMD_LINE_QUEUE_FULL
} Uart_ProtocolNotice_t;

typedef struct
{
    uint32_t rx_overflow_count;
    uint32_t cmd_queue_full_count;
    uint32_t control_cmd_queue_full_count;
    uint32_t cmd_pool_alloc_fail_count;
    uint32_t cmd_pool_max_used;
    uint32_t tx_dropped_count;
    uint32_t tx_error_count;
    uint32_t uart_rx_restart_fail_count;
    uint32_t comm_tx_queue_used;
    uint32_t comm_tx_queue_max;
    uint32_t cmd_queue_used;
    uint32_t cmd_queue_max;
} Cmd_StatsSnapshot_t;

#endif /* INC_UART_PROTOCOL_H_ */
