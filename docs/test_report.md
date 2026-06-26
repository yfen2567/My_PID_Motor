# RTOS 串口命令回归测试报告

## 1. 测试目的

本次测试用于确认当前 `My_PID_Motor` 的 FreeRTOS 串口命令链路是否能正常工作。重点验证 `CmdTask -> CmdQueue -> ControlTask` 路径下的运行、停止、参数设置、目标来源切换、状态查询、故障快照查询和错误命令处理。同时观察加入 `uartTxMutex` 后，`status`、`OK`、`fault` 等串口输出是否还会出现交叉粘连。

## 2. 测试环境

- 工程：`My_PID_Motor`
- 平台：STM32F103C8Tx
- RTOS：FreeRTOS / CMSIS-RTOS2
- 串口：USART1，`115200-8-N-1`
- 测试方式：串口助手手动发送命令并观察返回日志
- 当前结构：`ControlTask` 周期控制，`CmdTask` 处理串口接收、解析和入队，`LogTask` 周期输出状态，`CmdQueue` 传递控制命令，`uartTxMutex` 保护串口发送。

## 3. 当前任务结构

`CmdTask` 周期调用 `Uart_Task()` 处理 UART ring buffer 中的数据，并通过 `Uart_ReadLine()` 读取完整命令行。命令由 `App_Cmd_Parse()` 解析：查询类命令如 `status`、`help`、`get fault` 可由 `CmdTask` 直接处理；控制类命令如 `run=1`、`run=0`、`stop`、`t=xxx`、`kp=x`、`ki=x`、`kd=x`、`set target adc`、`set target uart`、`rst` 会被打包成 `App_Cmd_t` 并放入 `CmdQueue`。

`ControlTask` 每 10 ms 运行，非阻塞读取 `CmdQueue`，取到命令后调用 `Control_ApplyCommand()` 修改控制状态，然后执行 `Control_Tick10ms()`。`LogTask` 每 3000 ms 输出一次周期状态。所有文本输出统一通过 `Uart_TxText()`，由 `uartTxMutex` 做发送保护。

## 4. 测试命令与实际返回

### 4.1 run=1 运行测试

输入：

```text
run=1
```

返回：

```text
OK:CMD_QUEUED
```

状态日志：

```text
ms:9037,enable:1,State:RUN,source:ADC,target:551,actual:642,delta:3,PWM:132,adc1:2261,adc2:2058,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

### 4.2 run=0 / stop 停止测试

输入：

```text
run=0
```

或：

```text
stop
```

返回：

```text
OK:CMD_QUEUED
```

状态日志：

```text
ms:63253,enable:0,State:IDLE,source:ADC,target:551,actual:0,delta:0,PWM:0,adc1:2257,adc2:2057,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

### 4.3 status 查询测试

输入：

```text
status
```

返回状态日志：

```text
ms:161754,enable:0,State:IDLE,source:ADC,target:551,actual:0,delta:0,PWM:0,adc1:2261,adc2:2049,fault:NONE,kp:0.000,ki:0.000,kd:0.000
```

### 4.4 get fault 故障快照查询

输入：

```text
get fault
```

返回：

```text
FAULT_SNAPSHOT_NONE
```

### 4.5 非法命令测试

输入：

```text
hajimiyolanbeinvdou
```

返回：

```text
ERR:BAD_CMD
```

### 4.6 目标速度越界测试

输入：

```text
t=100000
```

返回：

```text
ERR:BAD_CMD
```

### 4.7 快速连续命令测试

测试方式：

```text
快速连续发送 20 条 kp=0.01
```

结果：

```text
每条均返回 OK:CMD_QUEUED
```

### 4.8 set target adc 测试

输入：

```text
set target adc
```

返回：

```text
OK:CMD_QUEUED
```

状态日志：

```text
ms:460838,enable:0,State:IDLE,source:ADC,target:551,actual:0,delta:0,PWM:0,adc1:2260,adc2:2054,fault:NONE,kp:0.010,ki:0.000,kd:0.000
```

### 4.9 set target uart 测试

输入：

```text
set target uart
```

返回：

```text
OK:CMD_QUEUED
```

状态日志：

```text
ms:503006,enable:0,State:IDLE,source:Uart,target:551,actual:0,delta:0,PWM:0,adc1:2266,adc2:2056,fault:NONE,kp:0.010,ki:0.000,kd:0.000
```

### 4.10 rst 复位命令测试

输入：

```text
rst
```

返回：

```text
OK:CMD_QUEUED
```

状态日志：

```text
ms:545174,enable:0,State:IDLE,source:Uart,target:551,actual:0,delta:0,PWM:0,adc1:2260,adc2:2053,fault:NONE,kp:0.010,ki:0.000,kd:0.000
```

### 4.11 参数设置补充测试

上一轮手动测试中依次发送：

```text
t=100
kp=0.1
ki=0.1
kd=0.1
```

设备返回：

```text
OK:CMD_QUEUED
OK:CMD_QUEUED
OK:CMD_QUEUED
OK:CMD_QUEUED
```

随后周期状态日志显示：

```text
ms:93372,enable:0,State:IDLE,source:Uart,target:100,actual:0,delta:0,PWM:0,adc1:2256,adc2:2046,fault:NONE,kp:0.100,ki:0.100,kd:0.100
```

## 5. 结果分析

`run=1` 测试中，命令返回 `OK:CMD_QUEUED`，后续日志显示 `enable:1`、`State:RUN`、`PWM:132`，并且编码器反馈 `actual:642`、`delta:3`，说明运行命令已通过队列传递到控制任务，控制输出和速度反馈链路在该测试中有效。

`run=0` / `stop` 测试中，停止后日志显示 `enable:0`、`State:IDLE`、`PWM:0`、`actual:0`，说明停止命令可以使系统回到空闲状态并停止输出。

`status` 命令能够返回完整状态字段，包括状态、目标来源、目标速度、实际速度、PWM、ADC、fault 和 PID 参数。`get fault` 在当前无故障状态下返回 `FAULT_SNAPSHOT_NONE`，说明无故障状态下的查询路径正常，但本轮没有进行故障注入，因此不能认为故障快照功能已经完整验证。

非法命令 `hajimiyolanbeinvdou` 返回 `ERR:BAD_CMD`，目标速度越界命令 `t=100000` 也返回 `ERR:BAD_CMD`，说明命令解析和参数边界保护具备基础效果。

快速连续发送 20 条 `kp=0.01` 时，每条均返回 `OK:CMD_QUEUED`，未观察到串口输出粘连，也未观察到明显丢命令现象。该结果可以作为 CmdQueue 和 UART TX mutex 的初步压力测试，但仍不等同于完整压力测试。

`set target adc` 和 `set target uart` 测试中，状态日志分别显示 `source:ADC` 和 `source:Uart`，说明目标来源切换可以通过状态日志观察到。`rst` 在无故障状态下返回 `OK:CMD_QUEUED`，执行后系统保持 `IDLE`、`fault:NONE`、`PWM:0`，本轮没有测试故障状态下的恢复效果。

## 6. 测试结论

当前串口命令链路、运行/停止、状态查询、非法命令处理、目标速度越界保护、目标来源切换和基础参数设置已完成手动验证。UART TX mutex 修改后，本轮测试未观察到 status、OK、fault 输出粘连。

本结论只说明当前阶段手动回归测试通过，不能扩展为长期稳定性结论，也不能扩展为全功能验证结论。

## 7. 后续待补充测试

- 故障注入后执行 `get fault`，验证是否输出有效 fault snapshot。
- 故障状态下发送 `rst`，验证是否能清故障并回到可重新运行状态。
- 用 Python 串口脚本做半自动回归测试，减少人工复制粘贴误差。
- 针对 PID 参数做真实闭环调参记录，不只验证参数能设置。
- 扩大快速连续命令测试规模，观察 `CmdQueue`、UART 行队列和 ring buffer 边界行为。
- 在电机运行状态下混合发送 `status`、参数命令和 `get fault`，继续观察串口输出互斥效果。
