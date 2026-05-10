/**
 * link.ld — i.MX RT1180 QEMU 固件链接脚本
 *
 * 内存布局 (基于 docs/interfaces.md §3.1):
 *   ITCM  0x0000_0000  256KB  向量表 + .text + .rodata
 *   DTCM  0x2000_0000  256KB  .data + .bss + 栈
 *   OCRAM 0x2020_0000  512KB  未来网络 buffer / BD 环 (本阶段暂未使用)
 *
 * 注意:
 *  - QEMU armv7m 容器要求向量表放在 0x00000000
 *  - 栈由向量表 entry 0 初始化, Reset_Handler 无需再设 SP
 *  - 无堆: 不使用 malloc / sbrk
 */

MEMORY
{
    ITCM  (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    DTCM  (rw)  : ORIGIN = 0x20000000, LENGTH = 256K
    OCRAM (rw)  : ORIGIN = 0x20200000, LENGTH = 512K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* ===== ITCM 区域 — 只读 ===== */

    .text : ALIGN(4)
    {
        KEEP(*(.vectors))          /* 向量表 — 必须位于 0x00000000 */
        *(.text .text.*)           /* 所有 .text 代码 */
        *(.rodata .rodata.*)       /* 只读数据 */
        . = ALIGN(4);
    } > ITCM

    /* ===== DTCM 区域 — 可读写 ===== */

    .data : ALIGN(4)
    {
        _data_vma = .;             /* VMA 起始 (DTCM) */
        *(.data .data.*)           /* 已初始化全局/静态变量 */
        . = ALIGN(4);
        _data_end = .;
    } > DTCM AT > ITCM
    _data_lma = LOADADDR(.data);   /* LMA (ITCM 中的加载地址) */

    .bss (NOLOAD) : ALIGN(4)
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > DTCM

    /* 栈底 = DTCM 末尾 (满递减栈, SP 初始由向量表指向栈顶+1) */
    _estack = ORIGIN(DTCM) + LENGTH(DTCM);

    /* ===== 堆 (FreeRTOS heap_4.c 使用, Phase 2) ===== */
    .heap (NOLOAD) : ALIGN(8)
    {
        _heap_start = .;
        . += 32K;                  /* FreeRTOS configTOTAL_HEAP_SIZE */
        . = ALIGN(8);
        _heap_end = .;
    } > DTCM

    /* ===== OCRAM 预留 (Phase 2 网络 buffer / BD 环) ===== */
    /* 本阶段不放置任何 section */
}
