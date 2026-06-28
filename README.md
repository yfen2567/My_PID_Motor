# My_PID_Motor

基于 STM32F103C8Tx、STM32 HAL 和 FreeRTOS 的直流电机速度控制项目。项目已经完成 RTOS 命令队列发布基线，并在此基础上完成前馈修正、默认比例参数验证和轻载扰动观察。

## 1. 项目状态

当前主线处于 `v2.0-rtos-cmdqueue` 发布基线之后的控制调参补充阶段。

已经完成：

- ControlTask、CmdTask、LogTask 的 FreeRTOS 任务链路；
- UART 命令解析、命令队列传递和状态输出；
- 20 条、50 条连续命令压力验证；
- fault snapshot 字段评审及无快照分支验证；
- 零 PID 前馈基线验证；
- `Kp=0.05, Ki=0, Kd=0` 的正反向全范围验证；
- `target=600` 人工轻载扰动验证；
- 诊断结论、release 基线和验证记录审计。

当前正式默认参数为：

```c
#define APP_PID_DEFAULT_KP 0.05f
#define APP_PID_DEFAULT_KI 0.0f
#define APP_PID_DEFAULT_KD 0.0f
#define APP_LOG_TICK       1000
```

`Kp=0.05` 是经过前馈基线、正反向全范围控制结果和轻载扰动结果共同支撑的已验证默认值。`APP_LOG_TICK=200` 仅曾用于提高测试观察密度，正式默认日志周期已经恢复为 `1000 ms`。

当前不继续执行 ADC 目标源一致性、目标阶跃、Ki/Kd 试探、100 条命令压力、主动故障注入或正反转直接切换测试。

## 2. 主要功能

- STM32F103C8Tx 平台；
- TIM1 CH1（PA8）PWM 输出；
- PA1、PA2 电机方向控制；
- TIM2 编码器接口测速；
- ADC 双通道采样和 ADC/UART 目标源切换；
- 启动助推、速度前馈和 PID 修正；
- 正反向速度控制；
- UART 文本命令与状态查询；
- FreeRTOS 命令队列、串口发送互斥和周期日志；
- 基础故障状态及 fault snapshot 查询接口。

当前主要实测控制路径为 UART 目标源，非零目标的已验证范围为 `±500` 至 `±1000`。

## 3. 控制与命令架构

```text
USART1 接收
    │
    ▼
UART 行缓冲 / 行队列
    │
    ▼
CmdTask：解析命令
    ├── status / help / get fault：直接查询并输出
    └── 控制命令：写入 CmdQueue
                         │
                         ▼
              ControlTask（10 ms）
                         │
             应用命令、读取反馈、执行控制
                         │
             方向引脚 + TIM1 PWM 输出

LogTask（1000 ms）── 周期输出运行状态
```

命令入队成功返回 `OK:CMD_QUEUED`。该回应只代表命令成功进入队列；控制状态是否改变仍应结合后续 `status` 和真实硬件现象判断。

## 4. FreeRTOS 任务

| 任务 | 周期或阻塞方式 | 主要职责 |
| --- | --- | --- |
| `ControlTask` | 10 ms 周期 | 消费控制命令、更新控制状态、执行测速和 PWM 输出 |
| `CmdTask` | UART 行接收，约 5 ms 轮询间隔 | 接收并解析命令，执行查询或投递 `CmdQueue` |
| `LogTask` | 1000 ms 周期 | 输出 `status` 格式的运行状态 |

当前应用源码未创建名为 `defaultTask` 的业务任务，因此不把 FreeRTOS 内部空闲任务列入应用任务表。

## 5. 串口命令

串口命令使用文本模式和真实 CRLF 结束符。

| 命令 | 作用 | 备注 |
| --- | --- | --- |
| `run=1` | 启动运行 | 使用当前目标源和目标值 |
| `run=0` / `stop` | 停止运行 | 进入安全停止状态 |
| `status` | 查询当前状态 | 返回控制、反馈、PWM、故障和 PID 字段 |
| `get fault` | 查询 fault snapshot | 无快照时返回 `FAULT_SNAPSHOT_NONE` |
| `t=600` | 设置 UART 速度目标 | 绝对上限为 1000；当前非零支持区间为 `±500…±1000` |
| `kp=0.05` | 设置比例系数 | 在线参数设置 |
| `ki=0` | 设置积分系数 | 当前默认值为 0 |
| `kd=0` | 设置微分系数 | 当前默认值为 0 |
| `set target uart` | 选择 UART 目标源 | 当前控制验证采用该目标源 |
| `set target adc` | 选择 ADC 目标源 | 已实现，低速策略一致性尚未专项验证 |
| `rst` | 复位控制相关状态 | 具体行为以当前固件实现为准 |

`status` 当前包含：

```text
ms, enable, State, source, target, actual, delta, PWM,
adc1, adc2, fault, kp, ki, kd
```

## 6. 控制验证结果

### 6.1 Kp=0.05 正反向全范围验证

测试条件：UART 目标源，`Kp=0.05, Ki=0, Kd=0`。

| target | 稳态 actual 均值 | PWM 范围 | 平均误差 |
| -----: | ----------------: | -------: | -------: |
| 500 | 496.8 | 112～118 | -3.2 |
| 600 | 608.3 | 125～130 | +8.3 |
| 800 | 811.1 | 150～154 | +11.1 |
| 1000 | 999.3 | 176～178 | -0.7 |
| -500 | -496.8 | -116～-112 | +3.2 |
| -600 | -599.8 | -130～-125 | +0.2 |
| -800 | -799.7 | -154～-150 | +0.3 |
| -1000 | -993.6 | -178～-176 | +6.4 |

在上述八个档位中，电机均能持续运行，正反向控制方向正确，平均误差不超过约 11 RPM，PWM 仅在前馈值附近小幅修正；测试记录中未观察到明显振荡，`fault:NONE`。结合约 42.9 RPM 的测速量化粒度，现有证据不支持继续增加 Ki 或 Kd，因此保留 `Kp=0.05, Ki=0, Kd=0` 作为当前默认值。

### 6.2 target=600 人工轻载扰动

| 阶段 | actual 均值 | 最低 actual | PWM 均值 | 最高 PWM |
| --- | ---: | ---: | ---: | ---: |
| 扰动前 | 604.7 | — | 127.5 | — |
| 第一次负载 | 580.7 | 471 | 128.7 | 134 |
| 第一次释放 | 594.7 | — | 128.1 | — |
| 第二次负载 | 568.8 | 385 | 129.3 | 138 |
| 第二次释放 | 601.5 | — | 127.7 | — |

两次轻微负载期间均观察到 `actual` 下降、PWM 同步提高；释放负载后，`actual` 回到 600 附近，PWM 回到约 128 的基线附近。全程记录为 `fault:NONE`，最终 `stop` 后 `enable:0`、`State:IDLE`、`PWM:0`、`actual:0`。

本项只能支持“系统具备基本轻载扰动补偿能力”的定性结论。负载由人工施加，大小和动作时刻没有精确测量，因此不能据此计算精确恢复时间、控制带宽或其他严格动态性能指标。

### 6.3 串口命令队列

在已经执行的 20 条和 50 条连续 `kp=0.01` 测试条件下，未发现命令丢失、`CMD_QUEUE_FULL`、错误返回或测试后系统卡死。100 条极限压力测试已明确暂停，20/50 条结果不能外推为未测试负载下的保证。

## 7. 诊断与发布资料

- [v2.0 诊断结论汇总](docs/diagnosis/v2_diagnosis_summary.md)
- [命令队列压力测试](docs/diagnosis/cmd_queue_pressure_test.md)
- [fault snapshot 字段评审](docs/diagnosis/fault_snapshot_review.md)
- [Kp=0.05 控制调参记录](docs/diagnosis/control_tuning_summary_kp_0p05.md)
- [target=600 负载扰动结果](docs/diagnosis/load_disturbance_600_result.md)
- [验证记录归档审计](docs/diagnosis/verification_record_audit.md)
- [v2.0 release notes](release/v2.0-rtos-cmdqueue/RELEASE_NOTES.md)
- [v2.0 release 诊断摘要](release/v2.0-rtos-cmdqueue/DIAGNOSIS_SUMMARY.md)
- [release 文件校验值](release/v2.0-rtos-cmdqueue/SHA256SUMS.txt)

`v2.0-rtos-cmdqueue` release 目录同时保留对应 ELF。部分控制测试原始日志保存在项目目录外，证据完整度和人工确认项以《验证记录归档审计》为准；没有归档的日志不会被表述为已经完整纳入仓库。

## 8. 已知边界

- 当前主要控制结论来自 UART 目标源，ADC 目标源虽已实现，但最低转速策略一致性尚未专项验证；
- 当前连续运行验证范围为 `±500…±1000`，不宣称支持物理上不可持续的更低连续转速；
- `Ki=0`、`Kd=0` 是基于当前稳态结果和测速分辨率作出的阶段性选择，不代表所有负载和机械条件下的最终最优参数；
- 人工轻载扰动测试仅反映补偿趋势，不代表精确动态性能测量；
- 100 条命令压力、实际 fault snapshot 触发分支、主动故障注入和正反转直接切换未在当前阶段验证；
- 本项目是嵌入式控制与诊断验证项目，不应直接视为工业级电机驱动器或功能安全实现。

## 9. 构建环境

- MCU：STM32F103C8Tx；
- IDE / 工具链：STM32CubeIDE、GNU Arm Embedded Toolchain；
- 配置与驱动：STM32CubeMX、STM32 HAL；
- RTOS：FreeRTOS，CMSIS-RTOS V2 接口；
- 工程入口：使用 STM32CubeIDE 打开仓库根目录下的工程配置。

发布基线的可执行文件及 SHA-256 校验值位于 `release/v2.0-rtos-cmdqueue/`。README 更新不会重新构建或替换该发布文件。

## 10. 目录结构

```text
My_PID_Motor/
├─ App/                    应用配置、控制模块和 FreeRTOS 任务
├─ Core/                   CubeMX 生成的核心初始化与中断代码
├─ Drivers/                STM32 HAL/CMSIS 驱动
├─ Lib/                    项目使用的独立库
├─ Middlewares/            FreeRTOS 等中间件
├─ docs/diagnosis/         诊断、调参、结果和证据审计文档
├─ release/
│  └─ v2.0-rtos-cmdqueue/ 发布说明、诊断摘要、校验值和 ELF
├─ My_PID_Motor.ioc        CubeMX 工程配置
└─ README.md
```

## 11. 可选完善方向

以下项目仅作为未来可能的工程化方向，不属于当前测试计划，也不表示已经验证或承诺实施：

- 明确 ADC 目标源的低速映射和最低连续目标策略；
- 使用脚本自动解析串口日志并生成统计摘要；
- 在具备明确安全方案时补充 fault 与保护路径的真实证据；
- 增加受控、可测量的负载平台，以获得可重复的动态数据；
- 根据具体硬件应用补充电流、温升和长期运行验证。
