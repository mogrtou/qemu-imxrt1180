# 会话决策日志 (2026-05-10)

## 已确认决策
- [2026-05-10] [整体] PRD v1.0 已确认，范围：以太网功能 + lwIP 协议栈移植
- [2026-05-10] [分阶段] Phase 1 = 单 ENET MAC + DP83822 PHY；Phase 2 = 双 MAC + L2 Switch
- [2026-05-10] [技术选型] 协议栈：lwIP 2.x + mbedTLS + FreeRTOS (Cortex-M7)
- [2026-05-10] [QEMU] 网络后端：同时支持 TAP 和 SLIRP
- [2026-05-10] [固件] 板级抽象层设计，支持 QEMU / NXP EVK / 自定义板三目标
- [2026-05-10] [验证] 验收标准：Ping 可达 + HTTP Server 可访问
- [2026-05-10] [构建] 固件：Makefile + arm-none-eabi-gcc；QEMU：Meson 集成
- [2026-05-10] [测试] pytest 集成测试 + GitHub Actions CI/CD
- [2026-05-10] [基础设施] 为全部6个Agent创建了独立记忆文件
- [2026-05-10] [基础设施] 创建 project-status.md 作为跨Agent共享进度板
- [2026-05-10] [规则] 设定强制启动/收尾流程 (rules.md)
- [2026-05-10] [QEMU-Dev] ENET BD DMA 引擎实现完成：TX/RX 路径, MDIO PHY 接口
- [2026-05-10] [QEMU-Dev] 构建规则激活：hw/net/meson.build
- [2026-05-10] [QEMU-Dev] SoC 容器中 ENET1 实例化增加指针保存
- [2026-05-10] [Architect] 审查 M0-M6 全部产出，更新所有 agent memory + project-status.md
- [2026-05-10] [Architect] 发现复位值不一致：ECR `0xF0000100` vs `0xF0000000` — 标记 P2
- [2026-05-10] [FW] FW Dev 构建验证通过：0 warnings, 34.5KB, 工具链 arm-none-eabi-gcc 13.3 rel1
- [2026-05-11] [基础设施] 新建 Coordinator Agent，负责多 Agent 进度同步、阻塞项追踪、一致性审计
- [2026-05-11] [Coordinator] 权限：只读所有 agent memory，可写 project-status.md 和 coordinator.md
- [2026-05-11] [Coordinator] 命名定案 Coordinator（协调员），不走 Scrum Master 路线
- [2026-05-11] [Coordinator] 陈旧阈值：P0=6h, P1=12h, P2=24h（按小时级检测，非天数级）
- [2026-05-11] [规则] 新增「文件归属与越界禁止规则」到 rules.md — 每个工程 Agent 只能编辑自己的文件，跨域需等待
- [2026-05-11] [规则] 为 4 个工程 Agent (QEMU/FW/Test/DevOps) 添加 allowedPaths 白名单，硬限制编辑范围
- [2026-05-11] [Test Eng] 完成 qtest+pytest 静态验证, 修复 7/7 字符串匹配, ECR/RCR P2 ✅, RDSR RO ✅, M4 标记联调就绪
