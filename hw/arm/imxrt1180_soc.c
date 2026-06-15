/*
 * QEMU i.MX RT1180 SoC Container
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * References:
 * - QEMU armv7m container (hw/arm/armv7m.c)
 * - STM32 SoC models (hw/arm/stm32f205_soc.c)
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/arm/imxrt1180_soc.h"
#include "hw/qdev-properties.h"
#include "target/arm/cpu-qom.h"
#include "hw/arm/boot.h"
#include "hw/qdev-clock.h"

/* ------------------------------------------------------------------ */
/*  Forward declarations                                                */
/* ------------------------------------------------------------------ */
static void imxrt1180_soc_realize(DeviceState *dev, Error **errp);
static void imxrt1180_soc_class_init(ObjectClass *oc, void *data);
static void imxrt1180_soc_instance_init(Object *obj);

/* ------------------------------------------------------------------ */
/*  SoC Device Realize                                                  */
/* ------------------------------------------------------------------ */
static void imxrt1180_soc_realize(DeviceState *dev, Error **errp)
{
    IMXRT1180SoCState *s = IMXRT1180_SOC(dev);
    DeviceState *armv7m_dev = DEVICE(&s->armv7m);
    SysBusDevice *sbd;
    Object *obj;

    /* SoC realize */

    /* ---- 1. Initialize ARMv7-M Core ---- */
    object_property_set_link(OBJECT(&s->armv7m), "memory",
                             OBJECT(get_system_memory()), &error_abort);
    object_property_set_str(OBJECT(&s->armv7m), "cpu-type", s->cpu_type,
                            &error_abort);
    object_property_set_int(OBJECT(&s->armv7m), "num-irq", IMXRT1180_NUM_IRQS,
                            &error_abort);
    qdev_connect_clock_in(DEVICE(&s->armv7m), "cpuclk", s->sysclk);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), errp)) {
        return;
    }

    /* Load kernel after ARMv7M realized (v9.2.0: no "kernel" property) */
    if (s->kernel_filename) {
        armv7m_load_kernel(ARM_CPU(first_cpu), s->kernel_filename, 0,
                           IMXRT1180_ITCM_SIZE);
    }

    /* ---- 2. Create Memory Regions ---- */
    /* ITCM: 0x0000_0000, 256KB */
    memory_region_init_ram(&s->itcm, OBJECT(dev), "imxrt1180.itcm",
                           IMXRT1180_ITCM_SIZE, &error_abort);
    memory_region_add_subregion(get_system_memory(),
                                IMXRT1180_ITCM_BASE, &s->itcm);

    /* DTCM: 0x2000_0000, 256KB */
    memory_region_init_ram(&s->dtcm, OBJECT(dev), "imxrt1180.dtcm",
                           IMXRT1180_DTCM_SIZE, &error_abort);
    memory_region_add_subregion(get_system_memory(),
                                IMXRT1180_DTCM_BASE, &s->dtcm);

    /* OCRAM: 0x2020_0000, 512KB */
    memory_region_init_ram(&s->ocram, OBJECT(dev), "imxrt1180.ocram",
                           IMXRT1180_OCRAM_SIZE, &error_abort);
    memory_region_add_subregion(get_system_memory(),
                                IMXRT1180_OCRAM_BASE, &s->ocram);

    /* ---- 3. Create ENET1 ---- */
    obj = object_new(TYPE_IMXRT1180_ENET);
    object_property_set_uint(obj, "phy-addr", 0, &error_abort);
    object_property_add_child(OBJECT(dev), "enet1", obj);
    sysbus_realize(SYS_BUS_DEVICE(obj), &error_abort);

    sbd = SYS_BUS_DEVICE(obj);
    memory_region_add_subregion(get_system_memory(),
                                IMXRT1180_ENET1_BASE,
                                sysbus_mmio_get_region(sbd, 0));

    sysbus_connect_irq(sbd, 0,
                       qdev_get_gpio_in(armv7m_dev, IMXRT1180_ENET1_IRQ));

    s->enet1 = IMXRT1180_ENET(obj);
    /* ---- 4. Phase 2: ENET2 at IRQ 116, base TBD ---- */
}

/* ------------------------------------------------------------------ */
/*  Properties                                                          */
/* ------------------------------------------------------------------ */
static Property imxrt1180_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", IMXRT1180SoCState, cpu_type),
    DEFINE_PROP_STRING("kernel", IMXRT1180SoCState, kernel_filename),
};

/* ------------------------------------------------------------------ */
/*  QOM Type Registration                                               */
/* ------------------------------------------------------------------ */
static void imxrt1180_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = imxrt1180_soc_realize;
    device_class_set_props(dc, imxrt1180_soc_properties);
}

static void imxrt1180_soc_instance_init(Object *obj)
{
    IMXRT1180SoCState *s = IMXRT1180_SOC(obj);

    /* Initialize embedded objects */
    object_initialize_child(obj, "armv7m", &s->armv7m, TYPE_ARMV7M);

    /* Create system clock */
    s->sysclk = clock_new(obj, "sysclk");
    clock_set_hz(s->sysclk, 600000000); /* 600 MHz for i.MX RT1180 */

    /* Default CPU type */
    s->cpu_type = g_strdup(ARM_CPU_TYPE_NAME("cortex-m7"));
    s->kernel_filename = NULL;
}

static const TypeInfo imxrt1180_soc_info = {
    .name = TYPE_IMXRT1180_SOC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IMXRT1180SoCState),
    .instance_init = imxrt1180_soc_instance_init,
    .class_init = imxrt1180_soc_class_init,
};

static void imxrt1180_soc_register_types(void)
{
    type_register_static(&imxrt1180_soc_info);
}

type_init(imxrt1180_soc_register_types)
