/**
 * evk_config.h — NXP i.MX RT1180 EVK 板级配置
 *
 * 板卡标识: NXP RT1180 EVK
 * 编译: make BOARD=evk
 * 来源: docs/interfaces.md §3.4 板级配置文件模板
 */

#ifndef BAL_CONFIG_H
#define BAL_CONFIG_H

/* ==========================================================================
 * 板卡标识
 * ========================================================================== */
#define BOARD_NAME              "NXP i.MX RT1180 EVK"
#define BOARD_VARIANT           "EVK"

/* ==========================================================================
 * 内存布局 (与 docs/architecture.md §3 一致)
 * ========================================================================== */
#define ITCM_BASE               (0x00000000UL)
#define ITCM_SIZE               (256 * 1024)
#define DTCM_BASE               (0x20000000UL)
#define DTCM_SIZE               (256 * 1024)
#define OCRAM_BASE              (0x20200000UL)
#define OCRAM_SIZE              (512 * 1024)

/* ==========================================================================
 * ENET 配置
 * ========================================================================== */
#define ENET_BASE_ADDR          (0x40424000UL)
#define ENET_IRQ_N              114
#define ENET_PHY_ADDR           0               /* DP83822 MDIO address */

/* ==========================================================================
 * PHY 配置
 * ========================================================================== */
#define PHY_MDIO_CLAUSE         22              /* IEEE 802.3 Clause 22 */
#define PHY_RESET_GPIO          (-1)            /* -1 = no GPIO reset */
#define PHY_TYPE                "TI_DP83822"

/* ==========================================================================
 * 时钟 (Cortex-M7 核心时钟 + ENET 外设时钟)
 * ========================================================================== */
#define ENET_CLOCK_HZ           (50000000UL)    /* 50 MHz RMII REF_CLK */
#define CORE_CLOCK_HZ           (600000000UL)
#define SYSTICK_CLOCK_HZ        (CORE_CLOCK_HZ)

/* ==========================================================================
 * 调试
 * ========================================================================== */
#define DEBUG_UART_BASE         (0x40070000UL)  /* LPUART1 */
#define DEBUG_UART_BAUD         115200
#define DEBUG_USE_SEMIHOSTING   1               /* 1=QEMU semihosting, 0=UART */

/* ==========================================================================
 * lwIP 网络配置 (lwipopts.h 引用)
 * ========================================================================== */
#define LWIP_IPADDR_STATIC      0               /* 0=DHCP, 1=Static IP */
#define LWIP_IPADDR0            10
#define LWIP_IPADDR1            0
#define LWIP_IPADDR2            2
#define LWIP_IPADDR3            15
#define LWIP_NETMASK0           255
#define LWIP_NETMASK1           255
#define LWIP_NETMASK2           255
#define LWIP_NETMASK3           0
#define LWIP_GW0                10
#define LWIP_GW1                0
#define LWIP_GW2                2
#define LWIP_GW3                2

#endif /* BAL_CONFIG_H */
