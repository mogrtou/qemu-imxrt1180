/**
 * bal.c — 板级抽象层实现骨架
 *
 * 当前为 QEMU Phase 1 最小实现:
 *   - 时钟: 直接返回配置值, 无真实初始化
 *   - ENET: 空实现, 等待 QEMU ENET 设备模型就绪
 *   - 调试: 复用现有 semihosting / UART 路径
 */

#include "bal.h"

/* ==========================================================================
 * BAL 初始化总入口
 * ========================================================================== */
void BAL_Init(void)
{
    BAL_Clock_Init();
    BAL_Debug_Init();
    BAL_ENET_Init();
}

/* ==========================================================================
 * ENET — QEMU Phase 1 占位实现
 * ========================================================================== */
void BAL_ENET_Init(void)
{
    /* Phase 1 (QEMU): ENET 由 QEMU 设备模型初始化, 固件侧无需 pinmux/时钟配置 */
    /* Phase 2 (真实硬件): 配置 IOMUXC、CCM 时钟、复位 PHY */
    BAL_ENET_ResetPHY();
}

void BAL_ENET_ResetPHY(void)
{
    /* Phase 1 (QEMU): PHY 模型始终就绪, 无需 GPIO 复位 */
    /* Phase 2 (真实硬件): 通过 GPIO 拉低→延时→拉高 复位 DP83822 */
    if (PHY_RESET_GPIO >= 0) {
        /* 预留: GPIO 复位序列 */
    }
}

void BAL_ENET_GetMACAddr(uint8_t *mac)
{
    /*
     * QEMU 默认 MAC: 02:00:00:00:00:01
     * 真实硬件: 从 OTP/eFuse 或 Flash 配置区读取
     */
    mac[0] = 0x02;
    mac[1] = 0x00;
    mac[2] = 0x00;
    mac[3] = 0x00;
    mac[4] = 0x00;
    mac[5] = 0x01;
}

uint32_t BAL_ENET_GetClockHz(void)
{
    return ENET_CLOCK_HZ;
}

/* ==========================================================================
 * 时钟 — QEMU Phase 1 直接返回配置值
 * ========================================================================== */
void BAL_Clock_Init(void)
{
    /* Phase 1 (QEMU): 无时钟树, 直接使用配置值 */
    /* Phase 2 (真实硬件): 配置 CCM PLL + 分频器 */
}

uint32_t BAL_Clock_GetSystemCoreClock(void)
{
    return CORE_CLOCK_HZ;
}

uint32_t BAL_Clock_GetENETClock(void)
{
    return ENET_CLOCK_HZ;
}

/* ==========================================================================
 * 调试 — 复用现有路径
 * ========================================================================== */

/*
 * 调试输出函数在 main.c 中已有实现:
 *   semihosting_write() — USE_SEMIHOSTING=1 时
 *   uart_puts()         — USE_SEMIHOSTING=0 时
 *
 * BAL 层直接桥接到它们。
 */

extern void semihosting_write(const char *s);
extern void uart_puts(const char *s);

void BAL_Debug_Init(void)
{
    /*
     * semihosting: 无需初始化, QEMU 启动时即已就绪
     * UART: uart_init() 由 uart_demo.c 调用, 这里不重复
     */
}

void BAL_Debug_PutChar(char c)
{
    /* 简易实现: 构造单字符字符串 */
    char s[2] = { c, '\0' };
    BAL_Debug_PutString(s);
}

void BAL_Debug_PutString(const char *s)
{
#if DEBUG_USE_SEMIHOSTING
    semihosting_write(s);
#else
    uart_puts(s);
#endif
}
