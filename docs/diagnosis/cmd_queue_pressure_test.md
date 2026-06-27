# cmd queue 最小压力测试

## 目的

用分级短测试判断连续命令是否造成报满、丢回应、输出粘连或压力后无法查询 status。

## 执行方法

1. 先发送 `stop`、`status`，确认 `enable:0`、`PWM:0`。
2. 使用已确认有效的真实换行方式。
3. 在串口助手中连续发送 `kp=0.01`，先测试 20、50 条并分别保存日志；100 条仅在需要验证极限边界时执行。
4. 每组发送完成后等待约 1 秒，再发送 `status`。
5. 周期 status 不计入 OK/ERR 数量，但必须保留在原始日志中。
6. 若设备复位、持续无响应或 stop/status 无法恢复，立即停止后续组。

## 结果表

| 发送条数 | OK 数量 | ERR 数量 | 有 `CMD_QUEUE_FULL` | 丢回应 | 输出粘连 | 结束后 status 正常 |
|---:|---:|---:|---|---|---|---|
| 20 | 20 | 0 | 否 | 否 | 否（仅 RX 接收块合并显示，每条仍以 CRLF 分隔） | 是 |
| 50 | 50 | 0 | 否 | 否 | 否（仅 RX 接收块合并显示，每条仍以 CRLF 分隔） | 是 |
| 100 |  |  |  |  |  |  |

## 判断

- 正常：OK 数量等于发送条数、ERR 为 0、无丢回应/粘连，结束后 status 正常。
- 可定位边界：出现明确 ERR，但停止发送后 status 可恢复；记录首次出现的组别。
- 失稳：静默丢回应、输出破坏、复位或 status 无法恢复。

## 原始日志

20 条：

```text
测试时间：2026-06-27 14:24:52 起
串口工具：LLCOM
行结束符：真实 CRLF（TX HEX 为 0D 0A）
测试序列：stop -> status -> 20x kp=0.01 -> wait 1s -> status
发送节奏：20 条 kp=0.01 约在 1 秒内发出；单条毫秒级间隔未精确记录
完整原始日志：D:\llcom_uart_pressure_log.txt

核对结果：
- kp=0.01 发送 20 条，对应 OK:CMD_QUEUED 20 条
- stop 另对应 OK:CMD_QUEUED 1 条，因此日志内 OK 总数为 21
- ERR、CMD_QUEUE_FULL、CMD_ALLOC_FAIL、CMD_LINE_QUEUE_FULL 均未出现
- 未发现丢回应
- 多个 OK 虽出现在同一 RX ASCII 块中，但均有 CRLF 分隔，不判定为协议级输出粘连
- 压力期间周期 status 显示 kp:0.010
- 最终 status 正常：enable:0、State:IDLE、PWM:0、fault:NONE、kp:0.010
- 后续周期 status 持续输出，未见系统卡死
```

50 条：

```text
测试时间：2026-06-27 14:46:46 起
串口工具：LLCOM
行结束符：真实 CRLF（脚本主动发送 \r\n，TX HEX 为 0D 0A）
测试序列：stop -> status -> 50x kp=0.01 -> wait 1s -> status
发送配置：kpDelay=20 ms
发送节奏：日志显示 50 条命令约在 14:46:46 至 14:46:48 发完；因时间戳只有秒级，无法精确证明单条实际间隔
完整原始日志：D:\llcom_uart_pressure_50_log.txt

核对结果：
- kp=0.01 发送 50 条，KP_PRESSURE_INDEX 标记 50 条，对应 OK:CMD_QUEUED 50 条
- stop 另对应 OK:CMD_QUEUED 1 条，因此日志内 OK 总数为 51
- ERR、CMD_QUEUE_FULL、CMD_ALLOC_FAIL、CMD_LINE_QUEUE_FULL 均未出现
- 未发现丢回应
- 多个 OK 虽出现在同一 RX ASCII 块中，但均有 CRLF 分隔，不判定为协议级输出粘连
- 压力期间周期 status 显示 enable:0、State:IDLE、PWM:0、fault:NONE、kp:0.010
- 最终 status 正常：enable:0、State:IDLE、PWM:0、fault:NONE、kp:0.010
- 后续周期 status 持续输出，未见系统卡死
```

100 条：

```text

```

一句话结论：在 20 ms 发送间隔、连续 50 条 `kp=0.01` 的条件下，当前固件未出现命令丢失、队列满、错误返回或系统卡死。20 条与 50 条压力测试均通过。暂不继续 100 条测试，除非后续需要专门验证极限边界。  
