# QEMU i.MX RT1180 MCU 模拟与测试环境

[![CI Status](https://github.com/ZhuShaoYu/qemu-imxrt1180/actions/workflows/ci.yml/badge.svg)](https://github.com/ZhuShaoYu/qemu-imxrt1180/actions/workflows/ci.yml)

基于 QEMU 上游构建 NXP i.MX RT1180 (Cortex-M7 + Cortex-M33) MCU 模拟器及完整测试环境。

## 项目状态
🚧 Phase 1 — 以太网功能开发中

## Agent 团队
| Agent | 文件 | 职责 |
|-------|------|------|
| PM | `.github/agents/pm.agent.md` | 需求分析与 PRD |
| Architect | `.github/agents/architect.agent.md` | 系统架构设计 |
| QEMU Dev | `.github/agents/qemu-dev.agent.md` | QEMU 设备模型开发 |
| FW Dev | `.github/agents/fw-dev.agent.md` | 固件开发 |
| Test Eng | `.github/agents/test-eng.agent.md` | 测试工程师 |
| DevOps | `.github/agents/devops.agent.md` | CI/CD 与构建 |

## 快速开始
1. 在 VS Code 中打开此文件夹作为工作区
2. 切换到 **PM Agent** 开始需求对话
3. PM 输出 PRD → Architect 设计 → 开发 → 测试 → CI/CD

## 目录结构
```
├── .github/       # Agent 定义、Skills、CI 配置
├── docs/          # PRD、架构设计、接口文档
├── firmware/      # 测试固件源码
└── tests/         # qtest + Python 集成测试
```
