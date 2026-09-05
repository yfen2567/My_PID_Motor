# v2.2 串口命令参考

本文描述 `v2.2-param-persist-and-abnormal-regression` 当前固件的实际协议，不把历史版本的 `OK:CMD_QUEUED` 当作当前返回值。

## 串口配置

```text
115200 / 8N1 / 文本模式 / CRLF
```

命令由 USART1 ReceiveToIdle 中断进入 ring buffer，经 CmdTask 组装成行并解析。`status`、`help`、`get fault`、`comm stats` 是查询类命令；其余控制类命令通过对象池和 `ControlCmdQueue` 交给 ControlTask。

## 成功返回语义

v2.2 的 `OK:` 文本表示 ControlTask 或 NvTask 已完成对应动作。例如 `OK:RUN` 表示系统已接受运行状态切换；后续 `status` 用于确认持续状态。

## 命令列表

| 命令 | 功能 | 典型成功返回 | 约束 |
| --- | --- | --- | --- |
| `run=1` | 进入运行状态 | `OK:RUN` | 使用当前 target/source；fault 或 CALIB 状态会拒绝 |
| `run=0` / `stop` | 停止控制 | `OK:STOPPED` | 后续 status 应为 IDLE、PWM=0 |
| `t=<speed>` | 设置 UART 目标速度 | `OK:TARGET_SET` | 整数范围 -1000…1000；非零绝对值必须至少为 500 |
| `set target uart` | 选择 UART target | `OK:TARGET_SOURCE_UART` | 不自动启动电机 |
| `set target adc` | 选择 ADC target | `OK:TARGET_SOURCE_ADC` | 不自动启动电机 |
| `kp=<value>` | 设置 Kp | `OK:KP_SET` | 浮点范围 0…100 |
| `ki=<value>` | 设置 Ki | `OK:KI_SET` | 浮点范围 0…100 |
| `kd=<value>` | 设置 Kd | `OK:KD_SET` | 浮点范围 0…100 |
| `save params` | 保存当前 `kp/ki/kd` | `OK:PARAMS_SAVED` | 仅 IDLE；NvTask 执行 Flash 写入 |
| `load params` | 加载已保存的 `kp/ki/kd` | `OK:PARAMS_LOADED` | 仅 IDLE；加载后由 ControlTask 应用 |
| `reset params` | 恢复默认 PID 并写入 Flash | `OK:PARAMS_RESET` | 仅 IDLE；重启后仍保持默认值 |
| `status` | 输出一次控制状态 | `ms:...` | 查询命令，不进入控制队列 |
| `get fault` | 输出故障快照 | `FAULT_SNAPSHOT_NONE` 或 `valid:...` | 查询命令 |
| `comm stats` | 输出 UART、队列、对象池和 TX 统计 | `COMM_STATS ...` | 查询命令 |
| `help` | 输出命令帮助 | `cmd:...` | 查询命令 |
| `rst` | 清 PID 历史和当前 fault 状态 | `OK:RESET` | 不是 MCU 重启，不能验证参数重启加载 |

## 错误返回

| 返回 | 含义 |
| --- | --- |
| `ERR:BAD_CMD` | 未识别命令或不支持操作 |
| `ERR:BAD_INT` / `ERR:BAD_FLOAT` | 数值格式错误 |
| `ERR:OUT_OF_RANGE` | 目标或 PID 参数超出当前解析范围 |
| `ERR:NOT_IDLE` | 参数存储命令在非 IDLE 状态下执行 |
| `ERR:BUSY` | 正在执行 NvTask 参数操作 |
| `ERR:PARAMS_SAVE_FAILED` | Flash 写入或校验失败 |
| `ERR:PARAMS_LOAD_FAILED` | 存储记录无效或读取失败 |
| `ERR:QUEUE_FULL` / `ERR:CMD_POOL_EMPTY` | 控制命令队列或对象池资源不足 |
| `ERR:CMD_TOO_LONG` / `ERR:CMD_LINE_QUEUE_FULL` / `ERR:RX_OVERFLOW` | UART 接收路径保护触发 |

## status 字段

```text
ms,enable,State,source,target,actual,delta,PWM,adc1,adc2,fault,kp,ki,kd
```

`source`、`target`、`enable`、`State`、`actual`、`PWM` 和 ADC 字段不属于参数持久化字段；只有 `kp/ki/kd` 被 `save params`、`load params` 和 `reset params` 管理。

## v2.2 实机证据

- [参数持久化报告](../reports/v2.2_param_persistence_report.md)
- [异常命令回归报告](../reports/v2.2_abnormal_command_regression_report.md)
- [status/run/stop/restart 报告](../reports/v2.2_status_run_stop_report.md)
