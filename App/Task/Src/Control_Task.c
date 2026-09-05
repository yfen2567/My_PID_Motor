#include "Cmd_Service.h"
#include "Control_Task.h"
#include "Control.h"
#include "App_Cmd.h"
#include "cmsis_os.h"
#include "main.h"

#include "Nv_Service.h"
#include "Comm_Service.h"
#include "App_Cmd.h"
#include "CmdPool.h"
#include "Control_Task.h"


uint32_t previous_updata_start=0;
uint32_t period_us=0;
uint32_t execution_us=0;
uint32_t control_tick_execution_us =0;
uint16_t timeout_count=0;
Control_TimingStats_t control_timingsnapshot;//统计结构体复用为快照结构体类型

static App_Cmd_ExecResult_t ControlTask_PostNvRequest(const App_Cmd_t *cmd)
{
    Motor_Status_t status;
    NvRequest_t request = {0};

    Control_GetStatusSnapshot(&status);
    if ((status.enable != 0U) || (status.state != SYS_IDLE))
    {
        return APP_CMD_EXEC_NOT_IDLE;
    }
    if (Nv_Service_IsBusy()) { return APP_CMD_EXEC_BUSY; }

    request.operation = cmd->type;
    if (cmd->type == APP_CMD_SAVE_PARAMS)
    {
        Control_GetPID(&request.params.kp, &request.params.ki, &request.params.kd);
    }
    return Nv_Service_Post(&request) ? APP_CMD_EXEC_OK : APP_CMD_EXEC_QUEUE_FULL;
}

static void DebugGpio_CycleHigh()
{
	HAL_GPIO_WritePin(GPIO_CYCLE_GPIO_Port,GPIO_CYCLE_Pin,GPIO_PIN_SET);
}

static void DebugGpio_CycleLow()
{
	HAL_GPIO_WritePin(GPIO_CYCLE_GPIO_Port,GPIO_CYCLE_Pin,GPIO_PIN_RESET);
}

static void DebugGpio_TickHigh(void)
{
    HAL_GPIO_WritePin(GPIO_TICK_GPIO_Port, GPIO_TICK_Pin, GPIO_PIN_SET);
}

static void DebugGpio_TickLow(void)
{
    HAL_GPIO_WritePin(GPIO_TICK_GPIO_Port, GPIO_TICK_Pin, GPIO_PIN_RESET);
}

static void Control_Timing_TrySaveSnapshot(void)
{
	taskENTER_CRITICAL();
	control_timingsnapshot.control_tick_execution_us=control_tick_execution_us;
	control_timingsnapshot.execution_us=execution_us;
	control_timingsnapshot.period_us=period_us;
	control_timingsnapshot.timeout_count=timeout_count;
	taskEXIT_CRITICAL();
}

void Control_Timing_GetStats(Control_TimingStats_t *out)
{
	taskENTER_CRITICAL();
	*out=control_timingsnapshot;
	taskEXIT_CRITICAL();
}



void StartControlTask(void *argument)
{
    App_Cmd_t *cmd;
    App_Cmd_ExecResult_t result;
    App_Cmd_Type_t reply_command;
    TickType_t last_time = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10);
    uint32_t cycles_per_us=SystemCoreClock/1000000;

    (void)argument;
    for (;;)
    {
    	/*拉高GPIO_CYCLE*/
    	DebugGpio_CycleHigh();

    	/*记录开始时间，并计算任务周期*/
    	uint32_t updata_start = DWT->CYCCNT;
    	uint32_t period_cycles=updata_start-previous_updata_start;
    	period_us=period_cycles/cycles_per_us;

    	previous_updata_start=updata_start;

    	/*记录超期次数*/
    	if(period_us>10000U){
    		timeout_count++;
    	}

    	/*保存控制时序快照*/
    	Control_Timing_TrySaveSnapshot();//当传递的信息是多个不同时机才能更新的消息时，在合适的时机使用快照对信息进行保存可以保证多个数据所处上下文的一致性。再结合临界区就很不错了


    	/*接收并处理命令*/
        while (Cmd_Service_TryGetControlCommand(&cmd))
        {
            if (cmd->type == APP_CMD_APPLY_STORED_PARAMS)
            {
                reply_command = cmd->origin_type;
                result = Control_ApplyCommand(cmd);
                Nv_Service_Complete();
                (void)Comm_Service_PostCommandResult(reply_command, result);
            }
            else if (App_Cmd_IsParamStoreCommand(cmd->type) != 0U)
            {
                result = ControlTask_PostNvRequest(cmd);
                if (result != APP_CMD_EXEC_OK)
                {
                    (void)Comm_Service_PostCommandResult(cmd->type, result);
                }
                /* Success is deferred until NvTask has actually finished. */
            }
            else if (Nv_Service_IsBusy() && (cmd->type != APP_CMD_STOP))
            {
                (void)Comm_Service_PostCommandResult(cmd->type, APP_CMD_EXEC_BUSY);
            }
            else
            {
                result = Control_ApplyCommand(cmd);
                (void)Comm_Service_PostCommandResult(cmd->type, result);
            }
            CmdPool_Free(cmd);
        }

/*如果出现错误vTaskDelayUntil是不阻塞连续快速执行以追赶  ，也就是说last_time =last_time +period。而
 * osDelayUntil则是自己手动写代码，让next_wake 等于当前时间，然后不阻塞直接启动，二者区别就是，前者会
 * 使劲弥补落后的那些周期。后者则是落后了，那干脆不弥补了，从现在重新开始计时间。*/

        /*执行周期控制，并记录控制时间*/
    	/*拉高GPIO_TICK后再拉低*/
        uint32_t execution_start=DWT->CYCCNT;
        DebugGpio_TickHigh();
        Control_Tick10ms();
        DebugGpio_TickLow();
        uint32_t execution_end=DWT->CYCCNT;
        uint32_t control_tick_execution_cycles=execution_end-execution_start;
        control_tick_execution_us=control_tick_execution_cycles/cycles_per_us;

        uint32_t updata_end = DWT->CYCCNT;
        uint32_t execution_cycles=updata_end-updata_start;
        execution_us=execution_cycles/cycles_per_us;

        /*拉低GPIO_CYCLE*/
        DebugGpio_CycleLow();
        vTaskDelayUntil(&last_time, period);
    }
}
