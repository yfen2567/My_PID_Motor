/*
 * Uart.h
 *
 *  Created on: May 27, 2026
 *      Author: yuan
 */

#ifndef INC_UART_H_
#define INC_UART_H_
void Uart_Init();
void Uart_Task(void);
void Uart_PrintfStatus();
void Uart_PrintHelp();
void Uart_TxText(const char* text);
uint8_t Uart_ReadLine(char *line, uint16_t size);
#endif /* INC_UART_H_ */
