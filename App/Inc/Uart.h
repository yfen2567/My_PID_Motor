/*
 * Uart.h
 *
 *  Created on: May 27, 2026
 *      Author: yuan
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include <stdbool.h>
#include <stdint.h>

#define UART_EVENT_RX_OVERFLOW          (1UL << 0U)
#define UART_EVENT_CMD_TOO_LONG         (1UL << 1U)
#define UART_EVENT_LINE_QUEUE_FULL      (1UL << 2U)

typedef struct
{
    uint32_t rx_overflow_count;
    uint32_t line_queue_full_count;
    uint32_t rx_restart_fail_count;
    uint32_t max_line_len;
} Uart_Stats_t;

void Uart_Init();
uint32_t Uart_ProcessRx(void);
uint8_t Uart_ReadLine(char *line, uint16_t size);
bool Uart_WriteAsync(const uint8_t *data, uint16_t length);
bool Uart_WaitTxComplete(uint32_t timeout_ms);
void Uart_GetStats(Uart_Stats_t *stats);
#endif /* INC_UART_H_ */
