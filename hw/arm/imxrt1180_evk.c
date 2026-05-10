/*
 * QEMU i.MX RT1180 Evaluation Kit (EVK) Machine
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * References:
 * - b-l475e-iot01a.c (Machine with SoC instantiation)
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/imxrt1180_soc.h"
#include "hw/boards.h"
#include "net/net.h"
#include "sysemu/sysemu.h"

/* ------------------------------------------------------------------ */
/*  Machine Init                                                        */
/* ------------------------------------------------------------------ */
static void imxrt1180_evk_init(MachineState *machine)
{
    IMXRT1180SoCState *soc;

    /* Create SoC as a child of the machine */
    soc = IMXRT1180_SOC(object_new(TYPE_IMXRT1180_SOC));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(soc));

    /* Set CPU type */
    if (machine->cpu_type) {
        object_property_set_str(OBJECT(soc), "cpu-type",
                                machine->cpu_type, &error_abort);
    }

    /* Set kernel (firmware) if provided */
    if (machine->kernel_filename) {
        object_property_set_str(OBJECT(soc), "kernel",
                                machine->kernel_filename, &error_abort);
    }

    /* Realize the SoC (creates CPU, memories, peripherals) */
    sysbus_realize(SYS_BUS_DEVICE(soc), &error_abort);

    /* ---- Connect ENET1 to network backend ---- */
    if (soc->enet1) {
        qdev_set_nic_properties(DEVICE(soc->enet1), &nd_table[0]);
    }
}

/* ------------------------------------------------------------------ */
/*  Machine Class Init                                                  */
/* ------------------------------------------------------------------ */
static void imxrt1180_evk_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "NXP i.MX RT1180 Evaluation Kit (armv7m)";
    mc->init = imxrt1180_evk_init;
    mc->default_cpu_type = "cortex-m7";
    mc->min_cpus = 1;
    mc->max_cpus = 1;
}

static const TypeInfo imxrt1180_evk_info = {
    .name = MACHINE_TYPE_NAME("imxrt1180-evk"),
    .parent = TYPE_MACHINE,
    .class_init = imxrt1180_evk_class_init,
};

static void imxrt1180_evk_register_types(void)
{
    type_register_static(&imxrt1180_evk_info);
}

type_init(imxrt1180_evk_register_types)
