# ADR-006: 板级抽象层采用编译期配置

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

固件需要同时支持 QEMU 模拟和多种真实硬件板卡（NXP EVK + 用户自定义板）。板间差异包括：内存布局、外设基地址、PHY 地址、pinmux、时钟频率等。

有几种抽象方式：

| 方式 | 优点 | 缺点 |
|------|------|------|
| 编译期配置 (`#define`) | 零运行时开销 | 不同板需重新编译 |
| 运行时检测 (Device Tree) | 同一二进制多板 | RAM/Flash 开销大 |
| 动态加载配置 | 灵活 | 实现复杂 |

## Decision

**采用编译期配置 (`#include` 板级头文件) + Makefile `BOARD=` 变量切换。**

配置文件位于 `firmware/bal/config/<board>_config.h`，通过 `-include` 或 `#include` 引入。

### Board 选择流程
```
make BOARD=evk
    → CFLAGS += -DBOARD_CONFIG=\"evk_config.h\"
    → bal.h 中 #include BOARD_CONFIG
```

## Consequences

### 正面
- ✅ 零 RAM/Flash 开销 — 条件编译消除未用代码
- ✅ 编译时即可发现板级配置错误
- ✅ 固件二进制针对特定板优化
- ✅ Git 友好 — 板配置文件是纯头文件，diff 清晰

### 负面
- ⚠️ 不同板需不同编译产物（但这是 MCU 固件常态）
- ⚠️ 添加新板需新增头文件（Phase 1 仅两个板：evk / custom 模板）

### 配置项清单
| 配置项 | 类型 | 说明 |
|--------|------|------|
| `ENET_BASE_ADDR` | `uint32_t` | ENET 寄存器基地址 |
| `ENET_IRQ_N` | `int` | NVIC 中断号 |
| `ENET_PHY_ADDR` | `uint8_t` | MDIO PHY 地址 |
| `PHY_TYPE` | `string` | PHY 型号标识 |
| `ENET_CLOCK_HZ` | `uint32_t` | ENET 外设时钟 |
| `CORE_CLOCK_HZ` | `uint32_t` | 系统核心时钟 |
| `OCRAM_BASE` / `SIZE` | `uint32_t` | 共享 RAM 位置 |
| `DEBUG_USE_SEMIHOSTING` | `bool` | 调试通道选择 |
