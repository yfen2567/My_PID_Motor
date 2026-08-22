/*
 * Uart.c
 *
 * USART1 command interface.
 * RX path: ReceiveToIdle interrupt -> ring buffer -> Uart_ProcessRx line parser.
 */
#include <App_Cmd.h>
#include "Uart.h"
#include "app_config.h"
#include "Control.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "App_Rtos.h"
#include "cmsis_os.h"

static uint8_t s_uart_rx_idle_buf[APP_UART_RX_IDLE_CHUNK_SIZE];
static uint8_t s_uart_ring_buf[APP_UART_RX_RING_SIZE];
static volatile uint16_t s_uart_ring_head;
static volatile uint16_t s_uart_ring_tail;
static volatile uint8_t s_uart_ring_overflow;
static char s_uart_line_buf[APP_UART_LINE_SIZE];
static uint16_t s_uart_line_len;
static uint8_t s_uart_line_discarding;
static char s_line_queue[APP_UART_LINE_QUEUE_DEPTH][APP_UART_LINE_SIZE];
static uint8_t s_line_queue_head;
static uint8_t s_line_queue_tail;
static uint8_t s_line_queue_count;
static volatile uint8_t s_rx_restart_pending;
static volatile uint16_t s_rx_restart_fail_count;
static uint8_t s_uart_discard_too_long;
static volatile uint32_t s_max_line_len;
static volatile uint8_t s_tx_active;
static volatile uint32_t s_rx_overflow_count;
static volatile uint32_t s_line_queue_full_count;


static uint8_t Uart_LineQueueNext(uint8_t index)
{
	index++;
	if (index >= APP_UART_LINE_QUEUE_DEPTH) {
		index = 0;
	}
	return index;
}

static uint8_t Uart_LineQueuePush(const char *line)
{
    if (s_line_queue_count >= APP_UART_LINE_QUEUE_DEPTH)
    {
        s_line_queue_full_count++;
        return 0U;
    }
    strncpy(s_line_queue[s_line_queue_head], line, APP_UART_LINE_SIZE - 1U);
    s_line_queue[s_line_queue_head][APP_UART_LINE_SIZE - 1U] = '\0';
    s_line_queue_head = Uart_LineQueueNext(s_line_queue_head);
    s_line_queue_count++;
    return 1U;
}

static uint8_t Uart_StartReceiveToIdle(void)
{
	if(huart1.RxState!=HAL_UART_STATE_READY)
	{
		s_rx_restart_pending=0U;
		return 1U;
	}

	if (HAL_UARTEx_ReceiveToIdle_IT(&huart1,
	                                s_uart_rx_idle_buf,
	                                APP_UART_RX_IDLE_CHUNK_SIZE) == HAL_OK) {
		s_rx_restart_pending=0U;
		return 1U;
	}

	s_rx_restart_fail_count++;
	s_rx_restart_pending=1U;
	return 0U;
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

    if (next == s_uart_ring_tail)
    {
        if (s_uart_ring_overflow == 0U) { s_rx_overflow_count++; }
        s_uart_ring_overflow = 1U;
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
	memset(s_line_queue, 0, sizeof(s_line_queue));

	s_uart_ring_head = 0;
	s_uart_ring_tail = 0;
	s_uart_ring_overflow = 0;
	s_uart_line_len = 0;
	s_uart_line_discarding = 0;
	s_line_queue_head = 0;
	s_line_queue_tail = 0;
	s_line_queue_count = 0;
	s_rx_restart_pending=0;
	s_rx_restart_fail_count=0;
	s_uart_discard_too_long=0;
	s_max_line_len = 0U;
	s_tx_active=0U;
	s_rx_overflow_count = 0U;
	s_line_queue_full_count = 0U;
	Uart_StartReceiveToIdle();
}


uint8_t Uart_ReadLine(char *line, uint16_t size){
	if(line==NULL||size==0){
		return 0;
	}

	if(s_line_queue_count==0){
		return 0;
	}

	strncpy(line,s_line_queue[s_line_queue_tail],size-1);
	line[size-1]='\0';
	s_line_queue_tail = Uart_LineQueueNext(s_line_queue_tail);
	s_line_queue_count--;
	return 1;
}
uint32_t Uart_ProcessRx(void)
{
	uint8_t ch = 0;
	uint32_t events = 0U;


	if (s_rx_restart_pending != 0U) { (void)Uart_StartReceiveToIdle(); }

	if (Uart_TakeRingOverflow() != 0) {
		s_uart_line_len = 0;
		s_uart_line_discarding = 1;
		s_uart_discard_too_long=0;
		events|=UART_EVENT_RX_OVERFLOW;
	}

	while (Uart_RingPop(&ch) != 0) {
        if ((ch == '\r') || (ch == '\n'))
        {
            if (s_uart_line_discarding != 0U)
            {
                if (s_uart_discard_too_long != 0U)
                {
                    events |= UART_EVENT_CMD_TOO_LONG;
                }
                s_uart_line_discarding = 0U;
                s_uart_line_len = 0U;
                s_uart_discard_too_long = 0U;
            }
			else if (s_uart_line_len > 0) {
				s_uart_line_buf[s_uart_line_len] = '\0';
				if (s_uart_line_len > s_max_line_len) { s_max_line_len = s_uart_line_len; }
				if (Uart_LineQueuePush(s_uart_line_buf) == 0U) {
					events|=UART_EVENT_LINE_QUEUE_FULL;
				}
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
				s_uart_discard_too_long = 1U;
			}
		}
	}
	return events;
}


bool Uart_WriteAsync(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U) || (s_tx_done_SemHandle == NULL) ||
        (s_tx_active != 0U))
    {
        return false;
    }

    while (osSemaphoreAcquire(s_tx_done_SemHandle, 0U) == osOK) { }
    s_tx_active = 1U;
    if (HAL_UART_Transmit_IT(&huart1, (uint8_t *)data, length) != HAL_OK)
    {
        s_tx_active = 0U;
        return false;
    }
    return true;
}

bool Uart_WaitTxComplete(uint32_t timeout_ms)
{
    uint32_t timeout_ticks;

    timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if ((s_tx_done_SemHandle == NULL) ||
        (osSemaphoreAcquire(s_tx_done_SemHandle, timeout_ticks) != osOK))
    {
        (void)HAL_UART_AbortTransmit(&huart1);
        s_tx_active = 0U;
        return false;
    }
    return true;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART1)
	{
		for (uint16_t i = 0; i < Size && i < APP_UART_RX_IDLE_CHUNK_SIZE; i++)
		{
			Uart_RingPushFromIsr(s_uart_rx_idle_buf[i]);
		}
		Uart_StartReceiveToIdle();
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        s_tx_active = 0U;
        if (s_tx_done_SemHandle != NULL) { (void)osSemaphoreRelease(s_tx_done_SemHandle); }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		s_rx_restart_pending=1;
	}
}

void Uart_GetStats(Uart_Stats_t *stats)
{
    if (stats == NULL) { return; }
    __disable_irq();
    stats->rx_overflow_count = s_rx_overflow_count;
    stats->line_queue_full_count = s_line_queue_full_count;
    stats->rx_restart_fail_count = s_rx_restart_fail_count;
    stats->max_line_len = s_max_line_len;
    __enable_irq();
}
