/*
 * QEMU model of TI DP83822 10/100 Ethernet PHY
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Simplified behavioral model for Phase 1:
 * - Always reports Link Up
 * - Fixed 100M Full Duplex
 * - MDIO Clause 22 read/write
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/net/imxrt1180_dp83822_phy.h"

/* ------------------------------------------------------------------ */
/*  PHY Reset                                                           */
/* ------------------------------------------------------------------ */
void imxrt1180_dp83822_phy_reset(IMXRT1180DP83822PHYState *phy)
{
    memset(phy->regs, 0, sizeof(phy->regs));

    /* Standard IEEE registers */
    phy->regs[PHY_REG_BMCR]     = BMCR_RESET_VAL;
    phy->regs[PHY_REG_BMSR]     = BMSR_RESET_VAL;
    phy->regs[PHY_REG_PHYIDR1]  = DP83822_PHYID1;
    phy->regs[PHY_REG_PHYIDR2]  = DP83822_PHYID2;
    phy->regs[PHY_REG_ANAR]     = 0x0DE1;  /* 100FD, 100HD, 10FD, 10HD */
    phy->regs[PHY_REG_ANLPAR]   = 0x41E1;  /* Link partner: 100FD, 100HD,
                                               10FD, 10HD, ACK */
    phy->regs[PHY_REG_ANER]     = 0x0006;  /* No next page, ANEG valid */

    /* DP83822 extended registers */
    phy->regs[PHY_REG_PHYSTS]   = 0x0015;  /* Link up, 100M, full duplex */
    phy->regs[PHY_REG_MICR]     = 0x0000;
    phy->regs[PHY_REG_MISR]     = 0x0000;
    phy->regs[PHY_REG_RCSR]     = 0x0140;  /* RMII mode default */
    phy->regs[PHY_REG_LEDCR]    = 0x0440;
    phy->regs[PHY_REG_PHYCR]    = 0x0000;

    phy->link_up = true;
}

/* ------------------------------------------------------------------ */
/*  MDIO Read                                                           */
/* ------------------------------------------------------------------ */
uint16_t imxrt1180_dp83822_phy_mdio_read(IMXRT1180DP83822PHYState *phy,
                                         uint8_t reg_addr)
{
    if (reg_addr >= PHY_NUM_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "dp83822: MDIO read invalid register 0x%02x\n",
                      reg_addr);
        return 0xFFFF;
    }

    /* Auto-update BMSR link status: always up in Phase 1 */
    if (phy->link_up) {
        phy->regs[PHY_REG_BMSR] |= BMSR_LINK_STATUS;
    } else {
        phy->regs[PHY_REG_BMSR] &= ~BMSR_LINK_STATUS;
    }

    return phy->regs[reg_addr];
}

/* ------------------------------------------------------------------ */
/*  MDIO Write                                                          */
/* ------------------------------------------------------------------ */
void imxrt1180_dp83822_phy_mdio_write(IMXRT1180DP83822PHYState *phy,
                                      uint8_t reg_addr, uint16_t val)
{
    if (reg_addr >= PHY_NUM_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "dp83822: MDIO write invalid register 0x%02x = 0x%04x\n",
                      reg_addr, val);
        return;
    }

    switch (reg_addr) {
    case PHY_REG_BMCR:
        /* Handle reset */
        if (val & BMCR_RESET) {
            imxrt1180_dp83822_phy_reset(phy);
            /* Clear the reset bit after reset completes */
            phy->regs[PHY_REG_BMCR] = val & ~(uint16_t)BMCR_RESET;
            return;
        }
        /* Handle loopback: store loopback bit, no real effect */
        phy->regs[PHY_REG_BMCR] = val;
        break;

    case PHY_REG_BMSR:
        /* BMSR[3:0] are read-only per IEEE (some bits writable on DP83822) */
        /* For simplicity, ignore writes in Phase 1 */
        qemu_log_mask(LOG_GUEST_ERROR,
                      "dp83822: ignoring write to read-only BMSR (0x%04x)\n",
                      val);
        break;

    case PHY_REG_MICR:
        /* MII Interrupt Control */
        phy->regs[PHY_REG_MICR] = val;
        break;

    case PHY_REG_RCSR:
        /* RMII and Bypass Control */
        phy->regs[PHY_REG_RCSR] = val;
        break;

    case PHY_REG_PHYCR:
        phy->regs[PHY_REG_PHYCR] = val;
        break;

    case PHY_REG_LEDCR:
        phy->regs[PHY_REG_LEDCR] = val;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "dp83822: ignoring write to register 0x%02x = 0x%04x\n",
                      reg_addr, val);
        break;
    }
}
