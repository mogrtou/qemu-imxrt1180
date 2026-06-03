/**
 * imxrt_enet.h — i.MX RT1180 ENET 以太网驱动接口
 *
 * 提供 lwIP netif 所需的 init / output / input / IRQ 处理。
 * 寄存器地址和 BD 结构体严格遵循 docs/interfaces.md:
 *   §1.1 ENET 寄存器映射
 *   §3.2 Buffer Descriptor 结构体
 *
 * 来源: docs/interfaces.md §3.1 ENET lwIP 驱动 API
 */

#ifndef IMXRT_ENET_H
#define IMXRT_ENET_H

#include <stdint.h>
#include "lwip/err.h"

/* 前向声明: 避免引入整个 lwIP 头文件 */
struct netif;
struct pbuf;

/* ==========================================================================
 * ENET 寄存器映射 (docs/interfaces.md §1.1)
 * ========================================================================== */

/* 所有偏移量相对于 ENET_BASE_ADDR (0x40424000) */
typedef enum {
    ENET_ECR        = 0x0000,   /* Ethernet Control */
    ENET_EIR        = 0x0004,   /* Interrupt Event */
    ENET_EIMR       = 0x0008,   /* Interrupt Mask */
    ENET_RDAR       = 0x0010,   /* RX Descriptor Active */
    ENET_TDAR       = 0x0014,   /* TX Descriptor Active */
    ENET_ECR_MAGIC  = 0x0024,   /* ECR 写保护解锁魔法数 */
    ENET_MMFR       = 0x0040,   /* MDIO Frame */
    ENET_MSCR       = 0x0044,   /* MDIO Speed Control */
    ENET_MIBC       = 0x0048,   /* MIB Control */
    ENET_RCR        = 0x0064,   /* RX Control */
    ENET_TCR        = 0x0084,   /* TX Control */
    ENET_PALR       = 0x00C4,   /* Physical Address Lower */
    ENET_PAUR       = 0x00C8,   /* Physical Address Upper */
    ENET_OPD        = 0x00E4,   /* Opcode/Pause Duration */
    ENET_IAUR       = 0x00EC,   /* Individual Upper Address */
    ENET_IALR       = 0x00F0,   /* Individual Lower Address */
    ENET_GAUR       = 0x00F4,   /* Group Upper Address */
    ENET_GALR       = 0x00F8,   /* Group Lower Address */
    ENET_TFWR       = 0x0100,   /* Transmit FIFO Watermark */
    ENET_RDSR       = 0x0144,   /* RX Descriptor Ring Start */
    ENET_TDSR       = 0x0154,   /* TX Descriptor Ring Start */
    ENET_MRBR       = 0x0160,   /* Max RX Buffer Size */
} enet_reg_offset_t;

/* ── ECR 位 ── */
#define ENET_ECR_RESET     (1UL << 0)   /* 写 1 复位 ENET, 自清零 */
#define ENET_ECR_ETHEREN   (1UL << 1)   /* 以太网使能 */
#define ENET_ECR_DBSWP     (1UL << 8)   /* 描述符字节交换 */
#define ENET_ECR_DBGEN     (1UL << 6)   /* 调试使能 */

/* ── EIR / EIMR 位 (docs/interfaces.md §1.2) ── */
#define ENET_INT_TXF       (1UL << 23)  /* Transmit Frame */
#define ENET_INT_TXB       (1UL << 21)  /* Transmit Buffer */
#define ENET_INT_RXF       (1UL << 25)  /* Receive Frame */
#define ENET_INT_RXB       (1UL << 24)  /* Receive Buffer */
#define ENET_INT_MII       (1UL << 27)  /* MDIO Complete */
#define ENET_INT_EBERR     (1UL << 22)  /* Ethernet Bus Error */

/* ── RCR / TCR 位 ── */
#define ENET_RCR_MII_MODE  (1UL << 2)   /* MII 模式 (清 0 为 RMII) */
#define ENET_RCR_PROM      (1UL << 3)   /* Promiscuous 模式 */
#define ENET_RCR_FCE       (1UL << 5)   /* Flow Control Enable */
#define ENET_RCR_MAX_FL(x) (((x) & 0x7FF) << 16) /* 最大帧长 */
#define ENET_TCR_FDEN      (1UL << 2)   /* Full Duplex */
#define ENET_TCR_FCE       (1UL << 5)   /* Flow Control Enable */

/* ── MMFR 位 (docs/interfaces.md §1.3) ── */
#define ENET_MMFR_ST_C22   (1UL << 30)  /* Clause 22 Start */
#define ENET_MMFR_OP_WR    (0x1UL << 28)/* Write Opcode */
#define ENET_MMFR_OP_RD    (0x2UL << 28)/* Read Opcode */
#define ENET_MMFR_TA       (0x2UL << 16)/* Turnaround */
#define ENET_MMFR_PA(x)    (((x) & 0x1F) << 23)  /* PHY Address */
#define ENET_MMFR_RA(x)    (((x) & 0x1F) << 18)  /* Register Address */
#define ENET_MMFR_DATA(x)  (((x) & 0xFFFF) << 0) /* Data */

/* ── MSCR ── */
#define ENET_MSCR_SPEED(x) ((x) & 0x7F)  /* MDIO Speed (MII_SPEED) */

/* ==========================================================================
 * Buffer Descriptor 结构体 (docs/interfaces.md §3.2)
 * ========================================================================== */
typedef struct __attribute__((packed, aligned(4))) {
    uint16_t status;        /* R/E/W/L/M/BC/MC/LG/NO/CR/OV/TR */
    uint16_t length;        /* Data length (TX: 待发送; RX: buffer 大小) */
    uint32_t data_ptr;      /* 数据缓冲区物理地址 */
} imxrt_enet_bd_t;

/* ── TX BD Status 位 ── */
#define ENET_BD_TX_R         (1 << 15)   /* Ready (1=ENET owns, 0=CPU owns) */
#define ENET_BD_TX_TO1       (1 << 14)   /* TX Option 1 */
#define ENET_BD_TX_W         (1 << 13)   /* Wrap (环末尾标记) */
#define ENET_BD_TX_TO2       (1 << 12)   /* TX Option 2 (last) */
#define ENET_BD_TX_L         (1 << 11)   /* Last in frame */
#define ENET_BD_TX_TC        (1 << 10)   /* TX CRC append */

/* ── RX BD Status 位 ── */
#define ENET_BD_RX_E         (1 << 15)   /* Empty (1=ENET owns, 0=CPU owns) */
#define ENET_BD_RX_W         (1 << 13)   /* Wrap */
#define ENET_BD_RX_L         (1 << 11)   /* Last in frame (frame complete) */
#define ENET_BD_RX_M         (1 << 8)    /* Miss (promiscuous) */
#define ENET_BD_RX_BC        (1 << 7)    /* Broadcast */
#define ENET_BD_RX_MC        (1 << 6)    /* Multicast */
#define ENET_BD_RX_LG        (1 << 5)    /* Frame length violation */
#define ENET_BD_RX_NO        (1 << 4)    /* Non-octet aligned */
#define ENET_BD_RX_CR        (1 << 2)    /* CRC error */
#define ENET_BD_RX_OV        (1 << 1)    /* Overrun */
#define ENET_BD_RX_TR        (1 << 0)    /* Truncated */

/* ==========================================================================
 * 驱动 API (docs/interfaces.md §3.1)
 * ========================================================================== */

struct netif;  /* lwIP 网络接口 (前向声明) */

/**
 * imxrt_enet_init — 初始化 ENET 硬件, 并关联到 lwIP netif
 *
 * 序列:
 *   1. 解锁 ECR (写 ECR_MAGIC = 0x5A5A5A5A)
 *   2. 复位 ENET (ECR RESET)
 *   3. 等待复位完成, 配置 MAC 地址
 *   4. 初始化 BD 环 (TX/RX rings)
 *   5. 配置 RCR / TCR / MSCR
 *   6. 清除并配置中断
 *   7. 使能 ENET (ECR ETHEREN)
 *
 * @param netif  lwIP 网络接口指针
 * @return      0 成功, 非 0 失败
 */
int imxrt_enet_init(struct netif *netif);

/**
 * imxrt_enet_output — lwIP 发送回调
 *
 * @param netif  lwIP 网络接口
 * @param p      pbuf 数据包
 * @return       ERR_OK 成功, 其他为错误
 */
err_t imxrt_enet_output(struct netif *netif, struct pbuf *p);

/**
 * imxrt_enet_input — 从 RX BD 环读取帧并提交给 lwIP
 *
 * 由 ISR 或主循环调用。
 *
 * @param netif  lwIP 网络接口
 */
void imxrt_enet_input(struct netif *netif);

/**
 * ENET_IRQHandler — NVIC ENET1 中断服务例程 (IRQ 114)
 */
void ENET_IRQHandler(void);

#endif /* IMXRT_ENET_H */
