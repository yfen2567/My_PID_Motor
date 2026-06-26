# My_PID_Motor

`My_PID_Motor` 是一个基于 STM32F103C8Tx、STM32CubeIDE、HAL 和 FreeRTOS 的直流电机闭环控制项目。当前阶段已经从早期裸机控制整理为 RTOS 任务化结构：控制逻辑由 `ControlTask` 周期执行，串口命令由 `CmdTask` 接收解析并通过队列传递，状态日志由 `LogTask` 周期输出。

本仓库用于记录个人独立电机控制项目的代码、调试过程和阶段测试结果。

## 硬件平台

| 项目 | 当前配置 |
|---|---|
| MCU | STM32F103C8Tx |
| 开发环境 | STM32CubeIDE + STM32CubeMX + HAL |
| RTOS | FreeRTOS / CMSIS-RTOS2 |
| PWM 输出 | TIM1_CH1 / PA8 |
| 电机方向 | PA1 / PA2 |
| 编码器 | TIM2 Encoder，PA15 / PB3 |
| ADC | ADC1 + DMA，当前用于目标速度输入/辅助采集 |
| 串口 | USART1，PA9 TX / PA10 RX，115200-8-N-1 |
| HAL Tick | TIM4 |

## 已实现功能

- TIM1 PWM + 方向 GPIO 控制直流电机。
- TIM2 Encoder 读取编码器反馈并计算速度。
- ADC1 + DMA 采集模拟输入，可作为目标速度来源。
- PID 控制、前馈输出、启动助推和 PWM 限幅。
- 基础故障检测与故障快照，包括超速、编码器丢失、PWM 饱和等方向。
- FreeRTOS 三任务结构：`ControlTask`、`CmdTask`、`LogTask`。
- `CmdQueue` 用于 `CmdTask -> ControlTask` 的控制命令传递。
- `uartTxMutex` 用于保护串口发送，避免 status / OK / fault 输出交叉粘连。
- UART ReceiveToIdle + ring buffer + 小型命令行队列，用于接收连续串口命令。

## 软件模块结构

```text
Core/
  CubeMX 生成的 HAL 初始化、FreeRTOS 对象创建、中断和系统文件

App/Inc/
  应用层头文件，包括 Control、Uart、ADC_Sensor、Encoder、Motor、App_Cmd、App_Rtos

App/Src/
  电机控制、串口接收、命令解析、传感器和驱动封装

App/Task/Inc
App/Task/Src
  FreeRTOS 任务和命令服务，包括 ControlTask、CmdTask、LogTask、Cmd_Service

Lib/
  PID 等通用算法模块

docs/
  架构说明、串口命令、测试用例、测试报告和调试记录
```

## FreeRTOS 任务划分

| 任务 | 文件 | 周期/触发 | 职责 |
|---|---|---|---|
| `ControlTask` | `App/Task/Src/Control_Task.c` | 10 ms | 非阻塞读取 `CmdQueue`，应用控制命令，执行 `Control_Tick10ms()` |
| `CmdTask` | `App/Task/Src/CmdTask.c` | 5 ms | 调用 `Uart_Task()` 组行，解析命令，查询类命令直接输出，控制类命令入队 |
| `LogTask` | `App/Task/Src/LogTask.c` | 3000 ms | 周期输出状态日志 |

RTOS 对象在 `Core/Src/freertos.c` 中创建：

- `CmdQueue`：长度 16，item 为 `App_Cmd_t *`。
- `uartTxMutex`：保护所有 `Uart_TxText()` 文本发送。

## 串口命令简介

串口命令以文本形式发送，建议追加 `\r\n`。当前实际支持的主要命令如下：

| 命令 | 功能 |
|---|---|
| `run=1` | 启动控制 |
| `stop` / `run=0` | 停止控制 |
| `t=xxx` | 设置 UART 目标速度，范围 `-1000` 到 `1000` |
| `set target uart` | 切换目标来源为 UART |
| `set target adc` | 切换目标来源为 ADC |
| `kp=x` | 设置 Kp，范围 `0.0` 到 `100.0` |
| `ki=x` | 设置 Ki，范围 `0.0` 到 `100.0` |
| `kd=x` | 设置 Kd，范围 `0.0` 到 `100.0` |
| `status` | 立即输出一次状态 |
| `help` | 输出命令帮助 |
| `get fault` | 输出故障快照 |
| `rst` | 复位故障/控制状态 |

控制类命令成功解析并入队后，典型返回为：

```text
OK:CMD_QUEUED
```

详细命令说明见 [docs/serial_commands.md](docs/serial_commands.md)。

## 阶段测试记录

当前阶段已完成 FreeRTOS 任务化，并形成 `CmdTask / ControlTask / LogTask / CmdQueue` 的基础命令链路。串口发送侧已加入 `uartTxMutex`，所有文本输出统一走 `Uart_TxText()`，用于避免 `status`、`OK`、`fault` 输出交叉。

本轮手动串口测试覆盖了：

- `run=1`：命令入队后系统进入 `RUN`，PWM 有输出，编码器 `actual` 有反馈。
- `run=0` / `stop`：命令入队后系统回到 `IDLE`，PWM 归零。
- `status`：可以正常返回状态、速度、PWM、ADC、fault 和 PID 参数。
- `get fault`：无故障状态下返回 `FAULT_SNAPSHOT_NONE`。
- 非法命令：`hajimiyolanbeinvdou` 返回 `ERR:BAD_CMD`。
- 越界目标速度：`t=100000` 返回 `ERR:BAD_CMD`。
- 快速连续发送 20 条 `kp=0.01`：每条均返回 `OK:CMD_QUEUED`，未观察到串口输出粘连或明显丢命令。
- `set target adc` / `set target uart`：状态日志能显示 `source:ADC` 和 `source:Uart`。
- `rst`：无故障状态下可以入队，执行后系统保持 `IDLE`、`fault:NONE`、`PWM:0`。

关键运行日志示例：

```text
ms:9037,enable:1,State:RUN,source:ADC,target:551,actual:642,delta:3,PWM:132,adc1:2261,adc2:2058,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

详细测试记录见：

- [docs/test_cases.md](docs/test_cases.md)
- [docs/test_report.md](docs/test_report.md)
- [docs/debug_record.md](docs/debug_record.md)
- [docs/rtos_architecture.md](docs/rtos_architecture.md)
- [docs/serial_commands.md](docs/serial_commands.md)

## 当前阶段结论

当前 RTOS + CmdQueue + UART TX mutex 版本已经完成基础命令链路和一轮手动串口回归测试。控制任务不等待串口输入，控制类命令通过 `CmdQueue` 交给 `ControlTask`，周期日志和命令响应通过 `uartTxMutex` 做发送保护。

本轮测试说明运行/停止、状态查询、非法命令处理、目标来源切换、基础参数设置和无故障状态下 fault 查询均能按当前代码逻辑工作；测试中未再观察到 status / OK / fault 输出粘连。

该阶段结论为“手动回归测试初步通过”，还不是完整压力测试或完整故障测试完成。

## 后续计划

- 补充 Python 串口半自动测试，覆盖常用命令和异常命令。
- 补充闭环 PID 调参记录，记录不同参数下的实际响应。
- 补充故障注入测试，验证 `get fault` 有快照时的输出和故障状态下 `rst` 的恢复效果。
- 扩大快速连续命令测试规模，观察 `CmdQueue`、命令行队列和 UART ring buffer 的边界行为。
- 在当前基础 RTOS 命令链路稳定后，再继续整理更完整的故障处理和长期运行测试记录。
