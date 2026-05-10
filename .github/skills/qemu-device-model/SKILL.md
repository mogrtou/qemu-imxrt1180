---
name: qemu-device-model
description: "Use when: implementing QEMU device models, writing QOM types, MemoryRegion callbacks, SysBus devices, interrupt controllers, or understanding QEMU's object model conventions."
---

# QEMU Device Model Development Skill

## QEMU Object Model (QOM) Quick Reference

### Type Definition Pattern
```c
/* In header: */
#define TYPE_IMXRT1180_UART "imxrt1180-uart"
OBJECT_DECLARE_SIMPLE_TYPE(IMXRT1180UARTState, IMXRT1180_UART)

/* In source: */
struct IMXRT1180UARTState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq;
    CharBackend chr;
    /* registers */
    uint32_t reg_baud;
    uint32_t reg_stat;
    uint32_t reg_ctrl;
    uint32_t reg_data;
};
```

### Class Init
```c
static void imxrt1180_uart_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->realize = imxrt1180_uart_realize;
    dc->reset = imxrt1180_uart_reset;
    device_class_set_props(dc, imxrt1180_uart_properties);
}
```

### MemoryRegionOps
```c
static const MemoryRegionOps imxrt1180_uart_ops = {
    .read = imxrt1180_uart_read,
    .write = imxrt1180_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};
```

### Device Realize
```c
static void imxrt1180_uart_realize(DeviceState *dev, Error **errp) {
    IMXRT1180UARTState *s = IMXRT1180_UART(dev);
    memory_region_init_io(&s->iomem, OBJECT(dev), &imxrt1180_uart_ops, s,
                          TYPE_IMXRT1180_UART, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);
    qemu_chr_fe_set_handlers(&s->chr, ...);
}
```

## NVIC Interrupt Numbers
- ARMv7-M NVIC is built into QEMU's `armv7m` container
- UART IRQ: typically 0-15 are system exceptions, so peripheral IRQs start at 16
- Use `armv7m_init()` which takes a board-specific `armv7m_init()` function
- Reference: `hw/arm/armv7m.c`

## Machine Model Pattern
```c
static void imxrt1180_evk_init(MachineState *machine) {
    DeviceState *soc = qdev_new(TYPE_IMXRT1180_SOC);
    object_property_add_child(OBJECT(machine), "soc", OBJECT(soc));
    sysbus_realize(SYS_BUS_DEVICE(soc), &error_fatal);
    /* Connect chardev from machine to UART in SoC */
}
```

## Useful References
- `hw/arm/stm32f205_soc.c` — Simple SoC with UART + SysTick
- `hw/arm/b-l475e-iot01a.c` — Machine with SoC and multiple peripherals
- `hw/char/stm32f2xx_usart.c` — UART device model
- `hw/arm/armv7m.c` — ARMv7-M container (provides NVIC, SysTick, CPU)
