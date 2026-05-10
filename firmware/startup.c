/**
 * startup.c — i.MX RT1180 Cortex-M7 启动代码
 *
 * 职责:
 *  1. 定义向量表 (SP 初值 + 异常/中断入口)
 *  2. Reset_Handler: 复制 .data → DTCM, 清零 .bss, 调 main()
 *  3. Default_Handler: 所有未定义中断的默认处理 (死循环)
 *  4. SysTick_Handler: 声明为 weak, 由 systick_demo.c 覆盖
 *
 * 编译器: arm-none-eabi-gcc
 * 标准:   C11 -ffreestanding -nostdlib
 */

#include <stdint.h>

/* --------------------------------------------------------------------------
 * 链接脚本导出的符号
 * -------------------------------------------------------------------------- */
extern uint32_t _data_lma;    /* .data 的加载地址 (在 ITCM 内) */
extern uint32_t _data_vma;    /* .data 的运行地址 (在 DTCM 内) */
extern uint32_t _data_end;
extern uint32_t _bss_start;
extern uint32_t _bss_end;
extern uint32_t _estack;      /* 栈顶 (DTCM 末尾) */
extern uint32_t _heap_start;  /* FreeRTOS heap (link.ld 中定义) */
extern uint32_t _heap_end;

/* --------------------------------------------------------------------------
 * 函数声明
 * -------------------------------------------------------------------------- */
extern int main(void);
void SystemInit(void);

/* 默认中断处理 — 弱符号, 可被覆盖 */
void Default_Handler(void);
void Reset_Handler(void);
void ENET_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void NMI_Handler(void)           __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)           __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)       __attribute__((weak, alias("Default_Handler")));

/* --------------------------------------------------------------------------
 * 向量表 — 必须放在 .vectors section, 位于 0x00000000 (ITCM 入口)
 * Cortex-M7 异常顺序 (ARMv7-M):
 *   0: SP_main (栈顶)
 *   1: Reset
 *   2: NMI
 *   3: HardFault
 *   4: MemManage
 *   5: BusFault
 *   6: UsageFault
 *   7-10: Reserved
 *  11: SVCall
 *  12: DebugMonitor
 *  13: Reserved
 *  14: PendSV
 *  15: SysTick
 *  16+: External Interrupts (IRQ0 = NVIC IRQ 16)
 * -------------------------------------------------------------------------- */
__attribute__((section(".vectors"), used))
const uint32_t vector_table[] = {
    /* 0x0000 */ (uint32_t)&_estack,          /* SP 初始值 */
    /* 0x0004 */ (uint32_t)Reset_Handler,     /* Reset */
    /* 0x0008 */ (uint32_t)NMI_Handler,       /* NMI */
    /* 0x000C */ (uint32_t)HardFault_Handler, /* HardFault */
    /* 0x0010 */ (uint32_t)MemManage_Handler, /* MemManage */
    /* 0x0014 */ (uint32_t)BusFault_Handler,  /* BusFault */
    /* 0x0018 */ (uint32_t)UsageFault_Handler,/* UsageFault */
    /* 0x001C */ 0,                           /* Reserved */
    /* 0x0020 */ 0,                           /* Reserved */
    /* 0x0024 */ 0,                           /* Reserved */
    /* 0x0028 */ 0,                           /* Reserved */
    /* 0x002C */ (uint32_t)SVC_Handler,       /* SVCall */
    /* 0x0030 */ (uint32_t)DebugMon_Handler,  /* DebugMonitor */
    /* 0x0034 */ 0,                           /* Reserved */
    /* 0x0038 */ (uint32_t)PendSV_Handler,    /* PendSV */
    /* 0x003C */ (uint32_t)SysTick_Handler,   /* SysTick */

    /* ---- 外部中断 (IRQ 16+) ---- */
    /* IRQ  0-15: 系统异常, 已覆盖 */
    /* IRQ 16-111: 其他外设, 暂填 Default_Handler */
    /* 使用紧凑写法: 预留 96 个槽位 (IRQ 16 ~ IRQ 111) */

    /* NVIC IRQ 112-113: 占位 */
    [128] = (uint32_t)Default_Handler,   /* IRQ 112 */
    [129] = (uint32_t)Default_Handler,   /* IRQ 113 */

    /* NVIC IRQ 114 = ENET1 (来自 docs/interfaces.md §4) */
    [130] = (uint32_t)ENET_IRQHandler,   /* ENET1 IRQ (114) */

    /* NVIC IRQ 115-116: ENET1 1588 Timer / ENET2 (Phase 2) */
    [131] = (uint32_t)Default_Handler,   /* IRQ 115 — ENET1 1588 Timer */
    [132] = (uint32_t)Default_Handler,   /* IRQ 116 — ENET2 (Phase 2 预留) */
};

/* --------------------------------------------------------------------------
 * Reset_Handler — 上电/复位入口
 * -------------------------------------------------------------------------- */
void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* 1. 复制 .data 段: LMA (ITCM) → VMA (DTCM) */
    src = &_data_lma;
    dst = &_data_vma;
    while (dst < &_data_end) {
        *dst++ = *src++;
    }

    /* 2. 清零 .bss 段 */
    dst = &_bss_start;
    while (dst < &_bss_end) {
        *dst++ = 0;
    }

    /* 3. 可选: 初始化系统 (时钟、FPU 等) */
    SystemInit();

    /* 4. 跳转主函数 */
    main();

    /* 5. main() 不应返回; 若返回则死循环 */
    while (1) { }
}

/* --------------------------------------------------------------------------
 * SystemInit — 最小系统初始化 (在 main() 前调用)
 * -------------------------------------------------------------------------- */
void SystemInit(void)
{
    /* 使能 FPU (CPACR 寄存器) */
    /* CPACR 地址: 0xE000ED88, 设置 CP10/CP11 为完全访问 (0b11) */
    volatile uint32_t *cpacr = (volatile uint32_t *)0xE000ED88UL;
    *cpacr |= (0xF << 20);  /* CP10(bit21:20)=11, CP11(bit23:22)=11 */

    /* 后续可在此添加: 时钟初始化、MPU 配置等 */
}

/* --------------------------------------------------------------------------
 * Default_Handler — 未定义中断的默认处理
 * -------------------------------------------------------------------------- */
void Default_Handler(void)
{
    while (1) {
        /* 死循环 — 在 QEMU 中可用 GDB 中断检查 */
    }
}

/* ==========================================================================
 * 标准函数 — FreeRTOS 依赖 memset/memcpy (nostdlib 无 libc)
 * ========================================================================== */
void *memset(void *s, int c, unsigned int n)
{
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, unsigned int n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

int memcmp(const void *s1, const void *s2, unsigned int n)
{
    const unsigned char *a = s1, *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}

unsigned int strlen(const char *s)
{
    unsigned int n = 0;
    while (*s++) n++;
    return n;
}

int strncmp(const char *s1, const char *s2, unsigned int n)
{
    while (n--) {
        if (*s1 != *s2) return *s1 - *s2;
        if (!*s1) return 0;
        s1++; s2++;
    }
    return 0;
}

void *memmove(void *dest, const void *src, unsigned int n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

/* ── snprintf — 简易实现 (用于 httpd_task.c) ── */
int snprintf(char *buf, unsigned int size, const char *fmt, ...)
{
    char *p = buf;
    unsigned int left = size > 0 ? size - 1 : 0;
    const char *s = fmt;
    unsigned long ul_val;
    int i_val;
    const char *str_val;

    unsigned int *args = (unsigned int *)&fmt;
    int arg_idx = 1;

    while (*s && left > 0) {
        if (*s != '%') { *p++ = *s++; left--; continue; }
        s++;
        if (*s == '%') { *p++ = '%'; s++; left--; continue; }
        switch (*s) {
        case 's':
            str_val = (const char *)args[arg_idx++];
            if (!str_val) str_val = "(null)";
            while (*str_val && left > 0) { *p++ = *str_val++; left--; }
            break;
        case 'l':
            s++;
            if (*s == 'u') {
                ul_val = (unsigned long)args[arg_idx++];
                char tmp[20];
                int ti = 0;
                if (ul_val == 0) tmp[ti++] = '0';
                while (ul_val > 0 && ti < 19) { tmp[ti++] = '0' + (ul_val % 10); ul_val /= 10; }
                while (ti > 0 && left > 0) { *p++ = tmp[--ti]; left--; }
            }
            break;
        case 'd':
            i_val = (int)args[arg_idx++];
            if (i_val < 0) { if (left > 0) { *p++ = '-'; left--; } i_val = -i_val; }
            {
                char tmp[20];
                int ti = 0;
                if (i_val == 0) tmp[ti++] = '0';
                while (i_val > 0 && ti < 19) { tmp[ti++] = '0' + (i_val % 10); i_val /= 10; }
                while (ti > 0 && left > 0) { *p++ = tmp[--ti]; left--; }
            }
            break;
        default:
            if (left > 0) { *p++ = '?'; left--; }
            break;
        }
        s++;
    }
    *p = '\0';
    return (int)(p - buf);
}

/* ── atoi — 字符串转整数 (lwIP netif_find 需要) ── */
int atoi(const char *s)
{
    int val = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return sign * val;
}
