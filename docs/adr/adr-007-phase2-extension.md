# ADR-007: Phase 2 双 MAC 扩展预留设计

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

PRD 明确要求 Phase 2 支持双 MAC (ENET1 + ENET2) + 片上 L2 Switch。Phase 1 架构需预留扩展点，避免 Phase 2 重构。

## Decision

在 Phase 1 架构中预留以下扩展点，但不实现：

| # | 扩展点 | 具体操作 |
|---|--------|----------|
| 1 | **SoC 级** | `imxrt1180_soc_init()` 中 ENET 实例化使用循环或逐个调用，不硬编码单实例 |
| 2 | **中断** | ENET2 IRQ (116) 在 SoC 初始化中预留槽位，当前注释即可 |
| 3 | **MDIO** | `mdio_read/write(phy_addr, ...)` 已携带 `phy_addr` 参数，支持多 PHY |
| 4 | **固件 netif** | `imxrt_enet_init()` 接受 `netif` 参数，可多次调用于多接口 |
| 5 | **BAL 配置** | 配置头文件中使用 `ENET1_*` / `ENET2_*` 前缀，保留命名空间 |
| 6 | **QOM** | `imxrt1180-enet` 类型无硬编码实例限制 |

## Consequences

### 正面
- ✅ Phase 2 无需重构架构和接口契约
- ✅ 实现成本极低（仅命名和结构上的预留）
- ✅ 测试代码可提前用 "ENET1" 前缀编写，保持一致性

### 约束
- Phase 2 开始前需要 NXP 参考手册确认 ENET2 基地址和 IRQ 编号
- Switch 对象的接口设计需在未来单独的 ADR 中定义

### Phase 2 不兼容风险
- 若 NXP RT1180 的 ENET1/ENET2 寄存器布局有显著差异（目前假设相同），需在设备实例化时指定变体属性
