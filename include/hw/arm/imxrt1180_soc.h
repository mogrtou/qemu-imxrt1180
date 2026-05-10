/*
 * QEMU i.MX RT1180 SoC Container
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_ARM_IMXRT1180_SOC_H
#define HW_ARM_IMXRT1180_SOC_H

#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "hw/net/imxrt1180_enet.h"
#include "hw/net/imxrt1180_dp83822_phy.h"

#define TYPE_IMXRT1180_SOC "imxrt1180-soc"
OBJECT_DECLARE_SIMPLE_TYPE(IMXRT1180SoCState, IMXRT1180_SOC)

/* ------------------------------------------------------------------ */
/*  Memory Map                                                          */
/* ------------------------------------------------------------------ */
#define IMXRT1180_ITCM_BASE         0x00000000
#define IMXRT1180_ITCM_SIZE         (256 * 1024)

#define IMXRT1180_DTCM_BASE         0x20000000
#define IMXRT1180_DTCM_SIZE         (256 * 1024)

#define IMXRT1180_OCRAM_BASE        0x20200000
#define IMXRT1180_OCRAM_SIZE        (512 * 1024)

#define IMXRT1180_PERIPHERAL_BASE   0x40000000
#define IMXRT1180_PERIPHERAL_SIZE   (2 * 1024 * 1024)

#define IMXRT1180_ENET1_BASE        0x40424000

/* ------------------------------------------------------------------ */
/*  Interrupt Numbers (NVIC)                                            */
/* ------------------------------------------------------------------ */
#define IMXRT1180_ENET1_IRQ         114
#define IMXRT1180_ENET_1588_IRQ     115
#define IMXRT1180_ENET2_IRQ         116 /* Phase 2 */

/* ------------------------------------------------------------------ */
/*  Number of external IRQ lines for armv7m_init                        */
/* ------------------------------------------------------------------ */
#define IMXRT1180_NUM_IRQS          200

/* ------------------------------------------------------------------ */
/*  SoC State                                                           */
/* ------------------------------------------------------------------ */
struct IMXRT1180SoCState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    /* ARMv7-M container (CPU + NVIC + SysTick + MPU) */
    ARMv7MState armv7m;

    /* Memory regions */
    MemoryRegion itcm;
    MemoryRegion dtcm;
    MemoryRegion ocram;

    /* Peripheral devices */
    IMXRT1180ENETState *enet1;

    /* Properties */
    char *cpu_type;
    char *kernel_filename;
};

#endif /* HW_ARM_IMXRT1180_SOC_H */
