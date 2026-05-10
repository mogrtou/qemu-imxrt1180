# Agent 行为规则

## 强制启动流程（所有 Agent 必须遵守）
每轮对话开始时，agent 必须按顺序执行：
1. 读取 `.github/memories/project-status.md` — 了解项目整体进度和阻塞项
2. 读取 `.github/memories/{自己的agent名}.md` — 了解自己模块的进度和约定
3. 读取 `.github/memories/decision-log.md`（如果存在）— 当前会话已做的决策

## 强制收尾流程（所有 Agent 必须遵守）
每轮对话结束前，agent 必须按顺序执行以下步骤。**这是硬性要求，不可跳过。**

1. **更新自己的 memory** (`.github/memories/{agent-name}.md`)：
   - 时间戳：文件顶部 `> 最后更新: YYYY-MM-DD HH:MM`
   - 写入本轮产出 (完成的功能 / 修复的 bug / 创建的文档)
   - 更新进度 (从未完成→已完成，或百分比)
   - 记录新发现的坑 / 注意事项 / 技术债

2. **更新 `project-status.md`** (如果里程碑状态或阻塞项有变化)：
   - 新增的阻塞项立即登记
   - 已解决的阻塞项标记 ✅ 或移除
   - 里程碑状态变化同步更新

3. **追加 `decision-log.md`** (如果有重要决策)：
   - 格式: `[YYYY-MM-DD] [模块] 决策摘要`
   - 1-3 行为宜

4. **通知 coordinator** (如果解决了阻塞项或完成了里程碑)：
   - 在 `project-status.md` 的 "下一步交接" 中写清下游 agent 需做什么

> 🚨 **未收尾的后果**：Coordinator 巡查时发现 agent memory 滞后 → 标记为 🟡 休眠 → P0/P1 阻塞项可能被错误升级 → 通知用户介入

## 关键决策记录
- 每次完成一个关键决策，主动将决策摘要写入 `.github/memories/decision-log.md`。
- 格式：`[日期] [模块] 决策内容摘要`，保持简洁，每条 1-3 行。

## Agent 间交接
- 交接时，上游 agent 必须在 `project-status.md` 中明确写清：
  - 当前完成的里程碑
  - 下游 agent 需要接手的具体任务
  - 需要阅读的文档列表

---

## 文件归属与越界禁止规则（所有 Agent 必须遵守）

### 原则
每个工程 Agent 拥有**排他的文件修改权**。只有文件归属 Agent 可以编辑其负责的文件。
当遇到涉及其他 Agent 文件的改动时，必须**等待**该 Agent 完成，或通过 `project-status.md` 的阻塞项机制请求协作，**严禁越界接管**。

### 文件归属清单

| Agent | 独占编辑路径 |
|-------|------------|
| **QEMU Dev** | `hw/arm/*`, `hw/net/*`, `hw/char/*`, `hw/gpio/*`, `include/hw/arm/*`, `include/hw/net/*`, `include/hw/char/*`, `include/hw/gpio/*` |
| **FW Dev** | `firmware/**`（全部固件源码、驱动、协议栈、BAL、链接脚本） |
| **Test Eng** | `tests/qtest/*`, `tests/integration/*`, `tests/standalone/*` |
| **DevOps** | `Dockerfile`, `docker-compose.yml`, `.github/workflows/*`, `qemu/**/meson.build`, `firmware/Makefile`, `firmware/build.py`, `tests/**/meson.build` |
| **Architect** | `docs/architecture.md`, `docs/interfaces.md`, `docs/adr/*` |
| **PM** | `docs/prd.md` |
| **Coordinator** | `.github/memories/project-status.md`, `.github/memories/coordinator.md` |

### 共享（只读）
- `.github/memories/` 下其他 Agent 的 memory 文件对所有 Agent 只读
- `docs/prd.md`, `docs/architecture.md`, `docs/interfaces.md`, `docs/adr/*` 对所有 Agent 只读

### 越界处理流程
1. Agent A 发现需要修改 Agent B 的文件（如 QEMU Dev 需要固件配合改地址）
2. Agent A **在 `project-status.md` 添加阻塞项**，写明：需要 B 做什么、优先级、截止时间
3. Agent A **等待** B 完成，或通知用户手动切换到 Agent B
4. **严禁** Agent A 直接修改 Agent B 的文件
5. Coordinator 巡查时发现越界修改 → 标记为冲突，通知用户回滚
