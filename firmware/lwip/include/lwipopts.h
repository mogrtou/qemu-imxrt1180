/**
 * lwipopts.h — lwIP 编译期配置 (lwIP options)
 *
 * 基于 evk_config.h 中的宏定义, 控制:
 *   - 协议栈功能开关 (TCP / UDP / DHCP / DNS)
 *   - 内存池大小
 *   - 网络接口参数
 *   - 调试级别
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/*
 * lwIP 选项 — 硬编码配置 (不依赖 BAL, 避免循环依赖)
 * 板级 IP 配置在 bal/config/evk_config.h 中, 仅供应用层引用
 */

/* ==========================================================================
 * 协议栈功能开关 — M3 完整协议栈: TCP + UDP + DHCP + DNS
 * ========================================================================== */
#define LWIP_TCP                1
#define LWIP_UDP                1
#define LWIP_RAW                1
#define LWIP_DHCP               1
#define LWIP_DHCP_DOES_ACD_CHECK  0   /* Phase 1: 无需地址冲突检测 */
#define LWIP_DNS                0
#define LWIP_AUTOIP             0
#define LWIP_IGMP               0
#define LWIP_SNMP               0
#define LWIP_IPV4               1
#define LWIP_IPV6               0

/* ==========================================================================
 * 协议栈内存配置
 * ========================================================================== */
#define MEM_ALIGNMENT           4       /* 32-bit 对齐 */

/* 堆内存 (malloc) */
#define MEM_SIZE                (32 * 1024)  /* 32 KB 堆 */

/* ── 无标准库: 关闭 ctype / atoi / byteorder ── */
#define LWIP_NO_CTYPE_H         1
#define LWIP_NO_STDINT_H        1

/* ── netif_find 用 atoi, 在 -nostdlib 下不可用 ── */
#define LWIP_NETIF_API          1
#define LWIP_NETIF_HOSTNAME     0       /* Phase 1: 不设主机名 */

/* pbuf 内存池 */
#define MEMP_NUM_PBUF           16
#define MEMP_NUM_TCP_PBUF       16
#define MEMP_NUM_TCP_SEG        32
#define MEMP_NUM_UDP_PCB        8
#define MEMP_NUM_TCP_PCB        8
#define MEMP_NUM_TCP_PCB_LISTEN 4
#define MEMP_NUM_NETBUF         16
#define MEMP_NUM_NETCONN        8

/* ==========================================================================
 * TCP 参数
 * ========================================================================== */
#define TCP_MSS                 1460
#define TCP_WND                 (4 * TCP_MSS)
#define TCP_SND_BUF             (4 * TCP_MSS)
#define TCP_SND_QUEUELEN        16

/* ==========================================================================
 * 网络接口
 * ========================================================================== */
#define LWIP_NETIF_HOSTNAME     0       /* -nostdlib: 关闭主机名 (避免 atoi) */
#define LWIP_NETIF_API          1       /* 启用 netifapi */
#define LWIP_NETIF_STATUS_CALLBACK 1    /* 状态变化回调 */
#define LWIP_NETIF_LINK_CALLBACK    1   /* 链路状态回调 */
#define LWIP_HAVE_LOOPIF        0       /* 无回环接口 */
#define LWIP_NETIF_LOOPBACK     0

/* ==========================================================================
 * ARP 表
 * ========================================================================== */
#define ARP_TABLE_SIZE          8
#define ARP_QUEUEING            1

/* ==========================================================================
 * 校验和
 * ========================================================================== */
#define CHECKSUM_GEN_IP         0       /* 硬件计算 (ENET) */
#define CHECKSUM_GEN_UDP        0
#define CHECKSUM_GEN_TCP        0
#define CHECKSUM_CHECK_IP       0
#define CHECKSUM_CHECK_UDP      0
#define CHECKSUM_CHECK_TCP      0

/* ==========================================================================
 * 调试 — 全部关闭
 * ========================================================================== */
#define LWIP_DEBUG              0
#define LWIP_DBG_TYPES_ON       LWIP_DBG_OFF
#define LWIP_NOASSERT            1
#define LWIP_PLATFORM_DIAG(x)    do { } while (0)
#define LWIP_PLATFORM_ASSERT(x)  do { } while (0)

/* ==========================================================================
 * 线程 / 锁 (FreeRTOS 适配)
 * ========================================================================== */
#define LWIP_NO_STDINT_H        1       /* 使用 lwIP 内置类型 */
#define LWIP_PROVIDE_ERRNO      1       /* lwIP 提供 errno */

/* FreeRTOS 信号量 / 互斥锁 (Phase 2 启用) */
#if 0
#define SYS_ARCH_PROTECT(lev)   do { lev = 0; } while (0)  /* 占位 */
#define SYS_ARCH_UNPROTECT(lev) do { } while (0)
#endif

/* ==========================================================================
 * Socket API (可选)
 * ========================================================================== */
#define LWIP_SOCKET             0       /* Phase 1 使用 netconn API */
#define LWIP_COMPAT_SOCKETS     0

/* ==========================================================================
 * 统计信息
 * ========================================================================== */
#define LWIP_STATS              0
#define LINK_STATS              0
#define MEM_STATS               0
#define MEMP_STATS              0
#define SYS_STATS               0

#endif /* LWIPOPTS_H */
