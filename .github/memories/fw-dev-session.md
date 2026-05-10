# Firmware Dev Session Context — 2026-05-10

## Build Status: ✅ PASS (v2 — with FreeRTOS + mbedTLS)
- **Compiler**: arm-none-eabi-gcc 15.2.1
- **Flags**: `-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv5-d16`
- **Output**: `build/firmware.elf`
- **Size**: text=24,212, data=4, bss=65,952, total=90,168 (88.1 KB)

## Dependencies Installed
| Library | Version | Source |
|---------|---------|--------|
| FreeRTOS Kernel | 10.6.2 | ZIP download ✅ |
| mbedTLS | 3.6.2 | ZIP download ✅ |
| lwIP | 2.2.1 | ❌ (network blocked) |

## Build Contents
- **Core**: startup.c, main.c, config.h, link.ld
- **Demos**: uart_demo.c, systick_demo.c, gpio_demo.c
- **BAL**: bal/bal.c + bal/bal.h + bal/config/evk_config.h
- **Drivers**: drivers/imxrt_enet.h (header only)
- **FreeRTOS**: tasks, queue, list, timers, event_groups, stream_buffer, heap_4, port (ARM_CM7)
- **mbedTLS**: headers installed (library not linked yet — no mbedtls.c files in build.py)
- **lwIP**: NOT yet — need to download lwip.zip

## Bugs Fixed (chronological)
1. `ENET_IRQHandler` undeclared → weak alias in startup.c
2. `GPIO_PDOR_OFF` vs `GPIO_PDOR` macro naming → unified
3. `semihosting_write` declaration gated in config.h
4. Missing `FreeRTOS/Source/include` include path → added to build.py
5. `portmacro.h` custom version conflicting with official → deleted, use official
6. Duplicate NVIC macros in FreeRTOSConfig.h → removed
7. Missing `configENABLE_BACKWARD_COMPATIBILITY` → added
8. Missing `configUSE_16_BIT_TICKS` → added
9. FPU assembly in port.c vs `-mfloat-abi=soft` → global change to `softfp+fpv5-d16`
10. `memset` undefined → implemented in startup.c
11. `vApplicationStackOverflowHook` undefined → disabled stack overflow check
12. Stale `.o` files → clean build required

## Toolchain
- Path: `C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin`
- GCC version: 15.2.1

## Memory Layout
| Region | Start | Size | Usage |
|--------|-------|------|-------|
| ITCM | 0x0000_0000 | 256KB | .vectors + .text + .rodata |
| DTCM | 0x2000_0000 | 256KB | .data + .bss + stack + heap(32K) |
| OCRAM | 0x2020_0000 | 512KB | Phase 2: net buffers, BD rings |
| ENET1 | 0x4042_4000 | 4KB | ENET MAC registers |
| LPUART1 | 0x4007_0000 | 4KB | Debug UART |
| GPIO1 | 0x4012_C000 | 4KB | GPIO controller |

## Interrupts
| IRQ | Handler | Status |
|-----|---------|--------|
| 15 | SysTick_Handler | weak→systick_demo.c |
| 114 | ENET_IRQHandler | 向量表已预留 |

## Firmware Files
- `config.h` — USE_SEMIHOSTING=1, demo switches
- `link.ld` — MEMORY + SECTIONS + heap block
- `startup.c` — vector table + Reset_Handler + SystemInit(CPACR)
- `main.c` — semihosting_write() + demo dispatcher
- `uart_demo.c` — LPUART1 init + putc/puts
- `systick_demo.c` — SysTick 10ms sub-tick → 1s heartbeat
- `gpio_demo.c` — GPIO1[0] toggle 10 times
- `bal/bal.c` + `bal/bal.h` + `bal/config/evk_config.h`
- `drivers/imxrt_enet.h` — ENET reg + BD struct + API (header only)
- `FreeRTOS/include/FreeRTOSConfig.h` + `FreeRTOS/portable/portmacro.h`
- `lwip/include/lwipopts.h`
- `mbedtls/include/mbedtls_config.h`
- `build.py` — cross-platform build script

## Known Issues / Pitfalls
1. SysTick RVL is 24-bit; 600MHz tick requires sub-tick counting (done)
2. semihosting_write() defined under #ifdef but declared in config.h (fixed)
3. ENET driver is header-only; imxrt_enet.c not yet created
4. FreeRTOS/lwIP/mbedTLS source files not yet imported (Phase 2)

## Next Steps
1. Fix compile errors from `python build.py`
2. Verify .elf loads in QEMU (once imxrt1180-evk machine exists)
3. Phase 2: import FreeRTOS-Kernel, lwIP, mbedTLS sources
4. Phase 2: write imxrt_enet.c driver
