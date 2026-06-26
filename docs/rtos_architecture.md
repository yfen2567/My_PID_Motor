# FreeRTOS 阶段架构说明

当前工程已经从早期的裸机轮询/定时中断调度，整理为一个最小可展示的 FreeRTOS 任务化版本。系统仍然围绕直流电机闭环控制展开，但控制、命令接收和日志输出已经拆到不同任务中，避免所有逻辑堆在主循环或中断回调里。

## 任务划分

`ControlTask` 是周期控制任务，入口函数为 `StartControlTask()`，实际文件在 `App/Task/Src/Control_Task.c`。该任务以 10 ms 为周期运行，每个周期先从 `CmdQueue` 中非阻塞读取待处理控制命令，再调用 `Control_Tick10ms()` 执行控制逻辑。控制任务不等待串口输入，也不直接处理串口收发。

`CmdTask` 是串口命令任务，入口函数为 `StartCmdTask()`，实际文件在 `App/Task/Src/CmdTask.c`。它周期调用 `Uart_Task()`，把 USART1 ReceiveToIdle 中断放入 ring buffer 的数据组装成命令行，然后通过 `App_Cmd_Parse()` 解析命令。`status`、`help`、`get fault` 属于查询或显示类命令，可以由 `CmdTask` 直接触发输出；`run=1`、`stop`、`t=xxx`、`kp=x`、`ki=x`、`kd=x`、`rst` 等会改变控制状态的命令，会打包为 `App_Cmd_t`，通过命令服务放入 `CmdQueue`，交给 `ControlTask` 处理。

`LogTask` 是周期日志任务，入口函数为 `StartLogTask()`，实际文件在 `App/Task/Src/LogTask.c`。它每 3000 ms 调用 `LogTask_PrintPeriodicStatus()` 输出一次状态行。状态内容来自 `Control_GetStatusSnapshot()` 和 `Control_GetPID()`，包括系统时间、enable、状态、目标来源、目标速度、实际速度、PWM、ADC、fault 以及 PID 参数。

## 队列与互斥锁

`CmdQueue` 在 `Core/Src/freertos.c` 中创建，当前长度为 16，item size 为 `sizeof(App_Cmd_t*)`。命令队列采用“队列传结构体指针”的方式：`Cmd_Service_PostControlCommand()` 中申请内存并投递指针，`Cmd_Service_TryGetControlCommand()` 中取出指针、拷贝命令内容并释放内存。这样可以把队列和动态内存细节封装在 `Cmd_Service.c`，避免散落到 `CmdTask` 或 `ControlTask` 里。

`uartTxMutex` 同样在 `Core/Src/freertos.c` 中创建，并通过 `App/Inc/App_Rtos.h` 对外声明。它用于保护 USART1 发送。当前多个任务都可能调用串口发送：`CmdTask` 会输出 `OK:CMD_QUEUED`、错误提示、help、status 和 fault snapshot；`LogTask` 会周期输出状态日志。如果两个任务同时调用 HAL 串口发送，输出可能出现交叉、粘连，例如 status 行中插入 OK 或 fault 信息。

当前解决方式是：所有串口文本输出统一走 `Uart_TxText()`，该函数内部在 `uartTxMutexHandle` 有效时先 `osMutexAcquire()`，发送完成后再 `osMutexRelease()`。这样串口发送在文本级别串行化，保证一整条状态、OK 或 fault snapshot 输出不会被其他任务打断。
