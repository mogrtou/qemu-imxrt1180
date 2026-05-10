# ADR-001: 使用 QEMU 上游 armv7m 容器模拟 Cortex-M7

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

项目需要模拟 i.MX RT1180 的 Cortex-M7 内核。QEMU 上游已提供 `armv7m` 容器 (`hw/arm/armv7m.c`)，该容器已集成：
- ARMv7-M NVIC 中断控制器
- SysTick 系统定时器
- MPU
- 完整的 ARMv7E-M 指令集 (含 FPv5 FPU)

## Decision

**直接使用 QEMU 上游 `armv7m` 容器，通过 `armv7m_init()` 初始化。**

不自行实现 Cortex-M7 CPU 模型。通过 `-cpu cortex-m7` 参数选择 CPU 类型。

## Consequences

### 正面
- ✅ 零开发成本 — CPU/NVIC/SysTick 均开箱即用
- ✅ 跟随 QEMU 上游 Bug 修复和指令增强
- ✅ 与其他 ARM MCU 机器（STM32 等）共享同样的基础设施
- ✅ Semihosting 支持内置 (`-semihosting` 标志)

### 负面
- ⚠️ 某些 RT1180 特定的 CPU 行为可能未建模（如特定勘误表）
- ⚠️ 双核 (Cortex-M33 协同处理) 在 Phase 1 不涉及，Phase 2 需评估 armv8m 容器

### 约束
- 机器代码中调用 `armv7m_init()` 时需指定内存区域:
  ```c
  armv7m_init(get_system_memory(), MEMORY_REGION_SIZE,
              NUM_IRQS, kernel_filename, cpu_type);
  ```
