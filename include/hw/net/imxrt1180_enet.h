/*
 * QEMU model of i.MX RT1180 ENET Ethernet MAC Controller
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_NET_IMXRT1180_ENET_H
#define HW_NET_IMXRT1180_ENET_H

#include "hw/sysbus.h"
#include "net/net.h"
#include "hw/net/imxrt1180_dp83822_phy.h"

#define TYPE_IMXRT1180_ENET "imxrt1180-enet"
OBJECT_DECLARE_SIMPLE_TYPE(IMXRT1180ENETState, IMXRT1180_ENET)

/* ------------------------------------------------------------------ */
/*  ENET Register Offsets                                              */
/* ------------------------------------------------------------------ */
#define ENET_ECR            0x0000  /* Ethernet Control                  */
#define ENET_EIR            0x0004  /* Interrupt Event                   */
#define ENET_EIMR           0x0008  /* Interrupt Mask                    */
#define ENET_RDAR           0x0010  /* RX Descriptor Active              */
#define ENET_TDAR           0x0014  /* TX Descriptor Active              */
#define ENET_ECR_MAGIC      0x0024  /* ECR Write-Protect Magic           */
#define ENET_MMFR           0x0040  /* MDIO Management Frame             */
#define ENET_MSCR           0x0044  /* MDIO Speed Control                */
#define ENET_MIBC           0x0048  /* MIB Control                       */
#define ENET_RCR            0x0064  /* RX Control                        */
#define ENET_TCR            0x0084  /* TX Control                        */
#define ENET_PALR           0x00C4  /* Physical Address Lower            */
#define ENET_PAUR           0x00C8  /* Physical Address Upper            */
#define ENET_OPD            0x00E4  /* Opcode / Pause Duration           */
#define ENET_IAUR           0x00EC  /* Desc Individual Upper Address     */
#define ENET_IALR           0x00F0  /* Desc Individual Lower Address     */
#define ENET_GAUR           0x00F4  /* Desc Group Upper Address          */
#define ENET_GALR           0x00F8  /* Desc Group Lower Address          */
#define ENET_TFWR           0x0100  /* TX FIFO Watermark                 */
#define ENET_RDSR           0x0144  /* RX Descriptor Ring Start          */
#define ENET_TDSR           0x0154  /* TX Descriptor Ring Start          */
#define ENET_MRBR           0x0160  /* Max RX Buffer Size                */
#define ENET_RSFL           0x0184  /* RX FIFO Section Full Threshold    */
#define ENET_RSEM           0x018C  /* RX FIFO Section Empty Threshold   */
#define ENET_RAEM           0x0190  /* RX FIFO Almost Empty Threshold    */
#define ENET_RAFL           0x0194  /* RX FIFO Almost Full Threshold     */
#define ENET_TSEM           0x0198  /* TX FIFO Section Empty Threshold   */
#define ENET_TAEM           0x019C  /* TX FIFO Almost Empty Threshold    */
#define ENET_TAFL           0x01A0  /* TX FIFO Almost Full Threshold     */
#define ENET_TIPG           0x01A4  /* TX Inter-Packet Gap               */
#define ENET_ATCR           0x01C0  /* Adjustable Timer Control (1588)   */
#define ENET_ATVR           0x01C4  /* Adjustable Timer Value (1588)     */
#define ENET_ATOFF          0x01C8  /* Timer Offset (1588)               */
#define ENET_ATPER          0x01CC  /* Timer Period (1588)               */
#define ENET_ATCOR          0x01D0  /* Timer Correction (1588)           */
#define ENET_ATINC          0x01D4  /* Timer Increment (1588)            */
#define ENET_ATSTMP         0x01D8  /* Timestamp of Last 1588 Frame      */
#define ENET_TGSR           0x0200  /* Timer Global Status (1588)        */
#define ENET_TCSR0          0x0208  /* Timer Control Status 0            */
#define ENET_TCCR0          0x020C  /* Timer Compare Capture 0           */
#define ENET_TCSR1          0x0210  /* Timer Control Status 1            */
#define ENET_TCCR1          0x0214  /* Timer Compare Capture 1           */
#define ENET_TCSR2          0x0218  /* Timer Control Status 2            */
#define ENET_TCCR2          0x021C  /* Timer Compare Capture 2           */
#define ENET_TCSR3          0x0220  /* Timer Control Status 3            */
#define ENET_TCCR3          0x0224  /* Timer Compare Capture 3           */

/* ------------------------------------------------------------------ */
/*  EIR / EIMR Bit Definitions                                          */
/* ------------------------------------------------------------------ */
#define ENET_INT_TXF        (1u << 23)  /* Transmit Frame                */
#define ENET_INT_TXB        (1u << 21)  /* Transmit Buffer               */
#define ENET_INT_RXF        (1u << 25)  /* Receive Frame                 */
#define ENET_INT_RXB        (1u << 24)  /* Receive Buffer                */
#define ENET_INT_MII        (1u << 27)  /* MDIO Transfer Complete        */
#define ENET_INT_EBERR      (1u << 22)  /* Ethernet Bus Error            */
#define ENET_INT_LC         (1u << 2)   /* Late Collision                */
#define ENET_INT_RL         (1u << 4)   /* Collision Retry Limit         */
#define ENET_INT_UN         (1u << 5)   /* TX FIFO Underrun              */

/* ------------------------------------------------------------------ */
/*  ECR Bit Definitions                                                 */
/* ------------------------------------------------------------------ */
#define ENET_ECR_RESET      (1u << 0)   /* Soft Reset                     */
#define ENET_ECR_ETHEREN    (1u << 1)   /* Ethernet Enable                */
#define ENET_ECR_DBSWP      (1u << 8)   /* Desc Byte Swap (1=swap)       */
#define ENET_ECR_DBGEN      (1u << 6)   /* Debug Enable                   */
#define ENET_ECR_EN1588     (1u << 2)   /* 1588 Enable                    */

/* ECR Write-Protect Magic unlock value */
#define ENET_ECR_MAGIC_VAL  UINT32_C(0x5A5A5A5A)

/* ECR Reset Value: DBSWP=1 by default (ARM LE mode with byte-swap) */
#define ENET_ECR_RESET_VAL  UINT32_C(0xF0000100)

/* ------------------------------------------------------------------ */
/*  RCR Bit Definitions                                                 */
/* ------------------------------------------------------------------ */
#define ENET_RCR_LOOP       (1u << 0)   /* Internal Loopback              */
#define ENET_RCR_DRT        (1u << 1)   /* Disable RX on TX               */
#define ENET_RCR_MII_MODE   (1u << 2)   /* MII Mode (0=RMII)             */
#define ENET_RCR_PROM       (1u << 3)   /* Promiscuous Mode               */
#define ENET_RCR_RMII_MODE  (1u << 8)   /* RMII Mode (RT1180 specific)    */
#define ENET_RCR_RMII_10T   (1u << 9)   /* RMII 10Mbps Mode               */
#define ENET_RCR_FCE        (1u << 5)   /* Flow Control Enable            */
#define ENET_RCR_NLC        (1u << 30)  /* No Length Check                */
#define ENET_RCR_MAX_FL(x)  (((x) >> 16) & 0x3FFF)  /* Max Frame Length  */

#define ENET_RCR_RESET_VAL  0x05EE0001

/* ------------------------------------------------------------------ */
/*  TCR Bit Definitions                                                 */
/* ------------------------------------------------------------------ */
#define ENET_TCR_RFC_PAUSE  (1u << 3)   /* Receive Frame Control Pause   */
#define ENET_TCR_TFC_PAUSE  (1u << 4)   /* Transmit Frame Control Pause  */
#define ENET_TCR_FDEN       (1u << 2)   /* Full Duplex Enable            */
#define ENET_TCR_FCE        (1u << 5)   /* Flow Control Enable           */
#define ENET_TCR_ADDINS     (1u << 17)  /* Additional Insert (CRC)       */

#define ENET_TCR_RESET_VAL  0x00000010

/* ------------------------------------------------------------------ */
/*  Buffer Descriptor Structure (as seen in RAM)                        */
/* ------------------------------------------------------------------ */
typedef struct __attribute__((packed)) {
    uint16_t status;    /* BD flags: R/E, W, L, M, BC, MC, ...          */
    uint16_t length;    /* Data length                                   */
    uint32_t data_ptr;  /* Buffer pointer (physical address)             */
} IMXRT1180ENETBD;

/* TX BD Status bits */
#define ENET_BD_TX_R        (1u << 15)  /* Ready (HW owns)               */
#define ENET_BD_TX_TO1      (1u << 14)  /* Option 1                      */
#define ENET_BD_TX_W        (1u << 13)  /* Wrap (ring end)               */
#define ENET_BD_TX_TO2      (1u << 12)  /* Option 2                      */
#define ENET_BD_TX_L        (1u << 11)  /* Last in frame                 */
#define ENET_BD_TX_TC       (1u << 10)  /* TX CRC (append)               */

/* RX BD Status bits */
#define ENET_BD_RX_E        (1u << 15)  /* Empty (HW owns)               */
#define ENET_BD_RX_W        (1u << 13)  /* Wrap                          */
#define ENET_BD_RX_L        (1u << 11)  /* Last in frame                 */
#define ENET_BD_RX_M        (1u << 8)   /* Miss (promiscuous)            */
#define ENET_BD_RX_BC       (1u << 7)   /* Broadcast                     */
#define ENET_BD_RX_MC       (1u << 6)   /* Multicast                     */
#define ENET_BD_RX_LG       (1u << 5)   /* Frame length violation        */
#define ENET_BD_RX_NO       (1u << 4)   /* Non-octet aligned frame       */
#define ENET_BD_RX_CR       (1u << 2)   /* CRC error                     */
#define ENET_BD_RX_OV       (1u << 1)   /* Overrun                       */
#define ENET_BD_RX_TR       (1u << 0)   /* Truncated                     */

/* ------------------------------------------------------------------ */
/*  ENET QOM State                                                      */
/* ------------------------------------------------------------------ */
struct IMXRT1180ENETState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    /* Memory region for MMIO registers */
    MemoryRegion iomem;

    /* Interrupt line -> NVIC */
    qemu_irq irq;

    /* Network backend (TAP / SLIRP) */
    NICState *nic;
    NICConf conf;
    uint8_t macaddr[6];

    /* PHY sub-object (embedded) */
    IMXRT1180DP83822PHYState phy;

    /* Properties */
    uint8_t phy_addr;
    uint32_t tx_ring_size;
    uint32_t rx_ring_size;
    uint32_t max_frame_size;

    /* ---- Core Registers ---- */
    uint32_t ecr;           /* Ethernet Control                          */
    uint32_t eir;           /* Interrupt Event                           */
    uint32_t eimr;          /* Interrupt Mask                            */
    uint32_t rdar;          /* RX Descriptor Active                      */
    uint32_t tdar;          /* TX Descriptor Active                      */
    uint32_t ecr_magic;     /* ECR write-protect unlock                  */
    uint32_t mmfr;          /* MDIO Management Frame                     */
    uint32_t mscr;          /* MDIO Speed Control                        */
    uint32_t mibc;          /* MIB Control                               */
    uint32_t rcr;           /* RX Control                                */
    uint32_t tcr;           /* TX Control                                */
    uint32_t palr;          /* Physical Address Lower [31:0]             */
    uint32_t paur;          /* Physical Address Upper [47:32]            */
    uint32_t opd;           /* Opcode / Pause Duration                   */
    uint32_t iaur;          /* Desc Individual Upper Address             */
    uint32_t ialr;          /* Desc Individual Lower Address             */
    uint32_t gaur;          /* Desc Group Upper Address                  */
    uint32_t galr;          /* Desc Group Lower Address                  */
    uint32_t tfwr;          /* TX FIFO Watermark                         */

    /* ---- Descriptor Ring Registers ---- */
    uint32_t rdsr;          /* RX Descriptor Ring Start                  */
    uint32_t tdsr;          /* TX Descriptor Ring Start                  */
    uint32_t mrbr;          /* Max RX Buffer Size                        */

    /* ---- FIFO Threshold Registers ---- */
    uint32_t rsfl;          /* RX FIFO Section Full Threshold            */
    uint32_t rsem;          /* RX FIFO Section Empty Threshold           */
    uint32_t raem;          /* RX FIFO Almost Empty Threshold            */
    uint32_t rafl;          /* RX FIFO Almost Full Threshold             */
    uint32_t tsem;          /* TX FIFO Section Empty Threshold           */
    uint32_t taem;          /* TX FIFO Almost Empty Threshold            */
    uint32_t tafl;          /* TX FIFO Almost Full Threshold             */
    uint32_t tipg;          /* TX Inter-Packet Gap                       */

    /* ---- IEEE 1588 Registers (read-as-zero in Phase 1) ---- */
    uint32_t atcr;
    uint32_t atvr;
    uint32_t atoff;
    uint32_t atper;
    uint32_t atcor;
    uint32_t atinc;
    uint32_t atstmp;
    uint32_t tgsr;
    uint32_t tcsr[4];
    uint32_t tccr[4];

    /* TX/RX ring tracking */
    int32_t tx_curr;        /* Current TX BD index                       */
    int32_t rx_curr;        /* Current RX BD index                       */
};

#endif /* HW_NET_IMXRT1180_ENET_H */
