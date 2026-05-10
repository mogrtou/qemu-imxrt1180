/**
 * @file config.h
 * @brief 固件编译期配置 — 控制 semihosting / 调试开关等
 *
 * 修改此文件切换调试方式或启用/禁用功能模块。
 * 无需修改源码即可适配不同调试场景。
 */

#ifndef CONFIG_H
#define CONFIG_H

/* --------------------------------------------------------------------------
 * 调试输出方式选择
 *
 * USE_SEMIHOSTING=1: 通过 ARM semihosting BKPT 指令输出调试信息
 *                    优点: 无 UART 依赖, 适合 QEMU 早期开发
 *                    需要 QEMU 启动时加 -semihosting 标志
 *
 * USE_SEMIHOSTING=0: 通过 LPUART1 (0x4007_0000) 输出调试信息
 *                    适合真实硬件或不需要 semihosting 的场景
 * -------------------------------------------------------------------------- */
#define USE_SEMIHOSTING  1

/* --------------------------------------------------------------------------
 * 功能开关 — 控制编译哪些 demo 模块
 * -------------------------------------------------------------------------- */
#define ENABLE_UART_DEMO      1   /* UART Hello World */
#define ENABLE_SYSTICK_DEMO   1   /* SysTick 周期性中断 */
#define ENABLE_GPIO_DEMO      1   /* GPIO 翻转测试 */

/* --------------------------------------------------------------------------
 * 调试宏定义
 * -------------------------------------------------------------------------- */
/* 前向声明 — 无论哪种模式编译所有文件都需要 */
#ifdef USE_SEMIHOSTING
    void semihosting_write(const char *s);
    #define DBG_PRINT(s)  semihosting_write(s)
#else
    void uart_puts(const char *s);
    #define DBG_PRINT(s)  uart_puts(s)
#endif

#endif /* CONFIG_H */
