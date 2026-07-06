/*
 * Uart.h
 *
 *  Created on: May 27, 2026
 *      Author: yuan
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#define UART_EVENT_RX_OVERFLOW          (1UL << 0U)
#define UART_EVENT_CMD_TOO_LONG         (1UL << 1U)
#define UART_EVENT_LINE_QUEUE_FULL      (1UL << 2U)

void Uart_Init();
uint32_t Uart_Task(void);
void Uart_PrintfStatus();
void Uart_PrintHelp();
void Uart_TxText(const char* text);
uint8_t Uart_ReadLine(char *line, uint16_t size);
#endif /* INC_UART_H_ */
