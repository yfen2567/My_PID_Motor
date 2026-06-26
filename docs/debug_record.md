# 阶段调试记录：RTOS 串口输出互斥

## 1. UART TX mutex 问题处理记录

本阶段把原来的串口命令处理和周期日志输出整理到 FreeRTOS 任务结构中。现在 `CmdTask` 负责串口命令接收、解析和入队，`ControlTask` 周期执行控制逻辑，`LogTask` 周期输出状态日志。结构变清楚后，也暴露出一个共享资源问题：不止一个任务会使用 USART1 TX。

调试过程中曾观察到串口输出交叉、粘连。典型场景是 `CmdTask` 正在返回 `OK` 或错误信息，`LogTask` 又到 3 秒周期准备输出 status 行；或者执行 `get fault` 时，fault snapshot 与周期日志接近同时输出。由于 UART TX 是共享资源，如果多个任务直接调用底层发送，就可能出现一条文本还没发完，另一条文本插进来。

检查代码后，确认 `CmdTask` 和 `LogTask` 都会触发串口输出：

- `CmdTask` 输出 `OK:CMD_QUEUED`、`ERR:BAD_CMD`、help 文本、status 和 `get fault` 结果；
- `LogTask` 每 3000 ms 输出一次周期状态；
- fault snapshot 输出也需要纳入同一条串口发送路径。

因此问题根因不是某条命令解析错误，而是 USART1 TX 在 RTOS 下成为多个任务共享的临界资源。

当前处理方式是在 FreeRTOS 对象中加入 `uartTxMutex`，并在 `App/Inc/App_Rtos.h` 中声明 `uartTxMutexHandle`。所有串口文本输出统一走 `Uart_TxText()`，该函数在 mutex 句柄有效时先获取互斥锁，再调用 `HAL_UART_Transmit()` 发送，发送完成后释放互斥锁。

同时，`get fault` / fault snapshot 输出也统一改为调用 `Uart_TxText()`。这样无论输出来自 `CmdTask` 还是 `LogTask`，最终都走同一套发送保护。

本阶段还遇到过 Flash 空间接近上限的问题。工程中状态日志需要输出 PID 浮点参数，但 STM32F103C8Tx Flash 空间有限，如果启用 newlib-nano 的 `-u _printf_float`，链接体积会明显增加。当前方向是避免依赖 printf 浮点支持，在日志中使用手动格式化方式输出三位小数，并保持 printf float 选项关闭。

## 2. 串口命令回归测试记录

完成 UART TX mutex 后，进行了串口命令回归测试。测试覆盖了运行、停止、状态查询、无故障状态下 fault snapshot 查询、非法命令、越界命令、快速连续 PID 参数命令、目标来源切换和 `rst`。

运行测试发送：

```text
run=1
```

设备返回：

```text
OK:CMD_QUEUED
```

随后状态日志显示：

```text
ms:9037,enable:1,State:RUN,source:ADC,target:551,actual:642,delta:3,PWM:132,adc1:2261,adc2:2058,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

这次运行时 `RUN` 状态、PWM 输出和编码器 `actual` 均有反馈，说明运行命令已经走通到控制任务，电机控制链路在该测试条件下能工作。

停止测试发送 `run=0` 或 `stop`，设备返回 `OK:CMD_QUEUED`，状态日志显示：

```text
ms:63253,enable:0,State:IDLE,source:ADC,target:551,actual:0,delta:0,PWM:0,adc1:2257,adc2:2057,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

停止后系统回到 `IDLE`，PWM 归零，实际速度为 0，未触发故障。

`status` 能返回完整状态行；`get fault` 在当前无故障状态下返回 `FAULT_SNAPSHOT_NONE`；非法命令 `hajimiyolanbeinvdou` 返回 `ERR:BAD_CMD`；越界目标速度 `t=100000` 返回 `ERR:BAD_CMD`。这些结果说明基础命令解析、错误命令处理和目标速度边界保护初步有效。

快速连续发送 20 条 `kp=0.01` 时，每条均返回 `OK:CMD_QUEUED`。本轮测试中未再观察到 status / OK / fault 粘连，也没有观察到明显丢命令现象。这个结果可以作为 CmdQueue 和 UART TX mutex 的初步压力测试记录。

目标来源切换测试中，`set target adc` 后状态日志显示 `source:ADC`，`set target uart` 后状态日志显示 `source:Uart`。`rst` 在当前无故障状态下能入队，执行后系统保持 `IDLE`、`fault:NONE`、`PWM:0`。

需要注意的是，本轮没有进行故障注入测试，也没有验证故障状态下 `rst` 的恢复效果。因此这里只能记录“无故障状态下查询通过”和“无故障状态下 rst 入队正常”。后续需要单独制造故障场景，再补充 `get fault` 和 `rst` 的故障态记录。
