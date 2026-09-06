# v2.2 FreeRTOS 架构说明

当前工程使用 CMSIS-RTOS V2 接口和 FreeRTOS。控制、命令、Flash 存储、日志和串口发送由不同任务承担，控制任务不阻塞在串口输入、Flash 擦写或 UART 发送上。

## 任务划分

| 任务 | 优先级 | 周期或阻塞方式 | 职责 |
| --- | --- | --- | --- |
| `ControlTask` | AboveNormal | 10 ms `vTaskDelayUntil` | 消费控制命令、检查参数存储前置条件、更新控制状态、执行 PID/测速/PWM |
| `CmdTask` | Normal | 约 5 ms 轮询 | 消费 UART 行队列、解析命令、投递查询或控制请求 |
| `CommTxTask` | Normal | 阻塞等待 `CommTxQueue` | 格式化所有文本消息并发起 UART 中断发送 |
| `NvTask` | BelowNormal | 阻塞等待 `NvRequestQueue` | 执行 Flash 保存、加载和恢复默认值 |
| `LogTask` | Low | 1000 ms 周期 | 获取控制快照并投递周期 status |

## 数据流

```text
USART1 ReceiveToIdle ISR
    -> UART ring buffer
    -> CmdTask 行组装与解析
       -> 查询命令 -> CommTxQueue
       -> 控制命令 -> CmdPool -> ControlCmdQueue -> ControlTask
       -> 参数存储命令 -> ControlTask IDLE 检查 -> NvRequestQueue -> NvTask
                                                        -> ControlCmdQueue 内部应用命令

ControlTask / CmdTask / NvTask / LogTask
    -> CommTxQueue
    -> CommTxTask
    -> Uart_WriteAsync
    -> HAL_UART_TxCpltCallback
```

## RTOS 对象

| 对象 | 长度 | 内容 | 作用 |
| --- | ---: | --- | --- |
| `ControlCmdQueue` | 16 | `App_Cmd_t *` | CmdTask 和 NvTask 向 ControlTask 交付命令对象指针 |
| `CommTxQueue` | 8 | `Comm_Message_t` | 所有任务向 CommTxTask 交付待发送文本消息 |
| `NvRequestQueue` | 2 | `NvRequest_t` | ControlTask 向 NvTask 交付 Flash 参数操作 |
| `s_tx_done_Sem` | 最大 1 | 二进制发送许可 | 保护单个在飞 UART TX；发送完成回调归还许可 |
| `CmdPool` | 16 块 | 静态 `App_Cmd_t` | 避免把 CmdTask 局部变量地址放入异步队列 |

## 参数存储边界

Flash 擦写只在 NvTask 中执行。ControlTask 只在 `enable=0` 且 `State=IDLE` 时向 `NvRequestQueue` 投递参数请求；这避免控制 10 ms 路径直接执行 Flash 操作。`load params` 和 `reset params` 成功后，NvTask 通过内部命令把参数交回 ControlTask 应用，保证 PID 状态的修改仍由控制模块拥有。

## 串口发送边界

所有响应、status、fault 和统计信息先复制为 `Comm_Message_t` 并入 `CommTxQueue`。CommTxTask 是 UART 文本发送的唯一任务所有者；`Uart_WriteAsync` 获取发送许可、启动 `HAL_UART_Transmit_IT`，TX 完成回调释放许可。这样不同任务不会直接竞争同一个 HAL UART 发送调用。

## 已知边界

- ControlTask 使用绝对 10 ms 调度；当前 v2.2 未提供周期、抖动或执行时间统计，这些属于 v2.3.1 的工作。
- NvTask 仅允许在 IDLE 处理参数存储，不验证运行中 Flash 操作。
- 当前发送许可等待超时/错误恢复、长时间通信压力和所有队列极限不属于 v2.2 实机回归范围。
