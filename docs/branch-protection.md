# 分支保护规则

> **文档状态**: 已生效
> **作者**: DevOps Agent
> **日期**: 2026-05-11

---

## 1. 分支策略

| 分支 | 用途 | 保护级别 |
|------|------|----------|
| `main` | 稳定主线，随时可构建 | **完全保护** |
| `feature/*` | 功能开发分支 | 无保护 |
| `fix/*` | 缺陷修复分支 | 无保护 |
| `docs/*` | 文档更新分支 | 无保护 |

---

## 2. `main` 分支保护规则

### 2.1 必须通过的 CI Check

| Check | Job 名称 | 说明 |
|-------|----------|------|
| ✅ Build QEMU | `build-qemu` | QEMU arm-softmmu 编译通过 |
| ✅ Build Firmware | `build-firmware` | 固件编译通过 |
| ✅ QTest Unit Tests | `test-qtest` | qtest 单元测试通过 |

> **注意**: `test-integration` 和 `coverage` job 设为 `continue-on-error: true`，
> 不作为 blocking check。它们的结果仅供参考，不会阻止 PR 合并。

### 2.2 GitHub 仓库设置 (Settings → Branches → Branch protection rules)

在 GitHub 仓库页面配置以下规则：

```
Branch name pattern: main

☑ Require a pull request before merging
  ☑ Require approvals: 1
  ☑ Dismiss stale pull request approvals when new commits are pushed
  
☑ Require status checks to pass before merging
  ☑ Require branches to be up to date before merging
  Status checks that are required:
    - Build QEMU (arm-softmmu)
    - Build Firmware
    - QTest Unit Tests

☐ Require conversation resolution before merging (可选)

☐ Require signed commits (可选)

☐ Require linear history (可选)

☑ Do not allow bypassing the above settings
  ☑ Include administrators
```

### 2.3 手动配置步骤

1. 打开 GitHub 仓库 → **Settings** → **Branches**
2. 点击 **Add branch protection rule**
3. 在 "Branch name pattern" 输入 `main`
4. 勾选以上列出的选项
5. 点击 **Create** / **Save changes**

---

## 3. PR 合并流程

```
 feature/my-work
       │
       ├── 1. 开发 & commit
       ├── 2. git push origin feature/my-work
       ├── 3. 创建 Pull Request → main
       ├── 4. CI 自动运行 build-qemu, build-firmware, test-qtest
       ├── 5. 至少 1 人 Code Review & Approve
       ├── 6. 所有 required checks 通过 (绿灯)
       └── 7. Squash & Merge → main
```

---

## 4. CI 失败处理

| 场景 | 处理方式 |
|------|----------|
| `build-qemu` 失败 | DevOps 负责修复，检查编译错误日志 |
| `build-firmware` 失败 | FW Dev 负责修复，检查固件编译错误 |
| `test-qtest` 失败 | QEMU Dev / Test Eng 分析日志，修复设备模型或测试用例 |
| `test-integration` 失败 | 标记 `::warning::` 不阻塞，相关 Agent 自行排查 |
| `coverage` 失败 | 标记 `::warning::` 不阻塞，仅 main 分支运行 |

---

## 5. 注意事项

- **管理员也不能绕过保护**: 确保 `Include administrators` 已勾选
- **ccache 缓存**: Cache miss 不会导致构建失败，仅影响构建速度
- **首次 PR**: 首次设置保护规则后，需要至少一次 PR 先合入 CI 配置，后续 PR 才能触发 checks
- **强制推送**: `main` 分支禁止 force push
- **删除分支**: `main` 分支禁止删除
