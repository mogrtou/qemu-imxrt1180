/**
 * main.c — 固件主入口 (M3: FreeRTOS + lwIP + HTTP Server)
 *
 * 启动序列:
 *   1. sys_tick_init() — 配置 ARM SysTick 为 FreeRTOS tick (1kHz)
 *   2. FreeRTOS 内核启动
 *   3. lwip_init_task — 初始化 lwIP TCP/IP 协议栈 (FreeRTOS 任务)
 *   4. httpd_task    — HTTP Server demo (FreeRTOS 任务, 监听 80 端口)
 *
 * 调试通道: semihosting (QEMU -semihosting)
 *
 * Cortex-M7, arm-none-eabi-gcc, C11, FreeRTOS 10.6.2, lwIP 2.2.1
 */

#include <stdint.h>

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

#include "bal/config/evk_config.h"

/* 前向声明 */
void sys_tick_init(void);
void lwip_init_task(void *arg);
void httpd_task(void *arg);

/* ── 链接脚本导出 ── */
extern uint32_t _estack;

/* ==========================================================================
 * semihosting_write — 通过 ARM semihosting 输出字符串到 QEMU stderr
 * ========================================================================== */
#ifdef USE_SEMIHOSTING
__attribute__((noinline))
void semihosting_write(const char *s)
{
    __asm__ volatile (
        "mov  r0, #0x04\n"   /* SYS_WRITE0 */
        "mov  r1, %[str]\n"
        "bkpt #0xAB\n"
        :
        : [str] "r" (s)
        : "r0", "r1"
    );
}
#endif

#ifdef USE_SEMIHOSTING
__attribute__((noinline))
static void semihosting_putc(char c)
{
    __asm__ volatile (
        "mov  r0, #0x03\n"   /* SYS_WRITEC */
        "mov  r1, %[ch]\n"
        "bkpt #0xAB\n"
        :
        : [ch] "r" ((unsigned int)c)
        : "r0", "r1"
    );
}
#endif

#ifndef USE_SEMIHOSTING
void uart_puts(const char *s);
#endif

/* ==========================================================================
 * 简易整数转十六进制字符串 (semihosting 调试输出)
 * ========================================================================== */
#ifdef USE_SEMIHOSTING
static void dbg_put_hex(uint32_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    int i;
    semihosting_putc('0');
    semihosting_putc('x');
    for (i = 28; i >= 0; i -= 4) {
        semihosting_putc(hex[(val >> i) & 0xF]);
    }
}

static void dbg_put_dec(uint32_t val)
{
    char buf[12];
    int i = 0;
    if (val == 0) {
        semihosting_putc('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        semihosting_putc(buf[--i]);
    }
}
#endif /* USE_SEMIHOSTING */

/* ==========================================================================
 * sys_tick_init — 配置 ARM SysTick 为 FreeRTOS tick
 * ========================================================================== */
void sys_tick_init(void)
{
    /* SysTick 控制寄存器: 0xE000E010
     * SysTick 重载寄存器:  0xE000E014
     * SysTick 当前值寄存器: 0xE000E018
     *
     * 时钟源 = CPU 时钟 (不使用 /8 分频)
     * 重载值 = (CPU_CLK / TICK_RATE) - 1
     *        = (600,000,000 / 1000) - 1
     *        = 599,999
     */
    volatile uint32_t *syst_csr  = (volatile uint32_t *)0xE000E010UL;
    volatile uint32_t *syst_rvr  = (volatile uint32_t *)0xE000E014UL;
    volatile uint32_t *syst_cvr  = (volatile uint32_t *)0xE000E018UL;

    uint32_t reload = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - 1;

    *syst_cvr = 0;          /* 清零当前值 */
    *syst_rvr = reload;     /* 设置重载值 */
    /* 不使能 SysTick — FreeRTOS vTaskStartScheduler() 会通过
     * vPortSetupTimerInterrupt() 在合适的时机使能 */
}

/* ==========================================================================
 * vApplicationStackOverflowHook — FreeRTOS 栈溢出钩子
 * ========================================================================== */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    DBG_PRINT("ERROR: Stack overflow in task: ");
#ifndef USE_SEMIHOSTING
    DBG_PRINT(pcTaskName);
#else
    semihosting_write(pcTaskName);
#endif
    DBG_PRINT("\r\n");
    /* 死循环 — 在 QEMU 中可用 GDB 检查 */
    while (1) { }
}

/* ==========================================================================
 * vApplicationMallocFailedHook — FreeRTOS 内存分配失败钩子
 * ========================================================================== */
void vApplicationMallocFailedHook(void)
{
    DBG_PRINT("ERROR: Malloc failed\r\n");
    while (1) { }
}

/* ==========================================================================
 * main — 入口
 * ========================================================================== */
int main(void)
{
    DBG_PRINT("\r\n=== i.MX RT1180 Network Firmware (M3) ===\r\n");
    DBG_PRINT("Build: " __DATE__ " " __TIME__ "\r\n");
    DBG_PRINT("Stack: FreeRTOS 10.6.2 + lwIP 2.2.1\r\n");

    /* ── 1. 配置 SysTick ── */
    sys_tick_init();
    DBG_PRINT("[INIT] SysTick configured (1 kHz)\r\n");

    /* ── 2. 创建 lwIP 初始化任务 ── */
    BaseType_t ret = xTaskCreate(
        lwip_init_task,         /* 任务函数 */
        "lwip_init",            /* 任务名称 */
        1024,                   /* 栈大小 (words) */
        NULL,                   /* 参数 */
        2,                      /* 优先级 */
        NULL
    );
    if (ret != pdPASS) {
        DBG_PRINT("ERROR: Failed to create lwip_init_task\r\n");
        while (1) { }
    }

    DBG_PRINT("[INIT] Starting FreeRTOS scheduler...\r\n");

    /* ── 3. 启动 FreeRTOS 调度器 ──
     * 此调用不会返回。调度器开始运行 lwip_init_task。
     */
    vTaskStartScheduler();

    /* 不应该到达这里 */
    while (1) { }
    return 0;
}

/* ==========================================================================
 * test_heartbeat_task — 验证调度器是否工作的简单任务
 * ========================================================================== */
void test_heartbeat_task(void *arg)
{
    (void)arg;
    DBG_PRINT("[TEST] Heartbeat task running!\r\n");
    int count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  /* 等 1 秒 */
        count++;
        /* 每 5 秒输出一次 */
        if ((count % 5) == 0) {
            DBG_PRINT("[TEST] Alive: 5 more seconds\r\n");
        }
    }
}

/* ==========================================================================
 * lwip_init_task — lwIP 协议栈初始化任务
 *
 * 此任务在 FreeRTOS 调度器启动后最先运行。
 * 负责:
 *   1. 初始化 ENET 驱动 (imxrt_enet_init)
 *   2. 初始化 lwIP TCP/IP 协议栈
 *   3. 启动 DHCP 获取 IP 地址
 *   4. 等待 IP 就绪后创建 HTTP Server 任务
 *   5. 自身进入主循环 (处理 lwIP 定时器)
 * ========================================================================== */

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "drivers/imxrt_enet.h"

/* 全局 netif — lwIP 网络接口 */
static struct netif g_enet_netif;

void lwip_init_task(void *arg)
{
    ip4_addr_t ipaddr, netmask, gw;
    uint8_t mac[6];
    err_t err;

    (void)arg;

    DBG_PRINT("[LWIP] Starting lwIP init...\r\n");

    /* ── 1. 获取 MAC 地址 ── */
    extern void BAL_ENET_GetMACAddr(uint8_t *mac);
    BAL_ENET_GetMACAddr(mac);
    DBG_PRINT("[LWIP] MAC: ");
#ifdef USE_SEMIHOSTING
    dbg_put_hex((uint32_t)mac[0] << 16 | (uint32_t)mac[1] << 8 | mac[2]);
    semihosting_putc(':');
    dbg_put_hex((uint32_t)mac[3] << 16 | (uint32_t)mac[4] << 8 | mac[5]);
#endif
    DBG_PRINT("\r\n");

    /* ── 2. 初始化 ENET 硬件 ── */
    DBG_PRINT("[LWIP] Initializing ENET...\r\n");
    err = imxrt_enet_init(&g_enet_netif);
    if (err != 0) {
        DBG_PRINT("ERROR: ENET init failed\r\n");
        while (1) { }
    }
    DBG_PRINT("[LWIP] ENET initialized\r\n");

    /* ── 3. 初始化 lwIP TCP/IP 栈 ── */
    lwip_init();
    DBG_PRINT("[LWIP] lwIP stack initialized\r\n");

    /* ── 4. 配置 IP ── */
#if LWIP_IPADDR_STATIC
    IP4_ADDR(&ipaddr,
             LWIP_IPADDR0, LWIP_IPADDR1, LWIP_IPADDR2, LWIP_IPADDR3);
    IP4_ADDR(&netmask,
             LWIP_NETMASK0, LWIP_NETMASK1, LWIP_NETMASK2, LWIP_NETMASK3);
    IP4_ADDR(&gw,
             LWIP_GW0, LWIP_GW1, LWIP_GW2, LWIP_GW3);

    netif_set_addr(&g_enet_netif, &ipaddr, &netmask, &gw);
    DBG_PRINT("[LWIP] Static IP: %d.%d.%d.%d\r\n",
              LWIP_IPADDR0, LWIP_IPADDR1, LWIP_IPADDR2, LWIP_IPADDR3);
#else
    /* DHCP 配置 */
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
    IP4_ADDR(&gw, 0, 0, 0, 0);

    netif_set_addr(&g_enet_netif, &ipaddr, &netmask, &gw);
    netif_set_up(&g_enet_netif);

    err = dhcp_start(&g_enet_netif);
    if (err != ERR_OK) {
        DBG_PRINT("[LWIP] DHCP start failed, trying static fallback...\r\n");
        IP4_ADDR(&ipaddr, 10, 0, 2, 15);
        IP4_ADDR(&netmask, 255, 255, 255, 0);
        IP4_ADDR(&gw, 10, 0, 2, 2);
        netif_set_addr(&g_enet_netif, &ipaddr, &netmask, &gw);
    } else {
        DBG_PRINT("[LWIP] DHCP started, waiting for IP...\r\n");

        /* 等待 DHCP 获取 IP (最多等 30 秒) */
        int dhcp_retries = 300;
        while (g_enet_netif.ip_addr.addr == 0 && dhcp_retries-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (g_enet_netif.ip_addr.addr == 0) {
            DBG_PRINT("[LWIP] DHCP timeout, using static fallback\r\n");
            dhcp_stop(&g_enet_netif);
            IP4_ADDR(&ipaddr, 10, 0, 2, 15);
            IP4_ADDR(&netmask, 255, 255, 255, 0);
            IP4_ADDR(&gw, 10, 0, 2, 2);
            netif_set_addr(&g_enet_netif, &ipaddr, &netmask, &gw);
        } else {
            DBG_PRINT("[LWIP] DHCP: IP acquired\r\n");
        }
    }
#endif

    /* ── 5. 打印网络状态 ── */
    DBG_PRINT("[LWIP] IP: ");
#ifdef USE_SEMIHOSTING
    dbg_put_dec(ip4_addr1(&g_enet_netif.ip_addr));
    semihosting_putc('.');
    dbg_put_dec(ip4_addr2(&g_enet_netif.ip_addr));
    semihosting_putc('.');
    dbg_put_dec(ip4_addr3(&g_enet_netif.ip_addr));
    semihosting_putc('.');
    dbg_put_dec(ip4_addr4(&g_enet_netif.ip_addr));
#endif
    DBG_PRINT("\r\n");

    /* ── 6. 创建 HTTP Server 任务 ── */
    {
        BaseType_t ret2 = xTaskCreate(
            httpd_task,             /* 任务函数 */
            "httpd",                /* 任务名称 */
            1024,                   /* 栈大小 (words) */
            &g_enet_netif,          /* 参数: netif */
            3,                      /* 优先级 */
            NULL                    /* 句柄 */
        );
        if (ret2 != pdPASS) {
            DBG_PRINT("ERROR: Failed to create httpd_task\r\n");
        } else {
            DBG_PRINT("[LWIP] HTTP Server started on port 80\r\n");
        }
    }

    DBG_PRINT("[LWIP] Init complete. Waiting for connections...\r\n");

    /* ── 7. 主循环: 刷新 lwIP 定时器和 ARP ── */
    while (1) {
        /* lwIP 在 bare metal 模式下需要定期调用这些函数
         * 但在 FreeRTOS 模式下, tcpip_thread 自动处理
         * 这里留空, 仅作为保活循环
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
