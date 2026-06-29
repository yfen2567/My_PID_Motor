# My_PID_Motor v2.1 Release 检查表

> 本文件只用于 release 前核对，不会自动创建 tag 或 release。

## 1. 计划与边界

- [x] 已建立 `docs/verification/v2.1_verification_plan.md`；
- [x] 已建立 `docs/verification/log_field_dictionary.md`；
- [x] 已建立 `docs/verification/load_disturbance_test_plan.md`；
- [x] 已建立本 Release checklist；
- [x] README 已包含 `Verification Status / 验证状态` 边界说明；
- [x] LD-001/002/003 负载扰动子范围已经冻结并形成汇总报告；
- [ ] 完整 v2.1 diagnostic release tag 的整体实际范围已经最终冻结；
- [x] 未执行项、延期项和不能外推的结论已在负载扰动汇总报告和 release notes 草稿中明确。

勾选 `[x]` 仅表示文档骨架已经存在，不表示对应测试已经通过。

## 2. 原始日志与测试报告

- [x] LD-001 原始日志：`logs/v2.1_LD-001_raw.txt`；
- [x] LD-001 报告：`reports/v2.1_LD-001_report.md`；
- [x] LD-002 原始日志：`logs/v2.1_LD-002_raw.txt`；
- [x] LD-002 报告：`reports/v2.1_LD-002_report.md`；
- [x] LD-003 原始日志：`logs/v2.1_LD-003_raw.txt`；
- [x] LD-003 报告：`reports/v2.1_LD-003_report.md`；
- [x] 已建立负载扰动汇总报告：`reports/v2.1_load_disturbance_summary.md`；
- [ ] status 字段完整性检查有原始输出和结论；
- [ ] 半自动 UART 回归有脚本版本、原始输出和统计结果（可延期 / 后续阶段）；
- [ ] 非法命令、越界命令、run/stop 行为有可复现记录；
- [x] LD-001/002/003 日志均记录 `get fault` 无快照分支返回 `FAULT_SNAPSHOT_NONE`；
- [ ] 所有测试日志都写明日期、串口配置、硬件条件、commit 和构建产物标识。

## 3. 故障与异常记录

- [x] 负载扰动测试中的时序问题、stop 后单点 `actual=-85` 和启动瞬态 `actual=1200` 均已保留并说明；
- [x] 上述异常均已标明统计处理和结论边界，未被删除或直接归因为代码问题；
- [x] 本轮未自然出现 fault，因此不存在可保存的故障前 status / 有快照结果；
- [x] 最终文档明确写明“真实有快照分支未验证”，不为补齐记录主动制造破坏性故障；
- [x] fault snapshot 的 `adc_aux` 已按当前代码事实标记为未确认字段，没有当成有效故障测量值。

## 4. 版本与构建产物

- [x] 已记录当前源码基线 HEAD：`dd72ccc8b44cb0235c7c6e505b719adaa48d12b5`（工作区未提交，不能代表完整证据链）；
- [ ] 已记录包含全部候选证据的最终 commit hash；
- [x] 当前工作树状态已经记录；
- [ ] 已记录 STM32CubeIDE、固件包和工具链版本；
- [ ] 已记录构建命令或 CubeIDE 构建配置；
- [ ] 构建成功且警告/错误情况已记录；
- [x] 已盘点现有 ELF 的文件名、大小、生成时间和 SHA-256，但尚未确认其为 v2.1 最终候选产物；
- [ ] 已记录 v2.1 最终候选 ELF/HEX/BIN 的文件名、大小和生成时间；
- [ ] 已计算 v2.1 最终候选构建产物 SHA-256；
- [x] LD-001/002/003 正式原始日志及证据文件已计算 SHA-256；
- [ ] 构建产物、commit 和测试日志能够建立唯一对应关系。

## 5. README 与演示材料

- [x] README 已说明当前项目不是工业级电机控制器；
- [x] README 已说明 PID 参数不能外推到其他电机、负载或驱动板；
- [x] README 已说明测试不能证明长期可靠性或覆盖全部异常；
- [x] README 的“已验证/待验证”状态已按 LD-001/002/003 完成事实更新；
- [x] 已建立 `reports/v2.1_release_notes_draft.md` 草稿；
- [ ] 已形成项目负责人确认的最终 release notes；
- [ ] 如录制演示视频，视频注明固件 commit、测试条件和对应日志（可延期）；
- [x] 演示视频当前明确标记为延期。

## 6. 可以延期的项目

以下项目可以延期，但必须在 release 说明中明确，不得暗示已经完成：

- 演示视频；
- Python 串口半自动回归；
- 真实 fault snapshot 的有快照分支验证（前提是没有自然 fault，且不将其列为已验证能力）；
- 示波器/逻辑分析仪波形和电流测量；
- 日志可视化或自动生成图表；
- ADC target 的专项验证；
- 与本次诊断封版无关的 GUI、上位机或新功能。

延期项不能是未解释的安全异常，也不能与 release 声明中的已验证结论冲突。

## 7. 两层收口与 tag 门槛

### 7.1 负载扰动证据链收口

- [x] LD-001/002/003 负载扰动日志、单项报告和汇总报告已经完成；
- [x] 负载扰动证据索引已建立；
- [x] README 已将 LD-001/002/003 从待验证改为已验证范围；
- [x] Python 回归、非法/越界命令回归、status 完整性和真实 fault snapshot 有快照分支已明确为后续项或延期项。

满足本小节时，可以认为：

```text
v2.1 load-disturbance evidence freeze 可以收口
```

但这不等于完整 v2.1 diagnostic release 已发布，也不授权自动创建 tag。

### 7.2 完整 v2.1 diagnostic release tag

- [ ] README、验证计划、报告和 release notes 的最终表述彼此一致；
- [ ] 已记录包含全部候选证据的最终 commit hash；
- [ ] 已记录 STM32CubeIDE、固件包、工具链版本和构建配置；
- [ ] 已记录 v2.1 最终候选 ELF/HEX/BIN 的文件名、大小和生成时间；
- [ ] 已计算 v2.1 最终候选构建产物 SHA-256；
- [ ] commit、构建产物和测试日志能够建立唯一对应关系；
- [ ] 所有阻断性异常已关闭，或决定不发布；
- [ ] 最终工作树状态已检查，待发布文件已由项目负责人确认；
- [ ] 若 release notes 声称包含 Python 串口半自动回归、非法/越界命令回归或 status 完整性专项检查，则必须补齐对应证据；否则必须在 release notes 中明确延期；
- [ ] 项目负责人已经人工审阅本检查表并授权后续 tag 操作。

上述必需项完成前，不应把 v2.1 描述为“诊断验证已完成”，也不应创建 tag。
