/**
 * gpio_demo.c — GPIO 输出翻转测试
 *
 * 通过 GPIO1 控制器翻转引脚输出, 验证 GPIO 设备模型。
 *
 * GPIO1 基地址: 0x4012C000 (来自 imx-rt1180 skill)
 * 寄存器:
 *   PDOR = 0x00  (Port Data Output — 输出值)
 *   PSOR = 0x04  (Port Set Output — 写 1 置位)
 *   PCOR = 0x08  (Port Clear Output — 写 1 清零)
 *   PTOR = 0x0C  (Port Toggle Output — 写 1 翻转)
 *   PDIR = 0x10  (Port Data Input — 只读)
 *   PDDR = 0x14  (Port Data Direction — 1=输出)
 *
 * 本 demo 操作 GPIO1 的 bit 0 (假设是板载 LED 或测试点),
 * 通过 PTOR 翻转输出电平。
 */

#include <stdint.h>
#include "config.h"

/* GPIO1 基地址 */
#define GPIO1_BASE      0x4012C000UL

/* 寄存器偏移 */
#define GPIO_PDOR  0x00
#define GPIO_PSOR  0x04
#define GPIO_PCOR  0x08
#define GPIO_PTOR  0x0C
#define GPIO_PDIR  0x10
#define GPIO_PDDR  0x14

/* 宏: 寄存器指针 */
#define GPIO_REG(off)   (*(volatile uint32_t *)(GPIO1_BASE + (off)))

/* 测试引脚: GPIO1[0] */
#define TEST_PIN        0

/**
 * gpio_init — 配置 GPIO1 引脚为输出
 */
static void gpio_init(void)
{
    /* 设置 bit 0 为输出 */
    GPIO_REG(GPIO_PDDR) |= (1UL << TEST_PIN);

    /* 初始输出低电平 */
    GPIO_REG(GPIO_PCOR) = (1UL << TEST_PIN);
}

/**
 * gpio_toggle — 翻转指定引脚
 */
static void gpio_toggle(uint32_t pin)
{
    GPIO_REG(GPIO_PTOR) = (1UL << pin);
}

/**
 * gpio_read — 读取引脚当前输出状态
 */
static uint32_t gpio_read_output(uint32_t pin)
{
    return (GPIO_REG(GPIO_PDOR) >> pin) & 1;
}

/**
 * simple_delay — 简易软件延时 (约 N 个核心时钟周期)
 *
 * @param cycles  延时周期数 (CORE_CLOCK=600MHz 下, 600M ≈ 1 秒)
 */
static void simple_delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm__ volatile ("nop");
    }
}

/**
 * gpio_demo — GPIO 翻转演示
 *
 * 翻转 GPIO1[0] 10 次, 每次延时一小段时间 (模拟 LED 闪烁)。
 */
void gpio_demo(void)
{
    int i;

    DBG_PRINT("[GPIO] Starting GPIO toggle demo (GPIO1 bit 0)...\r\n");

    gpio_init();

    for (i = 0; i < 10; i++) {
        gpio_toggle(TEST_PIN);
        DBG_PRINT("[GPIO] Toggle #");
        /* 输出 i+1 (简易) */
        {
            char c = '0' + (i + 1);
            char s[2] = { c, '\0' };
            DBG_PRINT(s);
        }
        DBG_PRINT(" → ");
        {
            char c = gpio_read_output(TEST_PIN) ? '1' : '0';
            char s[2] = { c, '\0' };
            DBG_PRINT(s);
        }
        DBG_PRINT("\r\n");

        /* 延时: 约 60M 周期 ≈ 100ms @ 600MHz */
        simple_delay(60000000);
    }

    /* 恢复低电平 */
    GPIO_REG(GPIO_PCOR) = (1UL << TEST_PIN);

    DBG_PRINT("[GPIO] Demo complete.\r\n");
}
