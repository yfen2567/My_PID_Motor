/*
 * Uart.c
 *
 * USART1 command interface.
 * RX path: ReceiveToIdle interrupt -> ring buffer -> Uart_Task line parser.
 */
#include <App_Cmd.h>
#include "Uart.h"
#include "app_config.h"
#include "Control.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static uint8_t s_uart_rx_idle_buf[APP_UART_RX_IDLE_CHUNK_SIZE];
static uint8_t s_uart_ring_buf[APP_UART_RX_RING_SIZE];
static volatile uint16_t s_uart_ring_head = 0;
static volatile uint16_t s_uart_ring_tail = 0;
static volatile uint8_t s_uart_ring_overflow = 0;
static volatile uint8_t s_cmd_ready;
static char s_uart_line_buf[APP_UART_LINE_SIZE];
static uint16_t s_uart_line_len = 0;
static uint8_t s_uart_line_discarding = 0;
static char s_cmd_buf[APP_UART_LINE_SIZE];

static void Uart_StartReceiveToIdle(void)
{
	if (HAL_UARTEx_ReceiveToIdle_IT(&huart1,
	                                s_uart_rx_idle_buf,
	                                APP_UART_RX_IDLE_CHUNK_SIZE) != HAL_OK) {
		Error_Handler();
	}
}

static uint16_t Uart_RingNext(uint16_t index)
{
	index++;
	if (index >= APP_UART_RX_RING_SIZE) {
		index = 0;
	}
	return index;
}

static void Uart_RingPushFromIsr(uint8_t data)
{
	uint16_t next = Uart_RingNext(s_uart_ring_head);

	if (next == s_uart_ring_tail) {
		s_uart_ring_overflow = 1;
		return;
	}

	s_uart_ring_buf[s_uart_ring_head] = data;
	s_uart_ring_head = next;
}

static uint8_t Uart_RingPop(uint8_t *data)
{
	uint8_t has_data = 0;

	__disable_irq();
	if (s_uart_ring_tail != s_uart_ring_head) {
		*data = s_uart_ring_buf[s_uart_ring_tail];
		s_uart_ring_tail = Uart_RingNext(s_uart_ring_tail);
		has_data = 1;
	}
	__enable_irq();

	return has_data;
}

static uint8_t Uart_TakeRingOverflow(void)
{
	uint8_t overflow = 0;

	__disable_irq();
	if (s_uart_ring_overflow != 0) {
		s_uart_ring_overflow = 0;
		overflow = 1;
	}
	__enable_irq();

	return overflow;
}

void Uart_Init(void)
{
	memset(s_uart_rx_idle_buf, 0, sizeof(s_uart_rx_idle_buf));
	memset(s_uart_ring_buf, 0, sizeof(s_uart_ring_buf));
	memset(s_uart_line_buf, 0, sizeof(s_uart_line_buf));
	memset(s_cmd_buf, 0, sizeof(s_cmd_buf));

	s_uart_ring_head = 0;
	s_uart_ring_tail = 0;
	s_uart_ring_overflow = 0;
	s_uart_line_len = 0;
	s_uart_line_discarding = 0;
	s_cmd_ready=0;

	Uart_StartReceiveToIdle();
}


uint8_t Uart_ReadLine(char *line, uint16_t size){
	if(line==NULL||size==0){
		return 0;
	}
	__disable_irq();
	if(s_cmd_ready==0){
		__enable_irq();
		return 0;
	}

	strncpy(line,s_cmd_buf,size-1);
	line[size-1]='\0';
	 s_cmd_ready = 0U;
	__enable_irq();
	return 1;
}



void Uart_TxText(const char* text)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), HAL_MAX_DELAY);
}

/*


//命令无效
static void Uart_InvalidCmd(void)
{
	Uart_TxText("ERR:INVALID_CMD\r\n");
}

//处理获取错误状态快照命令
void Uart_HandleFaultSnapShot(void)
{
	if (Control_HasFaultShot() == 0) {
		Uart_TxText("FAULT_SNAPSHOT_NONE\r\n");
		return;
	}
	FaultSnapshot_t shot = Control_GetFaultShot();
	char Message[APP_UART_TX_SIZE] = {0};
	snprintf(Message, sizeof(Message),
	         "valid:%d,time:%lu,state:%s,fault:%s,T_speed:%ld,A_speed:%ld,pwm=%d,adc_target=%u,adc_aux=%u\r\n",
	         shot.valid,
	         shot.tick_ms,
	         Control_StateName(shot.state),
	         Control_FaultName(shot.fault),
	         (long)shot.target_speed,
	         (long)shot.actual_speed,
	         shot.pwm,
	         shot.adc_target,
	         shot.adc_aux);
	HAL_UART_Transmit(&huart1, (uint8_t*)Message, strlen(Message), HAL_MAX_DELAY);
}
*/





void Uart_Task(void)
{
	uint8_t ch = 0;

	if (Uart_TakeRingOverflow() != 0) {
		s_uart_line_len = 0;
		s_uart_line_discarding = 1;
		Uart_TxText("ERR:RX_OVERFLOW\r\n");
	}

	while (Uart_RingPop(&ch) != 0) {
		if (ch == '\r' || ch == '\n') {
			if (s_uart_line_discarding != 0) {
				s_uart_line_discarding = 0;
				s_uart_line_len = 0;
				Uart_TxText("ERR:CMD_TOO_LONG\r\n");
			}
			else if (s_uart_line_len > 0) {
				s_uart_line_buf[s_uart_line_len] = '\0';
				memcpy(s_cmd_buf, s_uart_line_buf, s_uart_line_len + 1U);
				s_cmd_ready = 1U;
				s_uart_line_len = 0;
			}
			else {
				/* Ignore empty lines, including LF after CRLF. */
			}
		}
		else {
			if (s_uart_line_discarding != 0) {
				continue;
			}
			if (s_uart_line_len < (APP_UART_LINE_SIZE - 1U)) {
				s_uart_line_buf[s_uart_line_len] = (char)ch;
				s_uart_line_len++;
			}
			else {
				s_uart_line_len = 0;
				s_uart_line_discarding = 1;
			}
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART1) {
		for (uint16_t i = 0; i < Size && i < APP_UART_RX_IDLE_CHUNK_SIZE; i++) {
			Uart_RingPushFromIsr(s_uart_rx_idle_buf[i]);
		}
		Uart_StartReceiveToIdle();
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1) {
		Uart_StartReceiveToIdle();
	}
}
