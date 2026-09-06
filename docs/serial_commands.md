# 串口命令说明

串口参数：`115200-8-N-1`。建议串口助手每条命令追加 `\r\n` 或 `\n`。当前命令解析在 `App/Src/App_Cmd.c`，串口组行在 `App/Src/Uart.c`，控制类命令经 `CmdQueue` 交给 `ControlTask` 执行。

## 命令列表

| 命令格式 | 功能 | 参数范围或注意事项 | 示例 | 典型返回 |
|---|---|---|---|---|
| `run=1` | 启动控制，使能电机控制状态 | 当前代码支持 `run=1`，不支持裸 `run` | `run=1` | `OK:RUN` |
| `stop` | 停止控制，关闭 enable | 与 `run=0` 等效 | `stop` | `OK:STOPPED` |
| `run=0` | 停止控制，关闭 enable | 与 `stop` 等效 | `run=0` | `OK:STOPPED` |
| `t=xxx` | 设置 UART 目标速度，并切换为 UART 目标来源 | 整数，范围 `-APP_TARGET_SPEED_MAX` 到 `APP_TARGET_SPEED_MAX`，当前配置为 `-1000` 到 `1000` | `t=600` | `OK:TARGET_SET` |
| `set target uart` | 切换目标速度来源为 UART | 不改变已有目标值 | `set target uart` | `OK:TARGET_SOURCE_UART` |
| `set target adc` | 切换目标速度来源为 ADC | ADC 映射范围按当前 `ADC_Sensor` 配置 | `set target adc` | `OK:TARGET_SOURCE_ADC` |
| `kp=x` | 设置 PID 的 Kp | 浮点数，当前解析范围 `0.0` 到 `100.0` | `kp=0.1` | `OK:KP_SET` |
| `ki=x` | 设置 PID 的 Ki | 浮点数，当前解析范围 `0.0` 到 `100.0` | `ki=0.1` | `OK:KI_SET` |
| `kd=x` | 设置 PID 的 Kd | 浮点数，当前解析范围 `0.0` 到 `100.0` | `kd=0.1` | `OK:KD_SET` |
| `status` | 立即输出一次状态日志 | 查询命令，不进入 `CmdQueue` | `status` | `ms:...,enable:...,State:...` |
| `timing stats` | 输出 ControlTask 当前时序和累计聚合统计 | 查询命令，不进入 `CmdQueue` | `timing stats` | `TIMING_STATS ...` |
| `help` | 输出命令帮助 | 查询命令，不进入 `CmdQueue` | `help` | 多行 `cmd:` 帮助文本 |
| `get fault` | 输出故障快照 | 无故障快照时返回空快照提示 | `get fault` | `FAULT_SNAPSHOT_NONE` 或 `valid:...` |
| `rst` | 复位故障/控制状态 | 控制类命令，经队列交给 `ControlTask` | `rst` | `OK:RESET` |

## 错误返回

| 返回 | 含义 |
|---|---|
| `ERR:BAD_CMD` | 命令无法识别，或参数解析失败/越界 |
| `ERR:CMD_ALLOC_FAIL` | 命令入队前动态内存申请失败 |
| `ERR:CMD_QUEUE_FULL` | `CmdQueue` 已满 |
| `ERR:CMD_QUEUE_NULL` | `CmdQueueHandle` 为空 |
| `ERR:CMD_TOO_LONG` | 单行命令超过 `APP_UART_LINE_SIZE - 1` |
| `ERR:CMD_LINE_QUEUE_FULL` | UART 组好的小型命令行队列已满 |
| `ERR:RX_OVERFLOW` | UART ring buffer 溢出 |

## 状态日志字段

典型状态行：

```text
ms:93372,enable:0,State:IDLE,source:Uart,target:100,actual:0,delta:0,PWM:0,adc1:2256,adc2:2046,fault:NONE,kp:0.100,ki:0.100,kd:0.100
```

其中 `source` 表示目标来源，`target` 为目标速度，`actual` 和 `delta` 来自编码器测速，`PWM` 为当前输出，`fault` 为故障状态，`kp/ki/kd` 为当前 PID 参数。

## timing stats 字段

当前输出格式为：

```text
TIMING_STATS sample_count:...,period_us:...,period_min_us:...,period_max_us:...,max_abs_jitter_us:...,exec_us:...,exec_min_us:...,exec_max_us:...,tick_exec_us:...,tick_exec_min_us:...,tick_exec_max_us:...,timeout_count:...
```

`period_us`、`exec_us` 和 `tick_exec_us` 是最近一次有效样本；`sample_count`、各组 min/max 和 `max_abs_jitter_us` 是自 ControlTask 产生第一份有效样本以来的累计聚合结果。`max_abs_jitter_us` 相对于标称 `10000 us` 周期计算，`timeout_count` 使用当前 `10010 us` 容差阈值。

