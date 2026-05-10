# FW Dev Agent — 上下文恢复指南

> 最后更新: 2026-05-10 ✅ 编译链接全部通过

## 当前进度
- ✅ 基础固件 (config/link/startup/main) — 编译通过
- ✅ UART demo (LPUART1) — 编译通过
- ✅ SysTick demo (1s heartbeat, 10ms sub-tick) — 编译通过
- ✅ GPIO demo — 编译通过
- ✅ BAL 板级抽象层 — 编译通过
- ✅ FreeRTOS 10.6.2 — 8个核心文件编译链接通过
- ✅ lwIP 2.2.1 — def+mem+memp+pbuf+sys_arch 编译链接通过
- ✅ mbedTLS 3.6.2 — headers 就绪, 库未链接
- ⚠️ lwIP 完整协议栈 (TCP/UDP/DHCP/IPv4) — 待 Phase 2 逐步开启
- ❌ ENET 驱动 — 待 QEMU ENET 模型完成后实现

## 产物
`build/firmware.elf` — 153 KB (text=31,760, data=4, bss=125,164)

## 工具链
- 编译器: arm-none-eabi-gcc 15.2.1
- Path: `C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin`
- Flags: `-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv5-d16`
- 构建: `cd firmware && python build.py`

## 已知架构决策
- lwIP 协议层开关在 `lwipopts.h`: TCP=UDP=DHCP=DNS=0, IPV4=1
- OS 适配空实现在 `sys_arch.c` (bare-metal)
- 标准函数补充在 `startup.c`: memset, memcpy, memcmp, memmove, strlen, strncmp
- 临界区空宏在 `sys_arch.h`: SYS_ARCH_PROTECT/UNPROTECT/LOCKED

## 恢复后第一步
1. 读 `.github/memories/repo/project-status.md` 了解全局进度
2. 读 `docs/architecture.md` + `docs/interfaces.md` (如有更新)
3. 运行 `cd firmware && python build.py` 验证编译
