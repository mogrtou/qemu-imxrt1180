/**
 * imxrt_enet.c — i.MX RT1180 ENET 以太网驱动实现
 *
 * 实现 lwIP netif 所需的 init / output / input / IRQ 处理。
 * 寄存器地址和 BD 结构体严格遵循 docs/interfaces.md:
 *   §1.1 ENET 寄存器映射
 *   §3.2 Buffer Descriptor 结构体
 *
 * BD 环和数据缓冲区位于 OCRAM (0x2020_0000, 512KB)。
 * TX: lwIP → imxrt_enet_output → TX BD → TDAR → QEMU enet_handle_tx
 * RX: QEMU enet_receive → RX BD → RXF IRQ → imxrt_enet_input → lwIP
 *
 * 来源: docs/interfaces.md §3.1 ENET lwIP 驱动 API
 * 依赖: bal/config/evk_config.h (ENET_BASE_ADDR, ENET_IRQ_N, ENET_PHY_ADDR)
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "imxrt_enet.h"
#include "bal/config/evk_config.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/etharp.h"
#include "lwip/err.h"

/* ==========================================================================
 * 寄存器访问宏
 * ========================================================================== */
#define ENET_REG(r)  (*(volatile uint32_t *)(ENET_BASE_ADDR + (r)))

/* ==========================================================================
 * BD 环配置
 * ========================================================================== */
#define ENET_TX_BD_COUNT  8
#define ENET_RX_BD_COUNT  8
#define ENET_RX_BUFSIZE   1536    /* 足够容纳最大以太网帧 (1522) */

/*
 * BD 环和数据缓冲区位于 OCRAM 中。
 * BD 环按 32 字节对齐，数据缓冲区按 16 字节对齐。
 *
 * OCRAM 布局:
 *   0x2020_0000: TX BD 环 (8 × 8 = 64 bytes) → 32B aligned
 *   0x2020_0040: RX BD 环 (8 × 8 = 64 bytes) → 32B aligned
 *   0x2020_0080: TX 数据缓冲区 (8 × 1536 = 12288 bytes)
 *   0x2020_3080: RX 数据缓冲区 (8 × 1536 = 12288 bytes)
 *   0x2020_6080: 固件堆栈/其他 (剩余 ~448KB)
 */
#define TX_BD_BASE     (OCRAM_BASE)
#define RX_BD_BASE     (OCRAM_BASE + 0x40)
#define TX_BUF_BASE    (OCRAM_BASE + 0x80)
#define RX_BUF_BASE    (OCRAM_BASE + 0x3080)

/* BD 环访问宏 */
#define TX_BD(n)  ((volatile imxrt_enet_bd_t *)(TX_BD_BASE + (n) * sizeof(imxrt_enet_bd_t)))
#define RX_BD(n)  ((volatile imxrt_enet_bd_t *)(RX_BD_BASE + (n) * sizeof(imxrt_enet_bd_t)))

/* 数据缓冲区指针 (CPU 侧虚拟地址，QEMU 中即物理地址) */
#define TX_BUF(n)  ((volatile uint8_t *)(TX_BUF_BASE + (n) * ENET_RX_BUFSIZE))
#define RX_BUF(n)  ((volatile uint8_t *)(RX_BUF_BASE + (n) * ENET_RX_BUFSIZE))

/* ==========================================================================
 * 全局状态
 * ========================================================================== */
static uint8_t g_mac_addr[6];
static int g_tx_bd_head;        /* 下一个要填充的 TX BD */
static int g_rx_bd_tail;        /* 下一个要检查的 RX BD */
static int g_link_up;

static struct netif *g_netif;

/* ==========================================================================
 * 前向声明
 * ========================================================================== */
static void enet_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data);
static uint16_t enet_mdio_read(uint8_t phy_addr, uint8_t reg_addr);
static void enet_init_bd_rings(void);
static void enet_set_mac_addr(const uint8_t mac[6]);

/* ==========================================================================
 * 寄存器操作辅助函数
 * ========================================================================== */

/**
 * enet_wait_mdio — 等待 MDIO 传输完成
 * @return 0=成功, -1=超时
 */
static int enet_wait_mdio(void)
{
    volatile uint32_t eir;
    int timeout = 100000;

    while (timeout--) {
        eir = ENET_REG(ENET_EIR);
        if (eir & ENET_INT_MII) {
            /* 清除 MII 中断标志 (W1C) */
            ENET_REG(ENET_EIR) = ENET_INT_MII;
            return 0;
        }
    }
    return -1;  /* 超时 */
}

/**
 * enet_mdio_write — 通过 MDIO 写 PHY 寄存器 (Clause 22)
 */
static void enet_mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    uint32_t mmfr;

    mmfr = ENET_MMFR_ST_C22
         | ENET_MMFR_OP_WR
         | ENET_MMFR_PA(phy_addr)
         | ENET_MMFR_RA(reg_addr)
         | ENET_MMFR_TA
         | ENET_MMFR_DATA(data);

    ENET_REG(ENET_MMFR) = mmfr;
    enet_wait_mdio();
}

/**
 * enet_mdio_read — 通过 MDIO 读 PHY 寄存器 (Clause 22)
 * @return 16-bit 寄存器值
 */
static uint16_t enet_mdio_read(uint8_t phy_addr, uint8_t reg_addr)
{
    uint32_t mmfr;

    mmfr = ENET_MMFR_ST_C22
         | ENET_MMFR_OP_RD
         | ENET_MMFR_PA(phy_addr)
         | ENET_MMFR_RA(reg_addr)
         | ENET_MMFR_TA;

    ENET_REG(ENET_MMFR) = mmfr;
    enet_wait_mdio();

    return (uint16_t)(ENET_REG(ENET_MMFR) & 0xFFFF);
}

/* ==========================================================================
 * BD 环管理
 * ========================================================================== */

/**
 * enet_init_bd_rings — 初始化 TX/RX Buffer Descriptor 环
 *
 * TX BD: 初始全为 CPU 持有 (R=0), last BD 设置 W 位
 * RX BD: 初始全为 ENET 持有 (E=1), last BD 设置 W 位
 * 数据缓冲区指针指向 OCRAM 中的预分配 buffer
 */
static void enet_init_bd_rings(void)
{
    int i;

    /* ── TX BD 环初始化 ── */
    for (i = 0; i < ENET_TX_BD_COUNT; i++) {
        TX_BD(i)->status = 0;                    /* CPU owns */
        TX_BD(i)->length = 0;
        TX_BD(i)->data_ptr = (uint32_t)TX_BUF(i);
    }
    TX_BD(ENET_TX_BD_COUNT - 1)->status |= ENET_BD_TX_W;  /* last BD: wrap */

    /* ── RX BD 环初始化 ── */
    for (i = 0; i < ENET_RX_BD_COUNT; i++) {
        RX_BD(i)->status = ENET_BD_RX_E;         /* ENET owns (Empty) */
        RX_BD(i)->length = 0;
        RX_BD(i)->data_ptr = (uint32_t)RX_BUF(i);
    }
    RX_BD(ENET_RX_BD_COUNT - 1)->status |= ENET_BD_RX_W;  /* last BD: wrap */

    /* 设置 BD 环基地址寄存器 */
    ENET_REG(ENET_TDSR) = TX_BD_BASE;
    ENET_REG(ENET_RDSR) = RX_BD_BASE;

    g_tx_bd_head = 0;
    g_rx_bd_tail = 0;
}

/* ==========================================================================
 * MAC 地址配置
 * ========================================================================== */

/**
 * enet_set_mac_addr — 设置 ENET MAC 地址到 PALR/PAUR 寄存器
 *
 * PALR: MAC[31:0] = mac[0..3]
 * PAUR: MAC[47:32] = mac[4..5] (高 16 位为 Type 字段, 通常写 0x8808)
 */
static void enet_set_mac_addr(const uint8_t mac[6])
{
    uint32_t palr, paur;

    palr = ((uint32_t)mac[0] << 24)
         | ((uint32_t)mac[1] << 16)
         | ((uint32_t)mac[2] << 8)
         | ((uint32_t)mac[3]);

    paur = ((uint32_t)mac[4] << 24)
         | ((uint32_t)mac[5] << 16)
         | 0x8808;  /* Type field */

    ENET_REG(ENET_PALR) = palr;
    ENET_REG(ENET_PAUR) = paur;
}

/* ==========================================================================
 * 链路状态检测
 * ========================================================================== */

/**
 * imxrt_enet_get_link_status — 读取 PHY BMSR 寄存器获取链路状态
 * @return 1=Link Up, 0=Link Down
 */
uint32_t imxrt_enet_get_link_status(void)
{
    uint16_t bmsr;

    bmsr = enet_mdio_read(ENET_PHY_ADDR, 0x01);  /* BMSR */
    return (bmsr & 0x0004) ? 1 : 0;              /* BMSR[2] = Link Status */
}

/* ==========================================================================
 * ENET 硬件初始化
 * ========================================================================== */

/**
 * imxrt_enet_init — 初始化 ENET 硬件并关联到 lwIP netif
 *
 * 初始化序列:
 *   1. 解锁 ECR (写 ECR_MAGIC = 0x5A5A5A5A)
 *   2. 复位 ENET (ECR RESET bit), 等待自清零
 *   3. 配置 MAC 地址
 *   4. 初始化 BD 环
 *   5. 配置 MRBR (最大接收 buffer 大小)
 *   6. 配置 RCR / TCR
 *   7. 配置 MSCR (MDIO 时钟)
 *   8. 清除并配置中断
 *   9. 使能 ENET (ECR ETHEREN)
 *  10. 激活 RX (RDAR = 1)
 *
 * @param netif  lwIP 网络接口 (可为 NULL, 仅初始化硬件)
 * @return      0 成功, -1 失败
 */
int imxrt_enet_init(struct netif *netif)
{
    uint32_t ecr;
    int i;

    g_netif = netif;

    /* ── 1. 解锁 ECR ── */
    ENET_REG(ENET_ECR_MAGIC) = 0x5A5A5A5A;

    /* ── 2. 复位 ENET ── */
    ENET_REG(ENET_ECR) = ENET_ECR_RESET;
    for (i = 0; i < 1000; i++) {
        ecr = ENET_REG(ENET_ECR);
        if (!(ecr & ENET_ECR_RESET))
            break;
    }
    if (ecr & ENET_ECR_RESET) {
        return -1;  /* 复位超时 */
    }

    /* ── 3. 获取并配置 MAC 地址 ── */
    extern void BAL_ENET_GetMACAddr(uint8_t *mac);
    BAL_ENET_GetMACAddr(g_mac_addr);
    enet_set_mac_addr(g_mac_addr);

    /* ── 4. 初始化 BD 环 ── */
    enet_init_bd_rings();

    /* ── 5. 设置最大接收 buffer ── */
    ENET_REG(ENET_MRBR) = ENET_RX_BUFSIZE;

    /* ── 6. 配置 RCR ── */
    /*
     * RCR 配置:
     *   - RMII 模式 (MII_MODE=0): RT1180 使用 RMII
     *   - 最大帧长 1522 (支持 VLAN)
     *   - 流控关闭 (Phase 1)
     *   - Promiscuous 关闭 (MAC 地址过滤)
     *   - NLC=0 (不限制帧长)
     * 复位值: 0x05E0_0001
     */
    {
        uint32_t rcr = ENET_REG(ENET_RCR);
        rcr &= ~ENET_RCR_MII_MODE;              /* RMII 模式 */
        rcr |= ENET_RCR_MAX_FL(1522);           /* 最大帧长 1522 */
        ENET_REG(ENET_RCR) = rcr;
    }

    /* ── 6b. 配置 TCR ── */
    {
        uint32_t tcr = ENET_REG(ENET_TCR);
        tcr |= ENET_TCR_FDEN;                   /* 全双工 */
        ENET_REG(ENET_TCR) = tcr;
    }

    /* ── 7. 配置 MDIO 时钟 ──
     * MSCR: MII_SPEED = ENET_CLK / (2 * MDC_FREQ) - 1
     * 目标 MDC = 2.5 MHz, ENET_CLK = 50 MHz
     * MII_SPEED = 50,000,000 / (2 * 2,500,000) - 1 = 9
     */
    ENET_REG(ENET_MSCR) = ENET_MSCR_SPEED(9);

    /* ── 8. 清除全部中断标志 ── */
    ENET_REG(ENET_EIR) = 0xFFFFFFFF;

    /* ── 9. 配置中断屏蔽 ──
     * 使能: RXF (接收帧), TXF (发送帧), MII (MDIO)
     */
    ENET_REG(ENET_EIMR) = ENET_INT_RXF
                        | ENET_INT_TXF
                        | ENET_INT_MII;

    /* ── 10. 使能 ENET ── */
    ENET_REG(ENET_ECR_MAGIC) = 0x5A5A5A5A;
    ecr = ENET_REG(ENET_ECR);
    ecr |= ENET_ECR_ETHEREN;
    ENET_REG(ENET_ECR) = ecr;

    /* ── 11. 激活 RX BD 环 ── */
    ENET_REG(ENET_RDAR) = 0x00000001;

    /* ── 12. 检查 PHY 链路状态 ── */
    {
        uint16_t phyid1, phyid2;
        phyid1 = enet_mdio_read(ENET_PHY_ADDR, 0x02);
        phyid2 = enet_mdio_read(ENET_PHY_ADDR, 0x03);
        g_link_up = imxrt_enet_get_link_status();

        (void)phyid1;
        (void)phyid2;
    }

    /* ── 13. 关联到 lwIP netif ── */
    if (netif) {
        netif->hwaddr_len = 6;
        for (i = 0; i < 6; i++) {
            netif->hwaddr[i] = g_mac_addr[i];
        }
        netif->mtu = 1500;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
        if (g_link_up) {
            netif->flags |= NETIF_FLAG_LINK_UP;
        }
        netif->name[0] = 'e';
        netif->name[1] = 'n';

        /* 注册 lwIP 回调 — raw API: 仅注册 output, 不使用 linkoutput */
        netif->output = etharp_output;
#if LWIP_IPV6
        netif->output_ip6 = ethip6_output;
#endif
        /* linkoutput 为 netconn/socket API 使用, Phase 1 使用 raw API */
        netif->linkoutput = (netif_linkoutput_fn)imxrt_enet_output;
    }

    return 0;
}

/* ==========================================================================
 * TX 发送路径
 * ========================================================================== */

/**
 * imxrt_enet_output — lwIP 链路层发送回调
 *
 * lwIP 调用此函数发送以太网帧。数据已包含完整的以太网帧头
 * (dst MAC + src MAC + EtherType + payload)。
 *
 * 算法:
 *   1. 找到下一个 CPU 持有的 TX BD (R=0)
 *   2. 将 pbuf 数据链复制到 TX 数据缓冲区
 *   3. 设置 BD length 和 status (R=1, L=1, TC=1)
 *   4. 写 TDAR = 1 触发 DMA 发送
 *
 * @param netif  lwIP 网络接口
 * @param p      包含完整以太网帧的 pbuf
 * @return       ERR_OK 成功, ERR_BUF/ERR_IF 失败
 */
int imxrt_enet_output(struct netif *netif, struct pbuf *p)
{
    volatile imxrt_enet_bd_t *bd;
    uint8_t *buf;
    struct pbuf *q;
    uint16_t total_len;
    uint16_t offset;

    (void)netif;

    if (!g_link_up) {
        return ERR_IF;
    }

    /* ── 1. 找到下一个可用 TX BD ── */
    bd = TX_BD(g_tx_bd_head);
    if (bd->status & ENET_BD_TX_R) {
        /* ENET 仍持有此 BD, TX 环满 */
        return ERR_BUF;
    }

    /* ── 2. 获取数据缓冲区 ── */
    buf = (uint8_t *)TX_BUF(g_tx_bd_head);
    total_len = 0;
    offset = 0;

    /* ── 3. 复制 pbuf 链到缓冲区 ── */
    for (q = p; q != NULL; q = q->next) {
        if (offset + q->len > ENET_RX_BUFSIZE) {
            return ERR_BUF;  /* 帧太大 */
        }
        memcpy(buf + offset, q->payload, q->len);
        offset += q->len;
        total_len += q->len;
    }

    /* ── 4. 设置 BD ──
     * R=1 (ENET owns), L=1 (last in frame), TC=1 (append CRC)
     * W 位保持不变
     */
    bd->length = total_len;
    bd->status = (bd->status & (uint16_t)ENET_BD_TX_W)  /* 保留 W 位 */
               | ENET_BD_TX_R                           /* Ready */
               | ENET_BD_TX_L                           /* Last */
               | ENET_BD_TX_TC;                          /* CRC */

    /* ── 5. 更新索引并触发 TX ── */
    if (bd->status & ENET_BD_TX_W) {
        g_tx_bd_head = 0;
    } else {
        g_tx_bd_head++;
    }

    ENET_REG(ENET_TDAR) = 0x00000001;

    return ERR_OK;
}

/* ==========================================================================
 * RX 接收路径
 * ========================================================================== */

/**
 * imxrt_enet_input — 从 RX BD 环读取帧并提交给 lwIP
 *
 * 由 ISR 或轮询循环调用。
 * 扫描 RX BD 环, 检查是否有 CPU 持有的帧 (E=0, 表示 ENET 已写入数据)。
 *
 * 算法:
 *   1. 检查当前 RX BD 的 Empty 位
 *   2. 如果 E=0 (CPU owns), 提取帧数据
 *   3. 分配 pbuf, 提交给 netif->input()
 *   4. 将 BD 交还给 ENET (E=1)
 *   5. 移动到下一个 BD (有 W 位则回到 ring[0])
 */
void imxrt_enet_input(struct netif *netif)
{
    volatile imxrt_enet_bd_t *bd;
    struct pbuf *p;
    uint16_t status;
    uint16_t length;
    uint8_t *buf;
    int processed = 0;

    while (processed < ENET_RX_BD_COUNT) {
        bd = RX_BD(g_rx_bd_tail);

        /* 检查 ENET 是否已将数据写入此 BD (E=0 表示 CPU 持有) */
        status = bd->status;
        if (status & ENET_BD_RX_E) {
            /* 此 BD 仍为空, 没有新帧 */
            break;
        }

        /* ── 读取帧信息 ── */
        length = bd->length;
        buf = (uint8_t *)RX_BUF(g_rx_bd_tail);

        /* 检查错误 */
        if (!(status & ENET_BD_RX_L)) {
            /* 不是 last in frame — 多 BD 帧, Phase 1 不支持, 跳过 */
            goto return_bd;
        }
        if (status & (ENET_BD_RX_CR | ENET_BD_RX_OV | ENET_BD_RX_TR)) {
            /* CRC 错误 / 溢出 / 截断 → 丢弃 */
            goto return_bd;
        }

        /* ── 分配 pbuf 并提交给 lwIP ── */
        if (length > 0 && length < 1536) {
            p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
            if (p) {
                memcpy(p->payload, buf, length);
                if (netif->input(p, netif) != ERR_OK) {
                    pbuf_free(p);
                }
            }
        }

return_bd:
        /* ── 交还 BD 给 ENET ── */
        bd->length = 0;
        bd->status = ENET_BD_RX_E           /* Empty (ENET owns) */
                   | (status & ENET_BD_RX_W); /* 保留 W 位 */

        /* ── 移动索引 ── */
        if (status & ENET_BD_RX_W) {
            g_rx_bd_tail = 0;
        } else {
            g_rx_bd_tail++;
        }

        processed++;

        /* 重新激活 RX (如果因 BD 耗尽暂停) */
        ENET_REG(ENET_RDAR) = 0x00000001;
    }
}

/* ==========================================================================
 * 中断服务例程
 * ========================================================================== */

/**
 * ENET_IRQHandler — NVIC ENET1 中断服务例程 (IRQ 114)
 *
 * 处理流程:
 *   1. 读取 EIR 获取中断原因
 *   2. 处理 RXF (接收帧完成) → 调用 imxrt_enet_input()
 *   3. 处理 TXF (发送帧完成) → 记录/统计
 *   4. 处理 MII (MDIO 完成) → 记录 (已在 enet_wait_mdio 中同步处理)
 *   5. W1C 清除已处理的中断标志
 *
 * 注意: EIR 是 W1C (Write-1-to-Clear)。向相应位写 1 清除该中断。
 * 必须在处理完中断后才清除, 否则可能丢失新事件。
 */
void ENET_IRQHandler(void)
{
    uint32_t eir;
    uint32_t pending;

    /* ── 1. 读取中断事件 ── */
    eir = ENET_REG(ENET_EIR);
    pending = eir & ENET_REG(ENET_EIMR);  /* 仅处理已使能的中断源 */

    if (!pending) {
        return;  /* 无待处理中断 */
    }

    /* ── 2. 处理 RXF (接收帧) ── */
    if (pending & ENET_INT_RXF) {
        if (g_netif) {
            imxrt_enet_input(g_netif);
        }
        /* 清除 RXF 中断标志 */
        ENET_REG(ENET_EIR) = ENET_INT_RXF;
    }

    /* ── 3. 处理 TXF (发送帧完成) ── */
    if (pending & ENET_INT_TXF) {
        /* TX 完成 — lwIP raw API 无需额外处理, 清标志即可 */
        ENET_REG(ENET_EIR) = ENET_INT_TXF;
    }

    /* ── 4. 处理 MII (MDIO 事务完成) ── */
    if (pending & ENET_INT_MII) {
        /* MDIO 完成 — 已在 enet_wait_mdio() 中同步等待并清除 */
        ENET_REG(ENET_EIR) = ENET_INT_MII;
    }

    /* ── 5. 处理其他中断 (错误等) ── */
    if (pending & ENET_INT_EBERR) {
        /* 总线错误 — 记录后清除 */
        ENET_REG(ENET_EIR) = ENET_INT_EBERR;
    }
    if (pending & ENET_INT_TXB) {
        /* TX Buffer 用完 — 清除 */
        ENET_REG(ENET_EIR) = ENET_INT_TXB;
    }
    if (pending & ENET_INT_RXB) {
        /* RX Buffer 不够 — 清除, 上层重试 */
        ENET_REG(ENET_EIR) = ENET_INT_RXB;
    }
}

/* ==========================================================================
 * MAC 地址访问器
 * ========================================================================== */

/**
 * imxrt_enet_get_mac_addr — 获取当前 MAC 地址
 */
void imxrt_enet_get_mac_addr(uint8_t mac[6])
{
    int i;
    for (i = 0; i < 6; i++) {
        mac[i] = g_mac_addr[i];
    }
}

/**
 * imxrt_enet_set_mac_addr — 设置新 MAC 地址
 */
void imxrt_enet_set_mac_addr(struct netif *netif, const uint8_t mac[6])
{
    int i;
    for (i = 0; i < 6; i++) {
        g_mac_addr[i] = mac[i];
    }
    enet_set_mac_addr(mac);

    if (netif) {
        for (i = 0; i < 6; i++) {
            netif->hwaddr[i] = mac[i];
        }
    }
}
