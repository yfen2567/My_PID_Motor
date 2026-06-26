# 阶段测试用例

本文件记录当前 RTOS + CmdQueue + UART TX mutex 阶段的手动串口测试结果。已经实测的项目按实际现象填写“通过”；没有完整实测的项目只写“待测试”。

| 编号 | 测试项 | 输入命令 | 预期结果 | 实际结果 | 结论 | 备注 |
|---|---|---|---|---|---|---|
| TC-001 | 运行命令 | `run=1` | 返回 `OK:CMD_QUEUED`；系统进入 RUN；`enable=1`；PWM 有输出；编码器有速度反馈；无故障 | 返回 `OK:CMD_QUEUED`；日志显示 `enable:1,State:RUN,target:551,actual:642,delta:3,PWM:132,fault:NONE` | 通过 | 使用 ADC 目标来源，目标约为 551 |
| TC-002 | 停止命令 run=0 | `run=0` | 返回 `OK:CMD_QUEUED`；系统回到 IDLE；`enable=0`；PWM 归零；实际速度为 0；无故障 | 返回 `OK:CMD_QUEUED`；日志显示 `enable:0,State:IDLE,actual:0,delta:0,PWM:0,fault:NONE` | 通过 | 与 `stop` 等效 |
| TC-003 | 停止命令 stop | `stop` | 返回 `OK:CMD_QUEUED`；系统回到 IDLE；PWM 归零；无故障 | 已按 `run=0 / stop` 停止测试记录确认，停止后状态为 IDLE，PWM 为 0 | 通过 | 本轮记录未单独提供第二条 stop 原始日志，结论来自同一组停止测试 |
| TC-004 | status 查询 | `status` | 立即输出当前状态，包含状态、速度、PWM、ADC、fault、PID 参数 | 返回 `ms:161754,enable:0,State:IDLE,source:ADC,target:551,actual:0,delta:0,PWM:0,adc1:2261,adc2:2049,fault:NONE,kp:0.000,ki:0.000,kd:0.000` | 通过 | 查询命令不进入控制命令队列 |
| TC-005 | 无故障状态下查询 fault snapshot | `get fault` | 无故障快照时返回 `FAULT_SNAPSHOT_NONE` | 返回 `FAULT_SNAPSHOT_NONE` | 通过 | 仅验证无故障状态下查询；未做故障注入 |
| TC-006 | 非法命令 | `hajimiyolanbeinvdou` | 返回错误，不改变控制状态 | 返回 `ERR:BAD_CMD` | 通过 | 基础错误命令处理有效 |
| TC-007 | 目标速度越界 | `t=100000` | 超出目标速度范围，命令不被接受 | 返回 `ERR:BAD_CMD` | 通过 | 说明目标速度边界保护初步有效 |
| TC-008 | 快速连续 PID 参数命令 | 连续发送 20 条 `kp=0.01` | 每条命令应有回应，不应出现明显粘连或丢命令 | 每条均返回 `OK:CMD_QUEUED`；未观察到串口输出粘连；未观察到明显丢命令 | 通过 | 可作为 CmdQueue 和 UART TX mutex 的初步压力测试 |
| TC-009 | 切换目标来源为 ADC | `set target adc` | 返回 `OK:CMD_QUEUED`；状态日志显示 `source:ADC` | 返回 `OK:CMD_QUEUED`；日志显示 `source:ADC,target:551` | 通过 | 目标来源切换或保持为 ADC |
| TC-010 | 切换目标来源为 UART | `set target uart` | 返回 `OK:CMD_QUEUED`；状态日志显示 `source:Uart` | 返回 `OK:CMD_QUEUED`；日志显示 `source:Uart,target:551` | 通过 | 状态日志能正确显示目标来源 |
| TC-011 | rst 复位命令 | `rst` | 返回 `OK:CMD_QUEUED`；无故障状态下保持安全状态 | 返回 `OK:CMD_QUEUED`；日志显示 `enable:0,State:IDLE,source:Uart,target:551,PWM:0,fault:NONE` | 通过 | 仅验证无故障状态下执行；未验证故障状态下恢复 |
| TC-012 | 设置目标速度 | `t=100` | 返回 `OK:CMD_QUEUED`；后续状态日志显示 `target:100`，`source:Uart` | 返回 `OK:CMD_QUEUED`；日志显示 `source:Uart,target:100` | 通过 | 上一轮参数命令测试记录 |
| TC-013 | 设置 Kp | `kp=0.1` | 返回 `OK:CMD_QUEUED`；后续状态日志显示 `kp:0.100` | 返回 `OK:CMD_QUEUED`；日志显示 `kp:0.100` | 通过 | 上一轮参数命令测试记录 |
| TC-014 | 设置 Ki | `ki=0.1` | 返回 `OK:CMD_QUEUED`；后续状态日志显示 `ki:0.100` | 返回 `OK:CMD_QUEUED`；日志显示 `ki:0.100` | 通过 | 上一轮参数命令测试记录 |
| TC-015 | 设置 Kd | `kd=0.1` | 返回 `OK:CMD_QUEUED`；后续状态日志显示 `kd:0.100` | 返回 `OK:CMD_QUEUED`；日志显示 `kd:0.100` | 通过 | 上一轮参数命令测试记录 |
| TC-016 | 故障注入后的 fault snapshot | 人为制造故障后发送 `get fault` | 输出 `valid:...` 故障快照字段 | 未进行故障注入测试 | 待测试 | 当前只验证了无故障状态下查询 |
| TC-017 | 故障状态下 rst 恢复 | 故障状态下发送 `rst` | 清除故障并回到可重新运行状态 | 未进行故障状态测试 | 待测试 | 当前只验证了无故障状态下 `rst` |
| TC-018 | 真实 PID 调参效果 | 多组 `kp/ki/kd` 组合并观察闭环响应 | 记录超调、稳定性、响应速度等 | 未系统整理 | 待测试 | 本轮只确认参数能设置，不评价调参效果 |
