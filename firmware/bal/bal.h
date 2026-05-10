/**
 * bal.h — 板级抽象层 (Board Abstraction Layer) 公共接口
 *
 * 所有板级初始化、外设访问、调试输出均通过 BAL 间接调用。
 * 编译时根据 BOARD 宏切换板级配置:
 *   make BOARD=evk    → 使用 bal/config/evk_config.h
 *   make BOARD=custom → 使用 bal/config/custom_config.h
 *
 * 来源: docs/interfaces.md §3.3 板级抽象层接口
 */

#ifndef BAL_H
#define BAL_H

#include <stdint.h>

/* ==========================================================================
 * 板级配置引入
 * ========================================================================== */
#ifdef BOARD_CONFIG
#include BOARD_CONFIG
#else
#include "bal/config/evk_config.h"
#endif

/* ==========================================================================
 * BAL 初始化
 * ========================================================================== */

/**
 * BAL_Init — 板级初始化总入口
 *
 * 调用顺序:
 *   BAL_Clock_Init() → BAL_Debug_Init() → BAL_ENET_Init()
 *
 * 由 main() 在早期调用。
 */
void BAL_Init(void);

/* ==========================================================================
 * ENET 相关
 * ========================================================================== */

/**
 * BAL_ENET_Init — 配置 ENET pinmux、时钟、PHY 复位时序
 */
void BAL_ENET_Init(void);

/**
 * BAL_ENET_ResetPHY — 硬件复位 DP83822 PHY (GPIO 拉低→延时→拉高)
 */
void BAL_ENET_ResetPHY(void);

/**
 * BAL_ENET_GetMACAddr — 获取 MAC 地址
 *
 * 优先级:
 *   1. OTP/eFuse (真实硬件)
 *   2. 编译期配置 (QEMU 默认)
 *
 * @param mac  输出: 6 字节 MAC 地址
 */
void BAL_ENET_GetMACAddr(uint8_t *mac);

/**
 * BAL_ENET_GetClockHz — 返回 ENET 外设时钟频率 (Hz)
 */
uint32_t BAL_ENET_GetClockHz(void);

/* ==========================================================================
 * 时钟
 * ========================================================================== */

/**
 * BAL_Clock_Init — 初始化系统时钟树
 *
 * Phase 1 (QEMU): 空实现, QEMU 无时钟树
 * Phase 2 (真实硬件): 配置 PLL, 设置分频器
 */
void BAL_Clock_Init(void);

/**
 * BAL_Clock_GetSystemCoreClock — 返回系统核心时钟 (Hz)
 */
uint32_t BAL_Clock_GetSystemCoreClock(void);

/**
 * BAL_Clock_GetENETClock — 返回 ENET 外设时钟 (Hz)
 */
uint32_t BAL_Clock_GetENETClock(void);

/* ==========================================================================
 * 调试
 * ========================================================================== */

/**
 * BAL_Debug_Init — 初始化调试通道
 *
 * 根据 DEBUG_USE_SEMIHOSTING 选择:
 *   1: semihosting (QEMU)
 *   0: LPUART1
 */
void BAL_Debug_Init(void);

/**
 * BAL_Debug_PutChar — 输出单个字符
 */
void BAL_Debug_PutChar(char c);

/**
 * BAL_Debug_PutString — 输出字符串
 */
void BAL_Debug_PutString(const char *s);

#endif /* BAL_H */
