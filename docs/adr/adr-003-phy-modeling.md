# ADR-003: DP83822 PHY 独立建模 + MDIO 总线

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

DP83822 是以太网 PHY 芯片，通过 MDIO 总线与 ENET MAC 通信。在 QEMU 中建模 PHY 有三种方案：

- **方案 A**: 将 PHY 寄存器嵌入 ENET 寄存器空间（合并为一个设备）
- **方案 B**: PHY 作为独立对象，ENET 通过内部接口调用
- **方案 C**: PHY 作为独立设备，通过 QEMU MDIO bus 框架连接

## Decision

**选择方案 B — PHY 作为独立的 QOM 对象 (`TYPE_IMXRT1180_DP83822_PHY`)，嵌入在 ENET 对象中。** 二者通过内部函数调用接口通信（`mdio_read` / `mdio_write`），不引入完整的 QEMU MDIO bus 框架。

## Rationale

- QEMU 当前无成熟的 MDIO bus 框架（不同于 PCI/USB/I2C）
- Phase 1 仅有 1 个 PHY，引入通用 MDIO bus 过早抽象
- 内部函数调用性能最优，寄存器读写无额外开销
- Phase 2 需要多 PHY 时，可重构为通用 MDIO bus（预留扩展点）

## Consequences

### 正面
- ✅ 实现简单，PHY 与 ENET 紧密耦合符合物理现实
- ✅ qtest 可测试 ENET+PHY 联调行为
- ✅ 配置简便（`phy-addr` 属性）

### 负面
- ⚠️ Phase 2 多 PHY (ENET1+ENET2+Switch) 时需重构
- ⚠️ 若未来需要其他 PHY 型号，需额外的抽象层

### Phase 2 扩展预留
- ENET 结构中保留 `PHY *phy_link` 指针（当前为单指针，可改为数组）
- `mdio_read/write` 签名包含 `phy_addr` 参数，预留多 PHY 寻址
