# ADR-002: ENET 控制器作为独立 SysBusDevice

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

需要在 QEMU 中建模 i.MX RT1180 的 ENET 以太网控制器。有两种架构选择：

- **方案 A**: 将 ENET 作为 SoC 对象内联的 MemoryRegion（与 STM32 某些简单外设类似）
- **方案 B**: 将 ENET 建模为独立的 `SysBusDevice` QOM 类型

## Decision

**选择方案 B — ENET 作为独立的 `TYPE_IMXRT1180_ENET` SysBusDevice。**

ENET 是一个复杂外设（~200+ 寄存器、DMA、BD 环、MDIO），需独立封装以提高可维护性和可测试性。

## Consequences

### 正面
- ✅ 可独立进行 qtest 单元测试（无需加载完整 SoC）
- ✅ 代码隔离清晰，便于 QEMU Dev 和 FW Dev 独立开发
- ✅ Phase 2 添加 ENET2 时可直接实例化第二个对象
- ✅ 符合 QEMU 上游惯例（`hw/net/imx_fec.c` 也是独立设备）

### 负面
- ⚠️ 增加了 QOM 类型注册代码（但很少）

### 实现约束
- 父类型: `TYPE_SYS_BUS_DEVICE`
- MemoryRegion 大小: `0x1000` (4KB)
- IRQ: 1 条输出线 → SoC 连接到 NVIC IRQ 114
- MDIO: 通过设备内部 link 属性连接到 PHY 对象
- NIC: 通过 `NICConf` 集成到 QEMU net 子系统
