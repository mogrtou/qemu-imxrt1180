# QEMU Dev Agent 工作状态

> 最后更新: 2026-05-11 06:00

## 当前阶段
阶段：M1 ✅ 完成 — ENET+PHY 设备模型已交付
产出：enet.c(900+行/DMA/BD/TX/RX), dp83822_phy.c, soc.c, evk.c, 头文件, Meson, qtest
待办：P2 ECR/RCR 复位值不一致, QEMU+固件联调

## 已完成
- ✅ `hw/net/imxrt1180_enet.c` — 55个寄存器读写, MDIO Clause 22, DMA/BD 引擎, TX 路径(TDAR→BD扫描→qemu_send_packet), RX 路径(net后端→BD写入→RXF IRQ), 中断, W1C, ECR写保护
- ✅ `hw/net/imxrt1180_dp83822_phy.c` — 16个寄存器, MDIO R/W, BMCR复位/自协商, 链路始终Up
- ✅ `hw/arm/imxrt1180_soc.c` — armv7m + ITCM/DTCM/OCRAM + ENET1 IRQ 114
- ✅ `hw/arm/imxrt1180_evk.c` — Machine def `imxrt1180-evk`
- ✅ 头文件 + Meson 构建 + qtest 注册

## 待办 (M1 clean-up)
1. **P2**: 修复 ECR/RCR 复位值不一致 (ECR: 头文件 `0xF0000100` vs qtest `0xF0000000`, RCR: `0x05EE0001` vs `0x05E00001`)
2. QEMU+固件联调 — 验证 ENET TX/RX 端到端 (FW Dev M3 已完成, 固件已编译)
3. Phase 2: ENET2 + Switch 扩展
4. 获取 RT1180 参考手册后修正 NEARLY CORRECT 区域
