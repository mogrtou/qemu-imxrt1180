/**
 * uart_demo.c — LPUART1 "Hello World" 测试
 *
 * 通过 LPUART1 (0x4007_0000) 输出字符, 验证:
 *   1. MMIO 寄存器访问正常
 *   2. LPUART 设备模型工作正常
 *   3. QEMU chardev 后端连接正常
 *
 * 寄存器地址来自 docs/interfaces.md 和 imx-rt1180 skill:
 *   LPUART1_BASE = 0x40070000
 *   STAT  = 0x0C  (TDRE bit 23)
 *   CTRL  = 0x10  (TE bit 19)
 *   DATA  = 0x18
 *   BAUD  = 0x28
 */

#include <stdint.h>

/* LPUART1 基地址 (见 docs/interfaces.md §3.1 & imx-rt1180 skill) */
#define LPUART1_BASE  0x40070000UL

/* 寄存器偏移 */
#define LPUART_VERID  0x00
#define LPUART_PARAM  0x04
#define LPUART_STAT   0x0C
#define LPUART_CTRL   0x10
#define LPUART_DATA   0x18
#define LPUART_BAUD   0x28

/* STAT 位 */
#define LPUART_STAT_TDRE  (1UL << 23)  /* Transmit Data Register Empty */

/* CTRL 位 */
#define LPUART_CTRL_TE    (1UL << 19)  /* Transmitter Enable */
#define LPUART_CTRL_RE    (1UL << 18)  /* Receiver Enable */

/* BAUD 寄存器: OSR=15, SBR = (clock / (osr+1) / baud) */
/* 假设 LPUART 时钟 = 24MHz, 目标 115200 baud, OSR=15 → SBR ≈ 13 */
#define LPUART_OSR        15
#define LPUART_SBR        13   /* 24MHz / (15+1) / 115200 ≈ 13 */

/**
 * uart_init — 初始化 LPUART1
 *
 * 配置波特率 115200, 8N1, 使能发送器。
 */
static void uart_init(void)
{
    volatile uint32_t *baud = (volatile uint32_t *)(LPUART1_BASE + LPUART_BAUD);
    volatile uint32_t *ctrl = (volatile uint32_t *)(LPUART1_BASE + LPUART_CTRL);

    /* 配置波特率: OSR[12:8]=15, SBR[7:0]=13 */
    *baud = ((LPUART_OSR & 0x1F) << 8) | (LPUART_SBR & 0xFF);

    /* 使能发送器和接收器 */
    *ctrl = LPUART_CTRL_TE | LPUART_CTRL_RE;
}

/**
 * uart_putc — 阻塞发送一个字符
 */
static void uart_putc(char c)
{
    volatile uint32_t *stat = (volatile uint32_t *)(LPUART1_BASE + LPUART_STAT);
    volatile uint32_t *data = (volatile uint32_t *)(LPUART1_BASE + LPUART_DATA);

    /* 等待 TDRE (Transmit Data Register Empty) */
    while (!(*stat & LPUART_STAT_TDRE)) {
        /* spin */
    }

    /* 写入数据寄存器 */
    *data = (uint32_t)c;
}

/**
 * uart_puts — 阻塞发送字符串 (供 DBG_PRINT 使用)
 */
void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

/**
 * uart_demo — UART Hello World
 *
 * 用法: 在 main.c 中调用, 或通过 config.h ENABLE_UART_DEMO 控制。
 */
void uart_demo(void)
{
    uart_init();

    uart_puts("\r\n");
    uart_puts("========================================\r\n");
    uart_puts("  i.MX RT1180 UART Demo (LPUART1)\r\n");
    uart_puts("========================================\r\n");
    uart_puts("\r\n");

    uart_puts("Hello from i.MX RT1180 Cortex-M7!\r\n");
    uart_puts("LPUART1 @ 0x40070000, 115200 8N1\r\n");

    /* 回显测试: 如果启用了接收, 可在此添加回显循环 */
    uart_puts("UART demo complete.\r\n");
}
