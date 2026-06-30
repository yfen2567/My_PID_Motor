# My_PID_Motor v2.1 Release 检查表

> 本文件只用于 release 前核对，不会自动创建 tag 或 release。

## 1. 计划与边界

- [x] 已建立 `docs/verification/v2.1_verification_plan.md`；
- [x] 已建立 `docs/verification/log_field_dictionary.md`；
- [x] 已建立 `docs/verification/load_disturbance_test_plan.md`；
- [x] 已建立本 Release checklist；
- [x] README 已包含 `Verification Status / 验证状态` 边界说明；
- [x] LD-001/002/003 负载扰动子范围已经冻结并形成汇总报告；
- [x] `v2.1-diagnostic-verification` 本地证据封版范围已经冻结；
- [x] 未执行项、延期项和不能外推的结论已在负载扰动汇总报告和最终 release notes 中明确。

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

- [x] 已记录构建输入 HEAD：`c373bda0347ac860d7513e8a5432673917bce21b`；
- [x] 已保存完整 Build Console：`reports/v2.1_build_console.txt`；
- [x] 已记录构建配置 `Debug` 和构建命令 `make -j24 all`；
- [x] Build Console 明确记录构建成功：`0 errors, 0 warnings`；
- [x] Debug Makefile 工具链标注 `GNU Tools for STM32 (13.3.rel1)`；
- [ ] STM32CubeIDE 与固件包的精确版本未在 Build Console 中记录；
- [x] 已记录 ELF/MAP/LIST 的文件名、大小、生成时间和 SHA-256；
- [x] HEX/BIN 未生成，已明确不纳入 v2.1；
- [x] LD-001/002/003 正式原始日志及证据文件已计算 SHA-256；
- [x] 最终封版 commit、构建产物和测试日志的对应关系已通过 `git show --stat v2.1-diagnostic-verification` 核对；tag 变更仅限证据文档，不涉及源码或工程配置。

## 5. README 与演示材料

- [x] README 已说明当前项目不是工业级电机控制器；
- [x] README 已说明 PID 参数不能外推到其他电机、负载或驱动板；
- [x] README 已说明测试不能证明长期可靠性或覆盖全部异常；
- [x] README 的“已验证/待验证”状态已按 LD-001/002/003 完成事实更新；
- [x] 已保留 `reports/v2.1_release_notes_draft.md` 草稿；
- [x] 已形成 `reports/v2.1_release_notes.md` 最终证据封版说明；
- [ ] 如录制演示视频，视频注明固件 commit、测试条件和对应日志（可延期）；
- [x] 演示视频当前明确标记为延期。

## 6. 可以延期的项目

以下项目可以延期，但必须在 release 说明中明确，不得暗示已经完成：

- 演示视频；
- Python 串口半自动回归；
- 非法命令 / 越界命令专项回归；
- run / stop 行为专项回归；
- status 字段完整性专项检查；
- 真实 fault snapshot 的有快照分支验证（前提是没有自然 fault，且不将其列为已验证能力）；
- 示波器/逻辑分析仪波形和电流测量；
- 日志可视化或自动生成图表；
- ADC target 的专项验证；
- HEX / BIN 发布包；
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

### 7.2 本地 v2.1 diagnostic evidence tag

- [x] README、验证计划、报告和最终 release notes 的表述彼此一致；
- [x] Build Console、Debug ELF/MAP/LIST 和 SHA-256 已记录；
- [x] HEX/BIN 未生成并明确排除，不冒充完整发布包；
- [x] Python、非法/越界/run-stop、status、真实 snapshot、视频、仪器测量和 ADC 专项均明确延期；
- [x] 当前没有未解释的阻断性安全异常；
- [x] 最终 diff 和工作树状态已在 commit 前检查，变更仅限允许的证据文档；
- [x] 项目负责人已在本任务中授权本地 commit 和本地 annotated tag；
- [x] 明确禁止自动 push、远程 tag 或 GitHub Release。

本地 tag 只标记诊断验证证据封版，不代表工业级验证，也不代表远程 release 已发布。
