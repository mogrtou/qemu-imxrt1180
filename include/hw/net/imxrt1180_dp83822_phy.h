/*
 * QEMU model of TI DP83822 10/100 Ethernet PHY
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_NET_IMXRT1180_DP83822_PHY_H
#define HW_NET_IMXRT1180_DP83822_PHY_H

#include "qom/object.h"

#define TYPE_IMXRT1180_DP83822_PHY "imxrt1180-dp83822-phy"

/* ------------------------------------------------------------------ */
/*  Standard PHY Registers (IEEE 802.3 Clause 22)                      */
/* ------------------------------------------------------------------ */
#define PHY_REG_BMCR        0x00    /* Basic Mode Control               */
#define PHY_REG_BMSR        0x01    /* Basic Mode Status                */
#define PHY_REG_PHYIDR1     0x02    /* PHY Identifier 1                 */
#define PHY_REG_PHYIDR2     0x03    /* PHY Identifier 2                 */
#define PHY_REG_ANAR        0x04    /* Auto-Negotiation Advertisement   */
#define PHY_REG_ANLPAR      0x05    /* Auto-Neg Link Partner Ability    */
#define PHY_REG_ANER        0x06    /* Auto-Neg Expansion               */
#define PHY_REG_ANNPTR      0x07    /* Auto-Neg Next Page Tx            */

/* DP83822 Extended Registers */
#define PHY_REG_PHYSTS      0x10    /* PHY Status Register              */
#define PHY_REG_MICR        0x11    /* MII Interrupt Control            */
#define PHY_REG_MISR        0x12    /* MII Interrupt Status             */
#define PHY_REG_FCSCR       0x14    /* False Carrier Sense Counter      */
#define PHY_REG_RECR        0x15    /* RMII and Bypass Control          */
#define PHY_REG_RCSR        0x17    /* RMII and Bypass Control          */
#define PHY_REG_LEDCR       0x19    /* LED Control                      */
#define PHY_REG_PHYCR       0x1F    /* PHY Control (extended)           */

#define PHY_NUM_REGS        32

/* ------------------------------------------------------------------ */
/*  BMCR Bit Definitions                                                */
/* ------------------------------------------------------------------ */
#define BMCR_RESET          (1u << 15)
#define BMCR_LOOPBACK       (1u << 14)
#define BMCR_SPEED100       (1u << 13)
#define BMCR_ANEG_EN        (1u << 12)
#define BMCR_PWR_DOWN       (1u << 11)
#define BMCR_ISOLATE        (1u << 10)
#define BMCR_ANEG_RESTART   (1u << 9)
#define BMCR_DUPLEX         (1u << 8)

/* BMCR reset value: 100M, ANEG enabled, full duplex */
#define BMCR_RESET_VAL      (BMCR_ANEG_EN | BMCR_SPEED100 | BMCR_DUPLEX)

/* ------------------------------------------------------------------ */
/*  BMSR Bit Definitions                                                */
/* ------------------------------------------------------------------ */
#define BMSR_100BASE_T4     (1u << 15)
#define BMSR_100BASE_TX_FD  (1u << 14)
#define BMSR_100BASE_TX_HD  (1u << 13)
#define BMSR_10BASE_T_FD    (1u << 12)
#define BMSR_10BASE_T_HD    (1u << 11)
#define BMSR_ANEG_COMPLETE  (1u << 5)
#define BMSR_LINK_STATUS    (1u << 2)
#define BMSR_EXTENDED       (1u << 0)

/* BMSR reset value: link up, 100M FD/HD, 10M FD/HD, extended regs */
#define BMSR_RESET_VAL      (BMSR_100BASE_TX_FD | BMSR_100BASE_TX_HD | \
                             BMSR_10BASE_T_FD | BMSR_10BASE_T_HD | \
                             BMSR_ANEG_COMPLETE | BMSR_LINK_STATUS | \
                             BMSR_EXTENDED)

/* ------------------------------------------------------------------ */
/*  DP83822 PHY ID                                                      */
/* ------------------------------------------------------------------ */
#define DP83822_PHYID1      0x2000
#define DP83822_PHYID2      0xA221

/* ------------------------------------------------------------------ */
/*  PHY State Structure (embedded in ENET)                              */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t regs[PHY_NUM_REGS];  /* PHY register file (16-bit) */
    bool link_up;
} IMXRT1180DP83822PHYState;

/* ------------------------------------------------------------------ */
/*  PHY Interface                                                       */
/* ------------------------------------------------------------------ */

/**
 * imxrt1180_dp83822_phy_reset:
 * @phy: PHY state
 *
 * Reset all PHY registers to power-on defaults.
 * Called by ENET during device reset.
 */
void imxrt1180_dp83822_phy_reset(IMXRT1180DP83822PHYState *phy);

/**
 * imxrt1180_dp83822_phy_mdio_read:
 * @phy: PHY state
 * @reg_addr: Register address (0-31)
 *
 * Read a PHY register via MDIO. Returns the 16-bit register value.
 */
uint16_t imxrt1180_dp83822_phy_mdio_read(IMXRT1180DP83822PHYState *phy,
                                         uint8_t reg_addr);

/**
 * imxrt1180_dp83822_phy_mdio_write:
 * @phy: PHY state
 * @reg_addr: Register address (0-31)
 * @val: 16-bit value to write
 *
 * Write a PHY register via MDIO, updating internal state.
 */
void imxrt1180_dp83822_phy_mdio_write(IMXRT1180DP83822PHYState *phy,
                                      uint8_t reg_addr, uint16_t val);

#endif /* HW_NET_IMXRT1180_DP83822_PHY_H */
