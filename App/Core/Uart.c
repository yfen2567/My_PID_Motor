/*串口（业务模块触发动作）
1.串口初始化
2.串口发送：结合外部接口函数获取信息，然后按照正常逻辑实现功能
3.串口接收：每次只接收一个字符然后再在接收缓存区内拼接，可以实现不定长指令接收，下次可以升级为HAL_UARTEx_ReceiveToIdle
4.命令解析控制：Uart_HandleCmd决定专门控制，Uart_Task决定什么时候控制*/
#include "Uart.h"
#include "app_config.h"
#include "Control.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static char s_rx_buf[APP_UART_LINE_SIZE]={0};
static char s_cmd_buf[APP_UART_LINE_SIZE]={0};
static uint8_t s_rx_buf_len=0;
static uint8_t s_rx_buf_byte=0;
static char temp_cmd[APP_UART_LINE_SIZE]={0};
static Motor_Status_t motor_status;
static volatile uint8_t s_cmd_ready=0;
static uint8_t s_rx_discarding=0;
static volatile uint8_t s_cmd_overrun = 0;
void Uart_Init(){
	HAL_UART_Receive_IT(&huart1, &s_rx_buf_byte,1);
}

//命令清除空格
static char* Uart_TrimCmd(char* text){
	char* end=NULL;
	while(*text==' '||*text=='\t'){
		text++;
	}
	end=text+strlen(text);
	while(end>text&&(*(end-1)==' '||*(end-1)=='\t')){
		end--;
	}
	*end='\0';
	return text;
}
//浮点数转字符
static void Uart_FormatFloat2(char* buf,uint8_t size,float num){
	uint32_t scaled=0;
	uint8_t integer=0;
	uint8_t fraction=0;
	if(num>=0){
		scaled=(uint32_t)(num*1000+0.5);
		integer=scaled/1000;
		fraction=scaled%1000;
		snprintf(buf,size,"%d.%03d",integer,fraction);
	}
	else{
		scaled=(uint32_t)((-num)*1000+0.5);
		integer=scaled/1000;
		fraction=scaled%1000;
		snprintf(buf,size,"-%d.%03d",integer,fraction);
	}
}

//=================================================================================


//解析数字是否错误
static uint8_t Uart_ParseInt(char* text,int32_t* value){
	char* end=NULL;
	long result=strtol(text, &end, 10);
	if(end==text||*end!='\0') return 0;
	*value=(int32_t)result;
	return 1;

}

static uint8_t Uart_ParseFloat(char* text,float* value){
	char* end=NULL;
	float result=strtof(text, &end);
	if(end==text||*end!='\0') return 0;
	*value=(float)result;
	return 1;
}


void Uart_PrintfStatus(){
	float kp=0.0f;
	float ki=0.0f;
	float kd=0.0f;
	Control_GetStatusSnapshot(&motor_status);
	Control_GetPID(&kp,&ki,&kd);
	char kp_text[16],ki_text[16],kd_text[16];
	Uart_FormatFloat2(kp_text,sizeof(kp_text),kp);
	Uart_FormatFloat2(ki_text,sizeof(ki_text),ki);
	Uart_FormatFloat2(kd_text,sizeof(kd_text),kd);

	char Message[APP_UART_TX_SIZE]={0};
	snprintf(Message,sizeof(Message),"ms:%lu,enable:%d,State:%s,source:%s,target:%ld,actual:%ld,delta:%ld,PWM:%d,adc:%u,fault:%s,kp:%s,ki:%s,kd:%s\r\n",HAL_GetTick(),motor_status.enable,Control_StateName(motor_status.state),Control_IsAdcTargetEnabled()?"ADC":"Uart",(long)motor_status.target_speed,(long)motor_status.actual_speed,motor_status.encoder_delta,motor_status.PWM,motor_status.adc_raw,Control_FaultName(motor_status.fault),kp_text,ki_text,kd_text);
	HAL_UART_Transmit(&huart1, (uint8_t*)Message, strlen(Message), HAL_MAX_DELAY);
}

static void Uart_TxText(const char* text){
	HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), HAL_MAX_DELAY);
}
void Uart_PrintHelp(){
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
	);
}

//指令无效
static void Uart_InvalidCmd(){
	Uart_TxText("ERR:INVALID_CMD\r\n");
}

//PID参数检查
static uint8_t Uart_Check_PID_Gains(float Gains){
	return (Gains>=0&&Gains<=100)?1:0;
}

static void Uart_HandleCmd(){
	char* cmd=Uart_TrimCmd(temp_cmd);
	if(strcmp(cmd,"run=1")==0){
			Control_SetEnable(1);
			Control_GetStatusSnapshot(&motor_status);
			if (motor_status.state == SYS_RUN){
			    Uart_TxText("OK:RUN\r\n");
			}
			else if(motor_status.fault!=FAULT_NONE){
				Uart_TxText("ERR:FAULT_LOCKED\r\n");
			}
			else{
				 Uart_TxText("ERR:RUN_FAILED\r\n");
			}
		}
		else if(strcmp(cmd,"stop")==0||strcmp(cmd,"run=0")==0){
				Control_SetEnable(0);
				Uart_TxText("OK:STOP\r\n");
		}
		else if(strncmp(cmd,"t=",2)==0){
			int32_t temp_target;
			if(Uart_ParseInt(cmd+2,&temp_target)==0){
				Uart_InvalidCmd();
			}
			else if(temp_target<(-APP_TARGET_SPEED_MAX)||temp_target>APP_TARGET_SPEED_MAX){
				Uart_TxText("ERR:SPEED_OUT_OF_RANGE\r\n");
			}
			else{
				Control_SetTargetSpeed(temp_target);
				Uart_TxText("OK:TARGET\r\n");
			}
		}
		else if(strncmp(cmd,"kp=",3)==0){
			float temp_kp;
			if(Uart_ParseFloat(cmd+3,&temp_kp)==0){
				Uart_InvalidCmd();
			}
			else if(Uart_Check_PID_Gains(temp_kp)==0){
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");

			}

			else{
				Control_SetKp(temp_kp);
				Uart_TxText("OK:KP\r\n");
			}
		}
		else if(strncmp(cmd,"ki=",3)==0){
			float temp_ki;
			if(Uart_ParseFloat(cmd+3,&temp_ki)==0){
				Uart_InvalidCmd();
			}
			else if(Uart_Check_PID_Gains(temp_ki)==0){
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");

			}
			else{
				Control_SetKi(temp_ki);
				Uart_TxText("OK:KI\r\n");
			}
		}
		else if(strncmp(cmd,"kd=",3)==0){
			float temp_kd;
			if(Uart_ParseFloat(cmd+3,&temp_kd)==0){
				Uart_InvalidCmd();
			}
			else if(Uart_Check_PID_Gains(temp_kd)==0){
				Uart_TxText("ERR:GAIN_OUT_OF_RANGE\r\n");

			}
			else{
				Control_SetKd(temp_kd);
				Uart_TxText("OK:KD\r\n");
			}
		}
		else if(strcmp(cmd,"rst")==0){
			Control_PID_Rst();
			Control_ResetFault();
			Uart_TxText("OK:RESET\r\n");
		}
		else if(strcmp(cmd,"set target adc")==0){
				Control_UseAdcTarget(1);
				Uart_TxText("OK:TARGET_ADC\r\n");
		}
		else if(strcmp(cmd,"set target uart")==0){
				Control_UseAdcTarget(0);
				Uart_TxText("OK:TARGET_UART\r\n");
		}
		else if(strcmp(cmd,"status")==0){
			Uart_PrintfStatus();
		}
		else if(strcmp(cmd,"help")==0){
			Uart_PrintHelp();
		}
		else{
			Uart_InvalidCmd();
		}

}


void Uart_Task(void){
	uint8_t over_run=0;
		__disable_irq();
		if(s_cmd_overrun==1){
			s_cmd_overrun=0;
			over_run=1;
		}
		__enable_irq();
		if(over_run==1){
			Uart_TxText("ERR:CMD_BUSY\r\n");
		}
		if(s_cmd_ready==1){
		__disable_irq();
		memcpy(temp_cmd,s_cmd_buf,sizeof(s_cmd_buf));
		s_cmd_ready=0;
		__enable_irq();
		Uart_HandleCmd();
	}
}


//======================================================================================


//回调

void My_UART_RxCpltCallback(){
	char ch=(char)s_rx_buf_byte;
	if(ch=='\r'||ch=='\n'){
		if(s_rx_discarding==0&&s_rx_buf_len>0){
			if(s_cmd_ready ==0){
				s_rx_buf[s_rx_buf_len]='\0';
				memcpy(s_cmd_buf,s_rx_buf,s_rx_buf_len+1);
				s_cmd_ready=1;
			}
			else{
				s_cmd_overrun=1;
			}
		}
			s_rx_buf_len=0;
			s_rx_discarding = 0;

	}
	else{
		if(s_rx_discarding!=0){

		}
		else{
			if(s_rx_buf_len<sizeof(s_rx_buf)-1){
				s_rx_buf[s_rx_buf_len]=ch;
				s_rx_buf_len++;

			}
			else{
				s_rx_buf_len=0;
				s_rx_discarding=1;

			}
		}
	}
	HAL_UART_Receive_IT(&huart1, &s_rx_buf_byte,1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance==USART1){
		My_UART_RxCpltCallback();
	}
}

