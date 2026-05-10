/**
 * systick_demo.c — SysTick 周期性中断测试
 *
 * 利用 ARM Cortex-M7 内置 SysTick 定时器产生周期性中断,
 * 在中断服务例程中通过 semihosting 或 UART 输出心跳信息。
 *
 * SysTick 是 ARMv7-M 私有外设, 寄存器映射在:
 *   SYST_CSR   = 0xE000E010  (Control and Status)
 *   SYST_RVR   = 0xE000E014  (Reload Value)
 *   SYST_CVR   = 0xE000E018  (Current Value)
 *   SYST_CALIB = 0xE000E01C  (Calibration)
 *
 * 时钟源: 核心时钟 (CORE_CLOCK = 600MHz per docs/interfaces.md §3.4)
 *  或外部参考时钟 (STCLK), 由 CSR CLKSOURCE 位选择。
 *  本 demo 使用核心时钟 (CLKSOURCE = 1)。
 *
 * 目标: 每 1 秒产生一次中断 → 输出 "." 表示心跳。
 */

#include <stdint.h>
#include "config.h"

/* SysTick 寄存器 */
#define SYST_BASE       0xE000E010UL
#define SYST_CSR        (*(volatile uint32_t *)(SYST_BASE + 0x00))
#define SYST_RVR        (*(volatile uint32_t *)(SYST_BASE + 0x04))
#define SYST_CVR        (*(volatile uint32_t *)(SYST_BASE + 0x08))

/* SYST_CSR 位 */
#define SYST_CSR_ENABLE    (1UL << 0)
#define SYST_CSR_TICKINT   (1UL << 1)
#define SYST_CSR_CLKSOURCE (1UL << 2)
#define SYST_CSR_COUNTFLAG (1UL << 16)

/* 核心时钟: 600 MHz (与 docs/interfaces.md §3.4 一致) */
#define CORE_CLOCK_HZ  600000000UL

/*
 * SysTick 重装载值计算:
 *   RVL 寄存器仅 24 位 (最大 0x00FFFFFF = 16,777,215)。
 *   600 MHz 下无法用单次溢出覆盖 1 秒 (需 6 亿次计数)。
 *
 *   方案: 使用 10ms 子周期 (600 MHz × 0.01 = 6,000,000 次)
 *         SysTick 每 10ms 中断一次,
 *         ISR 中软件累加计数, 满 100 次 (1 秒) 输出心跳。
 */
#define SYSTICK_SUB_TICKS  100         /* 100 次子周期 = 1 秒 */
#define SYSTICK_RELOAD     (CORE_CLOCK_HZ / SYSTICK_SUB_TICKS - 1)
/* 600,000,000 / 100 - 1 = 5,999,999 (可在 24 位内表示) */

/* 心跳计数: 子周期累加器 */
static volatile uint32_t sub_tick_count = 0;
static volatile uint32_t tick_count     = 0;

/**
 * SysTick_Handler — 覆盖 startup.c 中的 weak 默认实现
 *
 * 每次 SysTick 中断调用一次。
 * 输出 "." 作为心跳指示, 每 10 次输出换行。
 */
void SysTick_Handler(void)
{
    sub_tick_count++;

    /* 每 100 次子周期 (10ms × 100 = 1 秒) 输出一次心跳 */
    if (sub_tick_count >= SYSTICK_SUB_TICKS) {
        sub_tick_count = 0;
        tick_count++;

        /* 每 10 次心跳输出换行 */
        if ((tick_count % 10) == 0) {
            DBG_PRINT(".\r\n");
        } else {
            DBG_PRINT(".");
        }
    }
}

/**
 * systick_init — 配置并启动 SysTick
 *
 * @param reload  重装载值 (计数值)
 */
static void systick_init(uint32_t reload)
{
    /* 清零当前值 */
    SYST_CVR = 0;

    /* 设置重装载值 */
    SYST_RVR = reload;

    /* 使能 SysTick: CLKSOURCE=1 (核心时钟), TICKINT=1, ENABLE=1 */
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

/**
 * systick_demo — SysTick 心跳演示
 *
 * 启动 SysTick 后, 主循环每 10 次心跳输出一次信息,
 * 5 秒后停止。
 */
void systick_demo(void)
{
    DBG_PRINT("[SysTick] Starting 1-second heartbeat...\r\n");

    systick_init(SYSTICK_RELOAD);

    /* 等待 5 次心跳 (5 秒) */
    while (tick_count < 5) {
        __asm__ volatile ("wfi");   /* 等待中断 */
    }

    /* 停止 SysTick */
    SYST_CSR = 0;

    DBG_PRINT("\r\n[SysTick] Demo complete. Total ticks: ");
    /* 输出 tick_count (简易数字输出) */
    {
        char buf[12];
        uint32_t n = tick_count;
        int i = 10;
        buf[11] = '\0';
        if (n == 0) {
            DBG_PRINT("0");
        } else {
            while (n > 0 && i >= 0) {
                buf[i--] = '0' + (n % 10);
                n /= 10;
            }
            /* 用 semihosting_write 输出 buf[i+1]; 简化处理 */
            DBG_PRINT(&buf[i + 1]);
        }
    }
    DBG_PRINT("\r\n");
}
