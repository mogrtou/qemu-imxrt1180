---
name: imx-rt1180
description: "Use when: need i.MX RT1180 hardware specs, memory map, peripheral addresses, interrupt assignments, or chip-specific reference data."
---

# NXP i.MX RT1180 Reference Data

## Chip Overview
- **Family**: i.MX RT1180 Crossover MCU
- **Cores**: ARM Cortex-M7 (up to 800 MHz) + ARM Cortex-M33 (up to 240 MHz)
- **Target market**: Industrial IoT, motor control, edge computing

## Cortex-M7 Core
- Architecture: ARMv7E-M
- FPU: Single + double precision (FPv5)
- MPU: 16 regions
- TCM: ITCM + DTCM (sizes vary by variant)
- NVIC: up to 240 interrupts (peripheral IRQs start at 16)

## Memory Map (Typical — verify against specific EVK)
| Region | Start | Size | Description |
|--------|-------|------|-------------|
| ITCM | 0x00000000 | 256KB | Instruction TCM (also aliased at 0x60000000) |
| DTCM | 0x20000000 | 256KB | Data TCM |
| OCRAM | 0x20200000 | 512KB | On-Chip RAM |
| Peripheral | 0x40000000 | ~512KB | Peripheral register space |
| LPUART1 | 0x40070000 | 4KB | Low Power UART 1 |
| LPUART2 | 0x40074000 | 4KB | Low Power UART 2 |
| GPIO1 | 0x4012C000 | 4KB | GPIO Controller 1 |
| SysTick | ARM-internal | - | Part of Cortex-M7 core |

## LPUART Registers (simplified)
| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| 0x00 | VERID | 32 | Version ID |
| 0x04 | PARAM | 32 | Parameter |
| 0x0C | STAT | 32 | Status (TDRE, RDRF, etc.) |
| 0x10 | CTRL | 32 | Control (TE, RE, etc.) |
| 0x18 | DATA | 32 | Data register |
| 0x28 | BAUD | 32 | Baud rate |

## GPIO Registers (simplified)
| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| 0x00 | PDOR | 32 | Port Data Output |
| 0x04 | PSOR | 32 | Port Set Output |
| 0x08 | PCOR | 32 | Port Clear Output |
| 0x0C | PTOR | 32 | Port Toggle Output |
| 0x10 | PDIR | 32 | Port Data Input |
| 0x14 | PDDR | 32 | Port Data Direction |

## ARM Semihosting
- Trap instruction: `BKPT 0xAB` on ARM
- Operations: WRITE (0x05), READ (0x06), etc.
- QEMU support: `-semihosting` flag enables ARM semihosting
- QEMU captures semihosting output to stderr or file

## Note
The exact memory map and register layout depends on the specific RT1180 variant and EVK board. The Architect agent MUST verify these addresses against the official NXP Reference Manual and document the final values in `docs/interfaces.md`. This skill file provides starting-point estimates only.
