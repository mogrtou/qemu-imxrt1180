/*
 * QEMU model of i.MX RT1180 ENET Ethernet MAC Controller
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Reference: NXP i.MX RT1180 Reference Manual, ENET chapter
 *            NXP i.MX ENET/FEC common register layout
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/net/imxrt1180_enet.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/main-loop.h"
#include "exec/address-spaces.h"

/* ------------------------------------------------------------------ */
/*  Forward declarations                                                */
/* ------------------------------------------------------------------ */
static uint64_t imxrt1180_enet_read(void *opaque, hwaddr addr, unsigned size);
static void imxrt1180_enet_write(void *opaque, hwaddr addr, uint64_t val,
                                 unsigned size);
static void imxrt1180_enet_reset(DeviceState *dev);
static void imxrt1180_enet_realize(DeviceState *dev, Error **errp);
static void imxrt1180_enet_class_init(ObjectClass *oc, void *data);
static void imxrt1180_enet_instance_init(Object *obj);
static void imxrt1180_enet_update_irq(IMXRT1180ENETState *s);

/* ------------------------------------------------------------------ */
/*  MDIO Frame Handling (MMFR register)                                 */
/* ------------------------------------------------------------------ */
static void enet_handle_mdio_write(IMXRT1180ENETState *s)
{
    uint32_t mmfr = s->mmfr;
    uint8_t phy_addr = (mmfr >> 23) & 0x1F;
    uint8_t reg_addr = (mmfr >> 18) & 0x1F;
    uint8_t op = (mmfr >> 28) & 0x3;
    uint16_t data = mmfr & 0xFFFF;

    if (op == 0x1) { /* Write operation */
        imxrt1180_dp83822_phy_mdio_write(&s->phy, reg_addr, data);
    }

    /* Clear MII event, set MII complete */
    s->eir |= ENET_INT_MII;
    imxrt1180_enet_update_irq(s);
}

static void enet_handle_mdio_read(IMXRT1180ENETState *s)
{
    uint32_t mmfr = s->mmfr;
    uint8_t reg_addr = (mmfr >> 18) & 0x1F;
    uint16_t data;

    data = imxrt1180_dp83822_phy_mdio_read(&s->phy, reg_addr);

    /* Update MMFR DATA field with read result */
    s->mmfr = (mmfr & 0xFFFF0000) | data;

    /* Set MII complete */
    s->eir |= ENET_INT_MII;
    imxrt1180_enet_update_irq(s);
}

/* ------------------------------------------------------------------ */
/*  Interrupt Logic                                                     */
/* ------------------------------------------------------------------ */
static void imxrt1180_enet_update_irq(IMXRT1180ENETState *s)
{
    bool pending = (s->eir & s->eimr) != 0;
    qemu_set_irq(s->irq, pending ? 1 : 0);
}

/* ------------------------------------------------------------------ */
/*  Register Read Dispatch                                              */
/* ------------------------------------------------------------------ */
static uint64_t imxrt1180_enet_read(void *opaque, hwaddr addr, unsigned size)
{
    IMXRT1180ENETState *s = IMXRT1180_ENET(opaque);
    uint32_t val = 0;

    switch (addr) {
    case ENET_ECR:
        val = s->ecr;
        break;
    case ENET_EIR:
        val = s->eir;
        break;
    case ENET_EIMR:
        val = s->eimr;
        break;
    case ENET_RDAR:
        val = s->rdar;
        break;
    case ENET_TDAR:
        val = s->tdar;
        break;
    case ENET_ECR_MAGIC:
        val = s->ecr_magic;
        break;
    case ENET_MMFR:
        val = s->mmfr;
        break;
    case ENET_MSCR:
        val = s->mscr;
        break;
    case ENET_MIBC:
        val = s->mibc;
        break;
    case ENET_RCR:
        val = s->rcr;
        break;
    case ENET_TCR:
        val = s->tcr;
        break;
    case ENET_PALR:
        val = s->palr;
        break;
    case ENET_PAUR:
        val = s->paur;
        break;
    case ENET_OPD:
        val = s->opd;
        break;
    case ENET_IAUR:
        val = s->iaur;
        break;
    case ENET_IALR:
        val = s->ialr;
        break;
    case ENET_GAUR:
        val = s->gaur;
        break;
    case ENET_GALR:
        val = s->galr;
        break;
    case ENET_TFWR:
        val = s->tfwr;
        break;
    case ENET_RDSR:
        val = s->rdsr;
        break;
    case ENET_TDSR:
        val = s->tdsr;
        break;
    case ENET_MRBR:
        val = s->mrbr;
        break;
    case ENET_RSFL:
        val = s->rsfl;
        break;
    case ENET_RSEM:
        val = s->rsem;
        break;
    case ENET_RAEM:
        val = s->raem;
        break;
    case ENET_RAFL:
        val = s->rafl;
        break;
    case ENET_TSEM:
        val = s->tsem;
        break;
    case ENET_TAEM:
        val = s->taem;
        break;
    case ENET_TAFL:
        val = s->tafl;
        break;
    case ENET_TIPG:
        val = s->tipg;
        break;
    case ENET_ATCR:
    case ENET_ATVR:
    case ENET_ATOFF:
    case ENET_ATPER:
    case ENET_ATCOR:
    case ENET_ATINC:
    case ENET_ATSTMP:
    case ENET_TGSR:
    case ENET_TCSR0:
    case ENET_TCCR0:
    case ENET_TCSR1:
    case ENET_TCCR1:
    case ENET_TCSR2:
    case ENET_TCCR2:
    case ENET_TCSR3:
    case ENET_TCCR3:
        /* IEEE 1588 read-as-zero in Phase 1 */
        val = 0;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unimplemented register read: offset 0x%04" HWADDR_PRIx
                      " size %u\n", __func__, addr, size);
        val = 0;
        break;
    }

    return val;
}

/* ------------------------------------------------------------------ */
/*  Register Write Dispatch                                             */
/* ------------------------------------------------------------------ */
static void imxrt1180_enet_write(void *opaque, hwaddr addr, uint64_t val,
                                 unsigned size)
{
    IMXRT1180ENETState *s = IMXRT1180_ENET(opaque);

    switch (addr) {
    case ENET_ECR:
        /* ECR write-protect: only update if magic was unlocked.
         * After a successful write the magic is consumed. */
        if (s->ecr_magic == ENET_ECR_MAGIC_VAL) {
            s->ecr = val;
            s->ecr_magic = 0;

            /* Soft reset: clear ECR_RESET and restore default state */
            if (val & ENET_ECR_RESET) {
                s->ecr &= ~(uint32_t)ENET_ECR_RESET;
                /* Full reset via device_class_set_parent_reset handles
                 * register defaults; this only clears the trigger bit. */
            }
        }
        break;

    case ENET_EIR:
        /* EIR is Write-1-to-Clear (W1C) */
        s->eir &= ~(uint32_t)val;
        imxrt1180_enet_update_irq(s);
        break;

    case ENET_EIMR:
        s->eimr = val;
        imxrt1180_enet_update_irq(s);
        break;

    case ENET_RDAR:
        s->rdar = val;
        /* Writing 1 activates RX BD ring scanning */
        if (val & 1) {
            enet_handle_rx(s);
        }
        break;

    case ENET_TDAR:
        s->tdar = val;
        /* Writing 1 activates TX BD ring scanning */
        if (val & 1) {
            enet_handle_tx(s);
        }
        break;

    case ENET_ECR_MAGIC:
        s->ecr_magic = val;
        break;

    case ENET_MMFR:
    {
        uint8_t op = (val >> 28) & 0x3;

        s->mmfr = val;

        /* Dispatch based on MMFR OP field:
         *   01 = MDIO Write
         *   10 = MDIO Read
         */
        if (op == 0x1) {
            enet_handle_mdio_write(s);
        } else if (op == 0x2) {
            enet_handle_mdio_read(s);
        }
        break;
    }

    case ENET_MSCR:
        s->mscr = val;
        break;

    case ENET_MIBC:
        s->mibc = val;
        break;

    case ENET_RCR:
        s->rcr = val;
        break;

    case ENET_TCR:
        s->tcr = val;
        break;

    case ENET_PALR:
        s->palr = val;
        break;

    case ENET_PAUR:
        s->paur = val;
        break;

    case ENET_OPD:
        s->opd = val;
        break;

    case ENET_IAUR:
        s->iaur = val;
        break;

    case ENET_IALR:
        s->ialr = val;
        break;

    case ENET_GAUR:
        s->gaur = val;
        break;

    case ENET_GALR:
        s->galr = val;
        break;

    case ENET_TFWR:
        s->tfwr = val;
        break;

    case ENET_RDSR:
        /* RDSR is read-only: hardware sets this from the RX BD ring */
        qemu_log_mask(LOG_GUEST_ERROR,
                      "imxrt1180_enet: write to read-only RDSR\n");
        break;

    case ENET_TDSR:
        s->tdsr = val;
        break;

    case ENET_MRBR:
        s->mrbr = val;
        break;

    case ENET_RSFL:
        s->rsfl = val;
        break;

    case ENET_RSEM:
        s->rsem = val;
        break;

    case ENET_RAEM:
        s->raem = val;
        break;

    case ENET_RAFL:
        s->rafl = val;
        break;

    case ENET_TSEM:
        s->tsem = val;
        break;

    case ENET_TAEM:
        s->taem = val;
        break;

    case ENET_TAFL:
        s->tafl = val;
        break;

    case ENET_TIPG:
        s->tipg = val;
        break;

    /* IEEE 1588 registers: silently ignore in Phase 1 */
    case ENET_ATCR:
    case ENET_ATVR:
    case ENET_ATOFF:
    case ENET_ATPER:
    case ENET_ATCOR:
    case ENET_ATINC:
    case ENET_ATSTMP:
    case ENET_TGSR:
    case ENET_TCSR0:
    case ENET_TCCR0:
    case ENET_TCSR1:
    case ENET_TCCR1:
    case ENET_TCSR2:
    case ENET_TCCR2:
    case ENET_TCSR3:
    case ENET_TCCR3:
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unimplemented register write: offset 0x%04" HWADDR_PRIx
                      " size %u val 0x%08" PRIx64 "\n",
                      __func__, addr, size, val);
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Memory Region Ops                                                   */
/* ------------------------------------------------------------------ */
static const MemoryRegionOps imxrt1180_enet_ops = {
    .read = imxrt1180_enet_read,
    .write = imxrt1180_enet_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/* ------------------------------------------------------------------ */
/*  BD DMA Engine — Guest RAM access helpers                            */
/* ------------------------------------------------------------------ */

/**
 * enet_bd_read: Read a Buffer Descriptor from guest RAM.
 * @bd_ptr: physical address of the 8-byte BD
 * @bd:    pointer to local BD structure (output)
 *
 * BD layout (little-endian, 8 bytes packed):
 *   [15:0]  status  (R/E, W, L, etc.)
 *   [31:16] length  (data length)
 *   [63:32] data_ptr (buffer physical address)
 */
static void enet_bd_read(hwaddr bd_ptr, IMXRT1180ENETBD *bd)
{
    uint32_t words[2];

    cpu_physical_memory_read(bd_ptr, words, sizeof(words));

    /* BD is stored LE by hardware; on LE host this is native. */
    bd->status   = le16_to_cpu((uint16_t)(words[0] & 0xFFFF));
    bd->length   = le16_to_cpu((uint16_t)((words[0] >> 16) & 0xFFFF));
    bd->data_ptr = le32_to_cpu(words[1]);
}

/**
 * enet_bd_write: Write BD status/length fields back to guest RAM.
 * Preserves the data_ptr field (read-modify-write).
 */
static void enet_bd_write(hwaddr bd_ptr, const IMXRT1180ENETBD *bd)
{
    uint32_t words[2];

    /* Read current data_ptr */
    cpu_physical_memory_read(bd_ptr, words, sizeof(words));

    /* Update status and length */
    words[0] = (uint32_t)le16_to_cpu(bd->status)
             | ((uint32_t)le16_to_cpu(bd->length) << 16);

    /* Preserve data_ptr */
    words[1] = le32_to_cpu(bd->data_ptr);

    cpu_physical_memory_write(bd_ptr, words, sizeof(words));
}

/**
 * enet_bd_write_full: Write complete BD (status + length + data_ptr)
 */
static void enet_bd_write_full(hwaddr bd_ptr, const IMXRT1180ENETBD *bd)
{
    uint32_t words[2];

    words[0] = (uint32_t)le16_to_cpu(bd->status)
             | ((uint32_t)le16_to_cpu(bd->length) << 16);
    words[1] = le32_to_cpu(bd->data_ptr);

    cpu_physical_memory_write(bd_ptr, words, sizeof(words));
}

/**
 * enet_next_bd_index: Advance BD index, wrapping at ring boundary.
 * @curr:   current BD index
 * @bd:     the BD just processed (to check W bit)
 * @ring_size: total number of BDs in the ring
 * @return: next BD index
 */
static int32_t enet_next_bd_index(int32_t curr, const IMXRT1180ENETBD *bd,
                                   uint32_t ring_size)
{
    if (bd->status & ENET_BD_TX_W) {
        return 0;  /* Wrap to start */
    }
    return (curr + 1) % (int32_t)ring_size;
}

/* ------------------------------------------------------------------ */
/*  TX Path: BD Ring → QEMU Net Backend                                 */
/* ------------------------------------------------------------------ */

/**
 * enet_do_tx_bd: Process a single TX Buffer Descriptor.
 *
 * Reads the data buffer from guest RAM and sends it via qemu_send_packet.
 * Returns true if another BD should be processed.
 */
static bool enet_do_tx_bd(IMXRT1180ENETState *s)
{
    hwaddr bd_addr, buf_addr;
    IMXRT1180ENETBD bd;
    uint8_t tx_buf[2048];
    uint16_t length;

    if (!s->tdsr) {
        /* No TX ring base set */
        return false;
    }

    bd_addr = s->tdsr + (hwaddr)s->tx_curr * sizeof(IMXRT1180ENETBD);
    enet_bd_read(bd_addr, &bd);

    /* Check if BD is owned by ENET (Ready bit set by firmware) */
    if (!(bd.status & ENET_BD_TX_R)) {
        return false;  /* No more ready TX BDs */
    }

    length = bd.length;
    if (length > sizeof(tx_buf)) {
        length = sizeof(tx_buf);
    }

    buf_addr = bd.data_ptr;
    if (buf_addr && length > 0) {
        cpu_physical_memory_read(buf_addr, tx_buf, length);
    }

    /* Send to QEMU network backend (TAP / SLIRP) */
    if (s->nic && length > 0) {
        qemu_send_packet(qemu_get_queue(s->nic), tx_buf, length);
    }

    /* Update BD: clear Ready, preserve wrap/last bits */
    bd.status &= ~(uint16_t)ENET_BD_TX_R;

    enet_bd_write(bd_addr, &bd);

    /* Check for wrap */
    s->tx_curr = enet_next_bd_index(s->tx_curr, &bd, s->tx_ring_size);

    /* Set TXF interrupt */
    s->eir |= ENET_INT_TXF;
    imxrt1180_enet_update_irq(s);

    return true;
}

/**
 * enet_handle_tx: Process all pending TX Buffer Descriptors.
 * Called when firmware writes TDAR=1.
 */
static void enet_handle_tx(IMXRT1180ENETState *s)
{
    int max_iter = s->tx_ring_size + 1;

    while (max_iter-- > 0) {
        if (!enet_do_tx_bd(s)) {
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  RX Path: QEMU Net Backend → BD Ring → Guest RAM                     */
/* ------------------------------------------------------------------ */

/**
 * enet_can_receive_bd: Check if at least one RX BD is available (Empty).
 */
static bool enet_can_receive_bd(IMXRT1180ENETState *s)
{
    hwaddr bd_addr;
    IMXRT1180ENETBD bd;

    if (!s->rdsr || s->rx_ring_size == 0) {
        return false;
    }

    bd_addr = s->rdsr + (hwaddr)s->rx_curr * sizeof(IMXRT1180ENETBD);
    enet_bd_read(bd_addr, &bd);

    return (bd.status & ENET_BD_RX_E) != 0;
}

/**
 * enet_handle_rx: Check RX ring for phantom frames (Phase 1: no-op).
 * The actual RX path is driven by incoming packets from the net backend.
 * This function is called on RDAR to notify that the ring is active.
 */
static void enet_handle_rx(IMXRT1180ENETState *s)
{
    (void)s;  /* Phase 1: RX is backend-driven, no polling needed */
}

/* ------------------------------------------------------------------ */
/*  Network Backend Callbacks                                            */
/* ------------------------------------------------------------------ */
static int imxrt1180_enet_can_receive(NetClientState *nc)
{
    IMXRT1180ENETState *s = qemu_get_nic_opaque(nc);

    return enet_can_receive_bd(s) ? 1 : 0;
}

static ssize_t imxrt1180_enet_receive(NetClientState *nc, const uint8_t *buf,
                                      size_t size)
{
    IMXRT1180ENETState *s = qemu_get_nic_opaque(nc);
    hwaddr bd_addr, buf_addr;
    IMXRT1180ENETBD bd;
    size_t copy_len;

    if (!s->rdsr || s->rx_ring_size == 0) {
        return 0;
    }

    if (size > s->max_frame_size) {
        return 0;  /* Drop oversized frame */
    }

    bd_addr = s->rdsr + (hwaddr)s->rx_curr * sizeof(IMXRT1180ENETBD);
    enet_bd_read(bd_addr, &bd);

    /* BD must be empty (owned by ENET) */
    if (!(bd.status & ENET_BD_RX_E)) {
        /* No empty BD — set RXB interrupt, drop frame */
        s->eir |= ENET_INT_RXB;
        imxrt1180_enet_update_irq(s);
        return 0;
    }

    /* Copy frame data into guest buffer */
    copy_len = (size < s->mrbr) ? size : s->mrbr;
    buf_addr = bd.data_ptr;

    if (buf_addr && copy_len > 0) {
        cpu_physical_memory_write(buf_addr, buf, copy_len);
    }

    /* Update BD: clear Empty, set Last, write length */
    bd.status &= ~(uint16_t)ENET_BD_RX_E;
    bd.status |= ENET_BD_RX_L;
    bd.length = (uint16_t)size;

    enet_bd_write(bd_addr, &bd);

    /* Advance RX ring */
    s->rx_curr = enet_next_bd_index(s->rx_curr, &bd, s->rx_ring_size);

    /* Set RXF interrupt */
    s->eir |= ENET_INT_RXF;
    imxrt1180_enet_update_irq(s);

    return (ssize_t)size;
}

static void imxrt1180_enet_cleanup(NetClientState *nc)
{
    /* No-op */
}

static NetClientInfo net_imxrt1180_enet_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .can_receive = imxrt1180_enet_can_receive,
    .receive = imxrt1180_enet_receive,
    .cleanup = imxrt1180_enet_cleanup,
};

/* ------------------------------------------------------------------ */
/*  Device Reset                                                        */
/* ------------------------------------------------------------------ */
static void imxrt1180_enet_reset(DeviceState *dev)
{
    IMXRT1180ENETState *s = IMXRT1180_ENET(dev);

    s->ecr = ENET_ECR_RESET_VAL;
    s->eir = 0x00000000;
    s->eimr = 0x00000000;
    s->rdar = 0x00000000;
    s->tdar = 0x00000000;
    s->ecr_magic = 0x00000000;
    s->mmfr = 0x00000000;
    s->mscr = 0x00000040;
    s->mibc = 0xC0000000;
    s->rcr = ENET_RCR_RESET_VAL;
    s->tcr = ENET_TCR_RESET_VAL;
    s->palr = 0x00000000;
    s->paur = 0x00008808;
    s->opd = 0x00010001;
    s->iaur = 0x00000000;
    s->ialr = 0x00000000;
    s->gaur = 0x00000000;
    s->galr = 0x00000000;
    s->tfwr = 0x00000000;
    s->rdsr = 0x00000000;
    s->tdsr = 0x00000000;
    s->mrbr = 0x00000000;
    s->rsfl = 0x00000000;
    s->rsem = 0x00000000;
    s->raem = 0x00000004;
    s->rafl = 0x00000004;
    s->tsem = 0x00000060;
    s->taem = 0x00000008;
    s->tafl = 0x00000008;
    s->tipg = 0x0000000C;

    /* IEEE 1588 registers */
    s->atcr = 0x00000000;
    s->atvr = 0x00000000;
    s->atoff = 0x00000000;
    s->atper = 0x00000000;
    s->atcor = 0x00000000;
    s->atinc = 0x00000000;
    s->atstmp = 0x00000000;
    s->tgsr = 0x00000000;
    memset(s->tcsr, 0, sizeof(s->tcsr));
    memset(s->tccr, 0, sizeof(s->tccr));

    /* TX/RX ring */
    s->tx_curr = 0;
    s->rx_curr = 0;

    /* PHY */
    imxrt1180_dp83822_phy_reset(&s->phy);

    imxrt1180_enet_update_irq(s);
}

/* ------------------------------------------------------------------ */
/*  Device Realize                                                      */
/* ------------------------------------------------------------------ */
static void imxrt1180_enet_realize(DeviceState *dev, Error **errp)
{
    IMXRT1180ENETState *s = IMXRT1180_ENET(dev);

    /* Initialize MMIO region */
    memory_region_init_io(&s->iomem, OBJECT(dev), &imxrt1180_enet_ops, s,
                          TYPE_IMXRT1180_ENET, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    /* Initialize interrupt */
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);

    /* Initialize network backend */
    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    memcpy(s->macaddr, s->conf.macaddr.a, 6);

    s->nic = qemu_new_nic(&net_imxrt1180_enet_info, &s->conf,
                          object_get_typename(OBJECT(dev)),
                          dev->id, s);

    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->macaddr);
}

/* ------------------------------------------------------------------ */
/*  Properties                                                          */
/* ------------------------------------------------------------------ */
static Property imxrt1180_enet_properties[] = {
    DEFINE_PROP_UINT8("phy-addr", IMXRT1180ENETState, phy_addr, 0),
    DEFINE_PROP_UINT32("tx-ring-size", IMXRT1180ENETState, tx_ring_size, 8),
    DEFINE_PROP_UINT32("rx-ring-size", IMXRT1180ENETState, rx_ring_size, 8),
    DEFINE_PROP_UINT32("max-frame-size", IMXRT1180ENETState, max_frame_size,
                       1522),
    DEFINE_NIC_PROPERTIES(IMXRT1180ENETState, conf),
};

/* ------------------------------------------------------------------ */
/*  VMState Migration                                                   */
/* ------------------------------------------------------------------ */
static const VMStateDescription vmstate_imxrt1180_enet = {
    .name = TYPE_IMXRT1180_ENET,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(ecr, IMXRT1180ENETState),
        VMSTATE_UINT32(eir, IMXRT1180ENETState),
        VMSTATE_UINT32(eimr, IMXRT1180ENETState),
        VMSTATE_UINT32(rdar, IMXRT1180ENETState),
        VMSTATE_UINT32(tdar, IMXRT1180ENETState),
        VMSTATE_UINT32(ecr_magic, IMXRT1180ENETState),
        VMSTATE_UINT32(mmfr, IMXRT1180ENETState),
        VMSTATE_UINT32(mscr, IMXRT1180ENETState),
        VMSTATE_UINT32(mibc, IMXRT1180ENETState),
        VMSTATE_UINT32(rcr, IMXRT1180ENETState),
        VMSTATE_UINT32(tcr, IMXRT1180ENETState),
        VMSTATE_UINT32(palr, IMXRT1180ENETState),
        VMSTATE_UINT32(paur, IMXRT1180ENETState),
        VMSTATE_UINT32(opd, IMXRT1180ENETState),
        VMSTATE_UINT32(iaur, IMXRT1180ENETState),
        VMSTATE_UINT32(ialr, IMXRT1180ENETState),
        VMSTATE_UINT32(gaur, IMXRT1180ENETState),
        VMSTATE_UINT32(galr, IMXRT1180ENETState),
        VMSTATE_UINT32(tfwr, IMXRT1180ENETState),
        VMSTATE_UINT32(rdsr, IMXRT1180ENETState),
        VMSTATE_UINT32(tdsr, IMXRT1180ENETState),
        VMSTATE_UINT32(mrbr, IMXRT1180ENETState),
        VMSTATE_UINT32(rsfl, IMXRT1180ENETState),
        VMSTATE_UINT32(rsem, IMXRT1180ENETState),
        VMSTATE_UINT32(raem, IMXRT1180ENETState),
        VMSTATE_UINT32(rafl, IMXRT1180ENETState),
        VMSTATE_UINT32(tsem, IMXRT1180ENETState),
        VMSTATE_UINT32(taem, IMXRT1180ENETState),
        VMSTATE_UINT32(tafl, IMXRT1180ENETState),
        VMSTATE_UINT32(tipg, IMXRT1180ENETState),
        VMSTATE_INT32(tx_curr, IMXRT1180ENETState),
        VMSTATE_INT32(rx_curr, IMXRT1180ENETState),
        VMSTATE_END_OF_LIST()
    },
};

/* ------------------------------------------------------------------ */
/*  QOM Type Registration                                               */
/* ------------------------------------------------------------------ */
static void imxrt1180_enet_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = imxrt1180_enet_realize;
    dc->reset = imxrt1180_enet_reset;
    dc->vmsd = &vmstate_imxrt1180_enet;
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    device_class_set_props(dc, imxrt1180_enet_properties);
}

static void imxrt1180_enet_instance_init(Object *obj)
{
    /* Nothing to initialize at instance creation time */
}

static const TypeInfo imxrt1180_enet_info = {
    .name = TYPE_IMXRT1180_ENET,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IMXRT1180ENETState),
    .instance_init = imxrt1180_enet_instance_init,
    .class_init = imxrt1180_enet_class_init,
};

static void imxrt1180_enet_register_types(void)
{
    type_register_static(&imxrt1180_enet_info);
}

type_init(imxrt1180_enet_register_types)
