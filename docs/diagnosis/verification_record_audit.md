# My_PID_Motor v2.0 验证记录归档审计

## 一、审计目的

本审计用于检查当前项目中的测试结论、验证文档、原始日志和源码修改是否能够相互对应，避免在证据链不完整、临时测试配置尚未确认或人工确认被误写成完整自动化验证时进行 commit。

本轮只进行现状核对和归档审计，不重新测试，不修改源码，不重新构建或烧录，也不执行 commit。

## 二、当前未提交内容

审计时真实工程 Git 状态如下：

| 路径 | Git 状态 | 审计结果 |
|---|---|---|
| `App/Inc/app_config.h` | 已修改 | 仅保留经过验证的 `APP_PID_DEFAULT_KP 0.0f → 0.05f`；临时日志周期已恢复为正式默认值 1000 |
| `docs/diagnosis/control_tuning_summary_kp_0p05.md` | 未跟踪 | 文件存在，记录零 PID 前馈、Kp=0.05 全范围和负载扰动补充 |
| `docs/diagnosis/load_disturbance_600_result.md` | 未跟踪 | 文件存在，记录 target=600 人工轻载扰动结果及结论边界 |
| `docs/diagnosis/verification_record_audit.md` | 未跟踪 | 本审计新增文件 |

未发现其他已跟踪 docs 或 release 文件存在未提交修改，也未发现其他未跟踪文件。

`App/Inc/app_config.h` 当前同时包含：

```c
#define APP_PID_DEFAULT_KP 0.05f
#define APP_LOG_TICK 1000
```

`APP_PID_DEFAULT_KP=0.05f` 是由零 PID 前馈基线、Kp=0.05 全范围验证和 target=600 负载扰动验证共同支撑的正式默认参数。`APP_LOG_TICK=200` 仅用于控制观察和负载扰动测试期间提高日志观察密度，不作为正式默认配置；当前已恢复为 `APP_LOG_TICK=1000`。后续如需密集采样，优先通过 LLCOM 脚本主动发送 `status`，不改变固件默认日志周期。

## 三、验证记录清单

证据等级：A 表示正式文档和所需原始证据均已归档；B 表示有正式文档且原始日志文件名明确，但日志未归档进项目；C 表示只有人工确认、人工整理结论或文档内片段，没有独立完整原始日志；D 表示只有计划、没有结果；N/A 表示已明确暂停且当前不需要测试。

| 项目 | 是否已有文档 | 是否有原始日志 | 当前证据等级 | 是否需要补测试 | 处理建议 |
|---|---|---|---|---|---|
| 20 条命令压力测试 | 是，`cmd_queue_pressure_test.md`、`v2_diagnosis_summary.md` | 文档记录 `D:\llcom_uart_pressure_log.txt`，但审计时未在原路径、项目或工作区找到 | B | 否 | 不重做；若以后找到原文件，只补 hash 和归档位置 |
| 50 条命令压力测试 | 是，`cmd_queue_pressure_test.md`、`v2_diagnosis_summary.md` | 文档记录 `D:\llcom_uart_pressure_50_log.txt`，但审计时未在原路径、项目或工作区找到 | B | 否 | 不重做；若以后找到原文件，只补 hash 和归档位置 |
| 100 条命令压力测试暂停决定 | 是，压力测试文档和总结均明确暂停 | 不适用 | N/A | 否 | 保持暂停，不补做 |
| `get fault` 无快照分支 | 是，`fault_snapshot_review.md`、`v2_diagnosis_summary.md` | 正式文档内保存了带时间戳的串口片段，未见独立完整日志文件 | C（含原始片段） | 否 | 保留现有片段并明确没有独立日志，不补编日志 |
| fault snapshot 字段评审 | 是，`fault_snapshot_review.md` | 不适用；该项主要依据当前源码结构与输出格式 | A（代码评审类） | 否 | 保留代码评审结论；真实有快照分支仍不得写成已实测 |
| 零 PID 全量前馈基线 | 是，`control_tuning_summary_kp_0p05.md` | 有，`D:\llcom_uart_forward_reverse_500_1000_test_log.txt`；未归档进项目或工作区 | B | 否 | 记录外部路径和 hash，是否复制入仓库由项目负责人决定 |
| Kp=0.05、target=±600 验证 | 是，`control_tuning_summary_kp_0p05.md` | 有，`D:\llcom_uart_kp005_forward_reverse_600_test_log.txt`；未归档进项目或工作区 | B | 否 | 记录外部路径和 hash，不重做 |
| Kp=0.05 全范围验证 | 是，`control_tuning_summary_kp_0p05.md` | 有，`D:\llcom_uart_kp005_forward_reverse_500_1000_test_log.txt`；未归档进项目或工作区 | B | 否 | 记录外部路径和 hash，不重做 |
| 默认 Kp=0.05 决策 | 是，`control_tuning_summary_kp_0p05.md` | 依赖上述 Kp 验证日志；日志仍在外部 | B | 否 | `APP_PID_DEFAULT_KP=0.05f` 可保留，并与调参文档对应 |
| 复位后 `kp:0.050` 检查 | 是，调参总结中有记录 | 无单独完整日志；用户人工确认 | C | 否 | 明确标记为人工确认，不表述为完整自动化验证 |
| target=600 负载扰动测试 | 是，`load_disturbance_600_result.md`，调参总结含补充 | 有，`D:\llcom_load_disturbance_600_log.txt`；未归档进项目或工作区 | B | 否 | 记录外部路径和 hash；保留定性结论边界，不重做 |
| 最终 stop 后安全归零记录 | 是，调参总结和负载扰动结果均有记录 | 被全范围和负载扰动外部原始日志覆盖，未单独归档 | B（由高层测试覆盖） | 否 | 不恢复独立 stop 测试；继续引用已有日志中的最终状态 |

## 四、当前已知缺口

- 四份近期控制日志目前只保存在 `D:\`，没有复制进真实工程或 Codex 工作区，统一标记为“原始日志未归档”。
- 20/50 条压力测试文档明确记录了原始日志文件名，但本次审计未在记录路径、真实工程或 Codex 工作区找到对应文件；不能因此补编日志，也不据此要求重做测试。
- 复位后 `kp:0.050、ki:0.000、kd:0.000` 只有用户人工确认，未单独保存完整复位检查日志；它不能写成完整自动化验证。
- `get fault` 无快照分支在正式文档中保留了带时间戳的真实输出片段，但没有独立完整日志文件。
- fault snapshot 字段评审属于源码与格式评审；它不等同于真实有快照分支已经通过硬件测试。
- 最终 stop 安全归零已被零 PID 全范围、Kp 全范围和负载扰动测试覆盖，不需要重新生成独立 stop 测试。
- 100 条压力测试只有明确暂停决定，没有测试结果，不能写成已通过。
- `APP_LOG_TICK=200` 已确认为测试期间的临时诊断设置，正式默认配置已经恢复为 `APP_LOG_TICK=1000`，该项不再是 commit 前待确认缺口。

## 五、不需要重做测试的项目

- 20/50 条命令压力已有正式记录，不重做；
- 100 条压力已暂停，不补做；
- Kp=0.05 全范围验证已有结论，不重做；
- target=600 负载扰动日志可用，不重做；
- 当前不做 ADC、目标阶跃、Ki/Kd 试探或故障注入。

## 六、需要补归档但不补测试的项目

当前找到的外部日志及完整性标识如下：

| 原始日志 | 当前外部路径 | SHA-256 | 当前归档状态 |
|---|---|---|---|
| 零 PID 全量前馈基线 | `D:\llcom_uart_forward_reverse_500_1000_test_log.txt` | `837D9390AFF98D2E8BB21200125991FAE2A60225D041BF491362329396502F8F` | 未归档进项目或工作区 |
| Kp=0.05、target=±600 | `D:\llcom_uart_kp005_forward_reverse_600_test_log.txt` | `0D1F668FF6426E2E998E37FAD309A9EC1AFC9D5270E300EEEEAE2F5F4ABEC7E7` | 未归档进项目或工作区 |
| Kp=0.05 全范围 | `D:\llcom_uart_kp005_forward_reverse_500_1000_test_log.txt` | `466CAF9E69C52B637D784546016C63172EA302A5562CDC82AA63DEC91D4A9918` | 未归档进项目或工作区 |
| target=600 负载扰动 | `D:\llcom_load_disturbance_600_log.txt` | `EC3B470857A248685BB6A715E9BBFD100BB78B3641E941F7BCD6EA07E1F60894` | 未归档进项目或工作区 |

后续只需要进行归档决策，不需要补测试：

- 由项目负责人决定是否把上述日志复制到合适的 `logs/diagnosis/` 子目录，或者只在项目文档中长期保留外部路径、文件大小和 hash；
- 如果以后找到 `llcom_uart_pressure_log.txt` 和 `llcom_uart_pressure_50_log.txt`，只补充归档位置和 hash；
- 如果找不到压力测试原始日志，继续标记“原始日志未归档，结论来自正式文档中的人工整理结果”，不得补编日志；
- 复位检查和 `get fault` 片段没有独立日志时，只保持现有证据等级，不重新包装成完整日志。

## 七、commit 前检查建议

1. `APP_LOG_TICK=200` 已确认为临时诊断设置，正式默认配置已恢复为 `APP_LOG_TICK=1000`；
2. 后续需要密集采样时，优先使用 LLCOM 脚本主动发送 `status`，不修改固件默认日志周期；
3. 已验证的 `APP_PID_DEFAULT_KP=0.05f` 可以与调参验证文档一起提交；
4. `control_tuning_summary_kp_0p05.md` 和 `load_disturbance_600_result.md` 可以提交；
5. 原始日志是否进入仓库，由项目负责人确认后再决定；
6. 本轮只完成配置恢复和审计更新，不执行 commit。
