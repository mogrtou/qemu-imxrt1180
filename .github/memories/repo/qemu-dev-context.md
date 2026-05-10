# QEMU Dev Agent — 上下文恢复指南

> 最后一次活跃: 2026-05-10

## 当前进度
- ✅ QEMU 源码已 clone 到 `qemu/` 目录
- ❌ hw/net/imxrt1180_enet.c — 设备模型代码、meson.build 尚未完成
- ❌ hw/net/imxrt1180_dp83822_phy.c — 未写
- ❌ hw/arm/imxrt1180_soc.c — SoC 容器未实现
- ❌ hw/arm/imxrt1180_evk.c — 机器定义未实现
- ❌ 测试用例 (tests/) — Test Eng 负责
- ❌ Kconfig 集成 — DevOps 负责

## 恢复后第一步
阅读 `docs/architecture.md` 和 `docs/interfaces.md`，
特别是:
- ENET 寄存器映射 (interfaces.md §1.1)
- 中断路由 (interfaces.md §4, architecture.md §4)
- 内存映射 (architecture.md §3)
- Buffer Descriptor 结构 (interfaces.md §3.2)
