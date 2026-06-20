/*
 * Uart.c
 *
 * USART1 command interface.
 * RX path: ReceiveToIdle interrupt -> ring buffer -> Uart_Task line parser.
 */
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

static char s_uart_line_buf[APP_UART_LINE_SIZE];
static uint16_t s_uart_line_len = 0;
static uint8_t s_uart_line_discarding = 0;

static Motor_Status_t motor_status;

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

	s_uart_ring_head = 0;
	s_uart_ring_tail = 0;
	s_uart_ring_overflow = 0;
	s_uart_line_len = 0;
	s_uart_line_discarding = 0;

	Uart_StartReceiveToIdle();
}

//命令清除空格
static char* Uart_TrimCmd(char* text)
{
	char* end = NULL;
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	end = text + strlen(text);
	while (end > text && (*(end - 1) == ' ' || *(end - 1) == '\t')) {
		end--;
	}
	*end = '\0';
	return text;
}

//浮点数转字符
static void Uart_FormatFloat2(char* buf, uint8_t size, float num)
{
	uint32_t scaled = 0;
	uint8_t integer = 0;
	uint8_t fraction = 0;
	if (num >= 0) {
		scaled = (uint32_t)(num * 1000 + 0.5);
		integer = scaled / 1000;
		fraction = scaled % 1000;
		snprintf(buf, size, "%d.%03d", integer, fraction);
	}
	else {
		scaled = (uint32_t)((-num) * 1000 + 0.5);
		integer = scaled / 1000;
		fraction = scaled % 1000;
		snprintf(buf, size, "-%d.%03d", integer, fraction);
	}
}

//=================================================================================

//解析数字是否错误
static uint8_t Uart_ParseInt(char* text, int32_t* value)
{
	char* end = NULL;
	long result = strtol(text, &end, 10);
	if (end == text || *end != '\0') return 0;
	*value = (int32_t)result;
	return 1;
}

static uint8_t Uart_ParseFloat(char* text, float* value)
{
	char* end = NULL;
	float result = strtof(text, &end);
	if (end == text || *end != '\0') return 0;
	*value = (float)result;
	return 1;
}

void Uart_PrintfStatus(void)
{
	float kp = 0.0f;
	float ki = 0.0f;
	float kd = 0.0f;
	Control_GetStatusSnapshot(&motor_status);
	Control_GetPID(&kp, &ki, &kd);
	char kp_text[16], ki_text[16], kd_text[16];
	Uart_FormatFloat2(kp_text, sizeof(kp_text), kp);
	Uart_FormatFloat2(ki_text, sizeof(ki_text), ki);
	Uart_FormatFloat2(kd_text, sizeof(kd_text), kd);

	char Message[APP_UART_TX_SIZE] = {0};
	snprintf(Message, sizeof(Message),
	         "ms:%lu,enable:%d,State:%s,source:%s,target:%ld,actual:%ld,delta:%ld,PWM:%d,adc1:%u,adc2:%u,fault:%s,kp:%s,ki:%s,kd:%s\r\n",
	         HAL_GetTick(),
	         motor_status.enable,
	         Control_StateName(motor_status.state),
	         Control_IsAdcTargetEnabled() ? "ADC" : "Uart",
	         (long)motor_status.target_speed,
	         (long)motor_status.actual_speed,
	         (long)motor_status.encoder_delta,
	         motor_status.PWM,
	         motor_status.adc_raw,
	         motor_status.adc_aux_raw,
	         Control_FaultName(motor_status.fault),
	         kp_text,
	         ki_text,
	         kd_text);
	HAL_UART_Transmit(&huart1, (uint8_t*)Message, strlen(Message), HAL_MAX_DELAY);
}

static void Uart_TxText(const char* text)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), HAL_MAX_DELAY);
}

void Uart_PrintHelp(void)
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
			"  get fault:return shot\r\n"
	);
}

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

//PID参数检查
static uint8_t Uart_Check_PID_Gains(float Gains)
{
	return (Gains >= 0 && Gains <= 100) ? 1 : 0;
}

static void Uart_HandleCmd(char *line)
{
	char* cmd = Uart_TrimCmd(line);
	if (strcmp(cmd, "run=1") == 0) {
			Control_SetEnable(1);
			Control_GetStatusSnapshot(&motor_status);
			if (motor_status.state == SYS_RUN) {
			    Uart_TxText("OK:RUN\r\n");
			}
			else if (motor_status.fault != FAULT_NONE) {
				Uart_TxText("ERR:FAULT_LOCKED\r\n");
			}
			else {
				 Uart_TxText("ERR:RUN_FAILED\r\n");
			}
		}
		else if (strcmp(cmd, "stop") == 0 || strcmp(cmd, "run=0") == 0) {
				Control_SetEnable(0);
				Uart_TxText("OK:STOP\r\n");
		}
		else if (strncmp(cmd, "t=", 2) == 0) {
			int32_t temp_target;
			if (Uart_ParseInt(cmd + 2, &temp_target) == 0) {
				Uart_InvalidCmd();
			}
			else if (temp_target < (-APP_TARGET_SPEED_MAX) || temp_target > APP_TARGET_SPEED_MAX) {
				Uart_TxText("ERR:SPEED_OUT_OF_RANGE\r\n");
			}
			else {
				Control_SetTargetSpeed(temp_target);
				Uart_TxText("OK:TARGET\r\n");
			}
		}
		else if (strncmp(cmd, "kp=", 3) == 0) {
			float temp_kp;
			if (Uart_ParseFloat(cmd + 3, &temp_kp) == 0) {
				Uart_InvalidCmd();
			}
			else if (Uart_Check_PID_Gains(temp_kp) == 0) {
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");
			}
			else {
				Control_SetKp(temp_kp);
				Uart_TxText("OK:KP\r\n");
			}
		}
		else if (strncmp(cmd, "ki=", 3) == 0) {
			float temp_ki;
			if (Uart_ParseFloat(cmd + 3, &temp_ki) == 0) {
				Uart_InvalidCmd();
			}
			else if (Uart_Check_PID_Gains(temp_ki) == 0) {
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");
			}
			else {
				Control_SetKi(temp_ki);
				Uart_TxText("OK:KI\r\n");
			}
		}
		else if (strncmp(cmd, "kd=", 3) == 0) {
			float temp_kd;
			if (Uart_ParseFloat(cmd + 3, &temp_kd) == 0) {
				Uart_InvalidCmd();
			}
			else if (Uart_Check_PID_Gains(temp_kd) == 0) {
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");
			}
			else {
				Control_SetKd(temp_kd);
				Uart_TxText("OK:KD\r\n");
			}
		}
		else if (strcmp(cmd, "rst") == 0) {
			Control_PID_Rst();
			Control_ResetFault();
			Uart_TxText("OK:RESET\r\n");
		}
		else if (strcmp(cmd, "set target adc") == 0) {
				Control_UseAdcTarget(1);
				Uart_TxText("OK:TARGET_ADC\r\n");
		}
		else if (strcmp(cmd, "set target uart") == 0) {
				Control_UseAdcTarget(0);
				Uart_TxText("OK:TARGET_UART\r\n");
		}
		else if (strcmp(cmd, "status") == 0) {
			Uart_PrintfStatus();
		}
		else if (strcmp(cmd, "help") == 0) {
			Uart_PrintHelp();
		}
		else if (strcmp(cmd, "get fault") == 0) {
			Uart_HandleFaultSnapShot();
		}
		else {
			Uart_InvalidCmd();
		}
}

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
				Uart_HandleCmd(s_uart_line_buf);
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
