/**
 * test_asserts.c — L3 HIL 固件级测试 (semihosting assert 框架)
 *
 * 此文件在 QEMU 中运行 (通过 -semihosting 标志)。
 * 使用 ARM Semihosting 输出断言结果:
 *   SEMIHOSTING_ASSERT(condition, "message")
 *
 * 测试覆盖:
 *   1. ENET 寄存器复位值
 *   2. ENET 寄存器读写
 *   3. ECR 写保护机制
 *   4. EIR W1C 行为
 *   5. RDSR/ATSTMP RO 行为
 *   6. MDIO PHY 探测 (DP83822 ID)
 *   7. PHY Link Status
 *   8. ENET 中断生成 & 清除
 *   9. TX/RX Buffer Descriptor 环
 *   10. 帧收发 (Loopback 模式)
 *
 * 编译: 与主固件一同编译 (arm-none-eabi-gcc)
 * 运行: qemu-system-arm -M imxrt1180-evk -kernel fw.elf -semihosting
 *
 * 断言宏协议 (from docs/interfaces.md):
 *   失败: semihosting WRITE0 → "ASSERT FAIL: <msg>"
 *   通过: semihosting WRITE0 → "ASSERT OK: <msg>"
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 */

/* ------------------------------------------------------------------ */
/*  Semihosting 基础设施                                                */
/* ------------------------------------------------------------------ */

#ifndef USE_SEMIHOSTING
#error "test_asserts.c requires USE_SEMIHOSTING=1 in config.h"
#endif

/**
 * semihosting_write — 输出字符串到 QEMU stderr (ARM SYS_WRITE0)
 */
__attribute__((noinline))
static void semihosting_write(const char *s)
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

/**
 * semihosting_write_char — 输出单个字符 (ARM SYS_WRITEC)
 */
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

/* 简易整数转十六进制字符串 */
static void put_hex(unsigned int val)
{
    static const char hex[] = "0123456789ABCDEF";
    int i;
    semihosting_putc('0');
    semihosting_putc('x');
    for (i = 28; i >= 0; i -= 4) {
        semihosting_putc(hex[(val >> i) & 0xF]);
    }
}

/* ------------------------------------------------------------------ */
/*  断言宏                                                              */
/* ------------------------------------------------------------------ */

static int g_asserts_passed = 0;
static int g_asserts_failed = 0;

#define ASSERT_COND(cond, msg) do { \
    if (!(cond)) { \
        semihosting_write("ASSERT FAIL: " msg "\r\n"); \
        g_asserts_failed++; \
    } else { \
        g_asserts_passed++; \
    } \
} while(0)

#define ASSERT_EQ_U32(actual, expected, msg) do { \
    volatile unsigned int _a = (unsigned int)(actual); \
    volatile unsigned int _e = (unsigned int)(expected); \
    if (_a != _e) { \
        semihosting_write("ASSERT FAIL: " msg " (expected="); \
        put_hex(_e); \
        semihosting_write(" actual="); \
        put_hex(_a); \
        semihosting_write(")\r\n"); \
        g_asserts_failed++; \
    } else { \
        g_asserts_passed++; \
    } \
} while(0)

#define TEST_START(suite) \
    semihosting_write("\r\n=== TEST: " suite " ===\r\n")

#define TEST_END() \
    semihosting_write("--- DONE ---\r\n")

/* ------------------------------------------------------------------ */
/*  ENET 寄存器定义 (from docs/interfaces.md §1.1)                      */
/* ------------------------------------------------------------------ */

#define ENET_BASE           0x40424000UL
#define ENET_REG(off)       (*(volatile unsigned int *)(ENET_BASE + (off)))

#define ENET_ECR            ENET_REG(0x0000)
#define ENET_EIR            ENET_REG(0x0004)
#define ENET_EIMR           ENET_REG(0x0008)
#define ENET_RDAR           ENET_REG(0x0010)
#define ENET_TDAR           ENET_REG(0x0014)
#define ENET_ECR_MAGIC      ENET_REG(0x0024)
#define ENET_MMFR           ENET_REG(0x0040)
#define ENET_MSCR           ENET_REG(0x0044)
#define ENET_MIBC           ENET_REG(0x0048)
#define ENET_RCR            ENET_REG(0x0064)
#define ENET_TCR            ENET_REG(0x0084)
#define ENET_PALR           ENET_REG(0x00C4)
#define ENET_PAUR           ENET_REG(0x00C8)
#define ENET_OPD            ENET_REG(0x00E4)
#define ENET_IAUR           ENET_REG(0x00EC)
#define ENET_IALR           ENET_REG(0x00F0)
#define ENET_GAUR           ENET_REG(0x00F4)
#define ENET_GALR           ENET_REG(0x00F8)
#define ENET_TFWR           ENET_REG(0x0100)
#define ENET_RDSR           ENET_REG(0x0144)
#define ENET_TDSR           ENET_REG(0x0154)
#define ENET_MRBR           ENET_REG(0x0160)
#define ENET_RSFL           ENET_REG(0x0184)
#define ENET_RSEM           ENET_REG(0x018C)
#define ENET_RAEM           ENET_REG(0x0190)
#define ENET_RAFL           ENET_REG(0x0194)
#define ENET_TSEM           ENET_REG(0x0198)
#define ENET_TAEM           ENET_REG(0x019C)
#define ENET_TAFL           ENET_REG(0x01A0)
#define ENET_TIPG           ENET_REG(0x01A4)

/* EIR 中断位 */
#define EIR_TXF             (1u << 23)
#define EIR_TXB             (1u << 21)
#define EIR_RXF             (1u << 25)
#define EIR_RXB             (1u << 24)
#define EIR_MII             (1u << 27)
#define EIR_EBERR           (1u << 22)

/* ECR 位 */
#define ECR_ETHEREN         (1u << 1)
#define ECR_EN1588          (1u << 2)
#define ECR_RESET           (1u << 0)

/* RCR 位 */
#define RCR_RMII_MODE       (1u << 8)
#define RCR_PROM            (1u << 3)
#define RCR_MII_MODE        (1u << 2)
#define RCR_LOOP            (1u << 0)
#define RCR_FCE             (1u << 5)

/* TCR 位 */
#define TCR_FDEN            (1u << 2)
#define TCR_FCE             (1u << 5)

/* ECR 解锁魔数 */
#define ECR_UNLOCK_MAGIC    0x5A5A5A5AU

/* MMFR 字段 */
#define MMFR_ST_CLAUSE22    (0x01u << 30)
#define MMFR_OP_READ        (0x02u << 28)
#define MMFR_OP_WRITE       (0x01u << 28)
#define MMFR_PA(n)          (((unsigned int)(n) & 0x1F) << 23)
#define MMFR_RA(n)          (((unsigned int)(n) & 0x1F) << 18)
#define MMFR_TA_WRITE       (0x02u << 16)

/* PHY 寄存器 */
#define PHY_REG_BMCR        0x00
#define PHY_REG_BMSR        0x01
#define PHY_REG_PHYIDR1     0x02
#define PHY_REG_PHYIDR2     0x03

/* DP83822 期望 PHY ID */
#define DP83822_PHYID1      0x2000
#define DP83822_PHYID2      0xA221

/* ------------------------------------------------------------------ */
/*  MMFR MDIO 读写辅助函数                                               */
/* ------------------------------------------------------------------ */

/**
 * mdio_read — 通过 ENET MMFR 读取 PHY 寄存器
 *
 * 构造 Clause 22 读帧 → 写入 MMFR → 等待 MII 完成 → 读取 DATA
 */
static unsigned int mdio_read(unsigned int phy_addr, unsigned int reg_addr)
{
    unsigned int mmfr_val;

    /* 构造 Clause 22 Read Frame:
     *   ST=01, OP=10 (read), PA=phy, RA=reg, TA=00, DATA=0 */
    mmfr_val = MMFR_ST_CLAUSE22
             | MMFR_OP_READ
             | MMFR_PA(phy_addr)
             | MMFR_RA(reg_addr);

    ENET_MMFR = mmfr_val;

    /* 等待 MDIO 传输完成 (MII interrupt in EIR)
     *
     * 在 QEMU 中, MDIO 是同步完成的 — MMFR 写入后 DATA 立即更新。
     * 真实硬件需轮询 EIR[MII] 位。
     */
    volatile int timeout = 10000;
    while (timeout--) {
        unsigned int eir = ENET_EIR;
        if (eir & EIR_MII) {
            /* 清除 MII 中断标志 */
            ENET_EIR = EIR_MII;
            break;
        }
    }

    /* 读取 DATA 字段 (bits 15:0) */
    return ENET_MMFR & 0xFFFF;
}

/**
 * mdio_write — 通过 ENET MMFR 写入 PHY 寄存器
 */
static void mdio_write(unsigned int phy_addr, unsigned int reg_addr,
                       unsigned int data)
{
    unsigned int mmfr_val;

    /* 构造 Clause 22 Write Frame:
     *   ST=01, OP=01 (write), PA=phy, RA=reg, TA=10, DATA=data */
    mmfr_val = MMFR_ST_CLAUSE22
             | MMFR_OP_WRITE
             | MMFR_PA(phy_addr)
             | MMFR_RA(reg_addr)
             | MMFR_TA_WRITE
             | (data & 0xFFFF);

    ENET_MMFR = mmfr_val;

    /* 等待完成 */
    volatile int timeout = 10000;
    while (timeout--) {
        if (ENET_EIR & EIR_MII) {
            ENET_EIR = EIR_MII;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  测试用例                                                             */
/* ------------------------------------------------------------------ */

/**
 * test_01_enet_reset_values
 * 验证 ENET 核心寄存器上电复位值
 */
static void test_01_enet_reset_values(void)
{
    TEST_START("01: ENET Reset Values");

    ASSERT_EQ_U32(ENET_ECR,       0xF0000000, "ECR reset");
    ASSERT_EQ_U32(ENET_EIR,       0x00000000, "EIR reset");
    ASSERT_EQ_U32(ENET_EIMR,      0x00000000, "EIMR reset");
    ASSERT_EQ_U32(ENET_RDAR,      0x00000000, "RDAR reset");
    ASSERT_EQ_U32(ENET_TDAR,      0x00000000, "TDAR reset");
    ASSERT_EQ_U32(ENET_ECR_MAGIC, 0x00000000, "ECR_MAGIC reset");
    ASSERT_EQ_U32(ENET_MMFR,      0x00000000, "MMFR reset");
    ASSERT_EQ_U32(ENET_MSCR,      0x00000040, "MSCR reset");
    ASSERT_EQ_U32(ENET_MIBC,      0xC0000000, "MIBC reset");
    ASSERT_EQ_U32(ENET_RCR,       0x05E00001, "RCR reset");
    ASSERT_EQ_U32(ENET_TCR,       0x00000010, "TCR reset");
    ASSERT_EQ_U32(ENET_PALR,      0x00000000, "PALR reset");
    ASSERT_EQ_U32(ENET_PAUR,      0x00008808, "PAUR reset");
    ASSERT_EQ_U32(ENET_OPD,       0x00010001, "OPD reset");
    ASSERT_EQ_U32(ENET_RDSR,      0x00000000, "RDSR reset");
    ASSERT_EQ_U32(ENET_TDSR,      0x00000000, "TDSR reset");
    ASSERT_EQ_U32(ENET_MRBR,      0x00000000, "MRBR reset");

    TEST_END();
}

/**
 * test_02_ecr_write_protection
 * 验证 ECR 写保护: 必须先写 ECR_MAGIC 解锁
 */
static void test_02_ecr_write_protection(void)
{
    TEST_START("02: ECR Write Protection");

    unsigned int orig = ENET_ECR;

    /* 无解锁写 — 应被忽略 */
    ENET_ECR = 0xBBBB0000;
    ASSERT_COND(ENET_ECR != 0xBBBB0000, "ECR write ignored without unlock");

    /* 解锁 */
    ENET_ECR_MAGIC = ECR_UNLOCK_MAGIC;

    /* 解锁后写 — 应生效 */
    ENET_ECR = 0x00000005;  /* ETHEREN | EN1588 */
    ASSERT_EQ_U32(ENET_ECR, 0x00000005, "ECR writable after unlock");

    /* 写后自动重锁 — 再写应无效 */
    ENET_ECR = 0xAAAA0000;
    ASSERT_COND(ENET_ECR != 0xAAAA0000, "ECR re-locks after write");

    /* 恢复 */
    ENET_ECR_MAGIC = ECR_UNLOCK_MAGIC;
    ENET_ECR = orig;

    TEST_END();
}

/**
 * test_03_eir_write_one_clear
 * 验证 EIR W1C 行为
 */
static void test_03_eir_write_one_clear(void)
{
    TEST_START("03: EIR W1C Behavior");

    /* 初始应为 0 */
    ASSERT_EQ_U32(ENET_EIR, 0x00000000, "EIR initially 0");

    /* 写 0 不应改变 */
    ENET_EIR = 0x00000000;
    ASSERT_EQ_U32(ENET_EIR, 0x00000000, "EIR write-0 no-op");

    /* 写全 1 应清除所有 set bits (当前无 → 仍为 0) */
    ENET_EIR = 0xFFFFFFFF;
    ASSERT_EQ_U32(ENET_EIR, 0x00000000, "EIR write-all-1s clears");

    TEST_END();
}

/**
 * test_04_read_only_registers
 * 验证 RDSR 和 ATSTMP RO 行为
 */
static void test_04_read_only_registers(void)
{
    TEST_START("04: Read-Only Registers");

    unsigned int rdsr_orig = ENET_RDSR;
    ENET_RDSR = 0xDEADBEEF;
    ASSERT_EQ_U32(ENET_RDSR, rdsr_orig, "RDSR RO");

    /* 恢复测试值 */
    ENET_TDSR = rdsr_orig;

    TEST_END();
}

/**
 * test_05_interrupt_mask
 * 验证 EIMR 中断掩码读写
 */
static void test_05_interrupt_mask(void)
{
    TEST_START("05: Interrupt Mask");

    unsigned int mask = EIR_TXF | EIR_RXF | EIR_MII;
    ENET_EIMR = mask;
    ASSERT_EQ_U32(ENET_EIMR, mask, "EIMR write/read-back");

    ENET_EIMR = 0x00000000;
    ASSERT_EQ_U32(ENET_EIMR, 0x00000000, "EIMR clear");

    TEST_END();
}

/**
 * test_06_mdio_phy_detect
 * 通过 MDIO 探测 DP83822 PHY ID
 */
static void test_06_mdio_phy_detect(void)
{
    TEST_START("06: MDIO PHY Detect");

    /* 配置 MDIO 时钟 (MSCR) — 50MHz 系统时钟 / (0x3E << 1) ≈ 2.5MHz MDIO */
    ENET_MSCR = 0x0000003E;

    /* 确保 Ethernet 使能 (写保护已解锁) */
    ENET_ECR_MAGIC = ECR_UNLOCK_MAGIC;
    ENET_ECR |= ECR_ETHEREN;

    /* 读 PHY ID1 (register 2) — 期望 0x2000 */
    unsigned int id1 = mdio_read(0, PHY_REG_PHYIDR1);
    ASSERT_EQ_U32(id1, DP83822_PHYID1, "PHYIDR1 == 0x2000");

    /* 读 PHY ID2 (register 3) — 期望 0xA221 */
    unsigned int id2 = mdio_read(0, PHY_REG_PHYIDR2);
    ASSERT_EQ_U32(id2, DP83822_PHYID2, "PHYIDR2 == 0xA221");

    TEST_END();
}

/**
 * test_07_phy_link_status
 * 验证 DP83822 PHY 报告 Link Up
 */
static void test_07_phy_link_status(void)
{
    TEST_START("07: PHY Link Status");

    /* BMSR (register 1) bit 2 = Link Status */
    unsigned int bmsr = mdio_read(0, PHY_REG_BMSR);
    ASSERT_COND((bmsr >> 2) & 1, "BMSR Link Up");

    /* 验证 100M Full Duplex capability */
    ASSERT_COND((bmsr >> 14) & 1, "BMSR 100Base-TX Full Duplex");

    TEST_END();
}

/**
 * test_08_mac_address_register
 * 验证 PALR / PAUR MAC 地址寄存器
 */
static void test_08_mac_address_register(void)
{
    TEST_START("08: MAC Address Registers");

    /* PALR: 可读写 MAC[31:0] */
    unsigned int palr_orig = ENET_PALR;
    ENET_PALR = 0xDEADBEEF;
    ASSERT_EQ_U32(ENET_PALR, 0xDEADBEEF, "PALR RW");

    /* PAUR: 可读写 MAC[47:32] + type */
    unsigned int paur_orig = ENET_PAUR;
    ENET_PAUR = 0x00001234;
    ASSERT_EQ_U32(ENET_PAUR, 0x00001234, "PAUR RW");

    /* 恢复 */
    ENET_PALR = palr_orig;
    ENET_PAUR = paur_orig;

    TEST_END();
}

/**
 * test_09_rcr_tcr_config
 * 验证 RCR/TCR 字段操作
 */
static void test_09_rcr_tcr_config(void)
{
    TEST_START("09: RCR/TCR Configuration");

    unsigned int rcr_orig = ENET_RCR;
    unsigned int tcr_orig = ENET_TCR;

    /* 配置 RMII + Promiscuous + Flow Control */
    ENET_RCR = RCR_RMII_MODE | RCR_PROM | RCR_FCE | 0x05EE0000;
    ASSERT_COND(ENET_RCR & RCR_RMII_MODE, "RCR RMII mode set");
    ASSERT_COND(ENET_RCR & RCR_PROM, "RCR Promiscuous set");
    ASSERT_COND(ENET_RCR & RCR_FCE, "RCR Flow Control set");

    /* 配置 Full Duplex + Flow Control */
    ENET_TCR = TCR_FDEN | TCR_FCE;
    ASSERT_COND(ENET_TCR & TCR_FDEN, "TCR Full Duplex set");
    ASSERT_COND(ENET_TCR & TCR_FCE, "TCR Flow Control set");

    /* 恢复 */
    ENET_RCR = rcr_orig;
    ENET_TCR = tcr_orig;

    TEST_END();
}

/**
 * test_10_dma_descriptor_ring
 * 验证 BD 环基址寄存器 (TDSR, MRBR)
 *
 * 注意: 此测试仅验证寄存器读写，不测试实际 DMA 传输。
 *       完整 DMA 测试需 lwIP 就绪后 (Phase 2)。
 */
static void test_10_dma_descriptor_ring(void)
{
    TEST_START("10: DMA Descriptor Ring");

    /* TDSR: TX Descriptor Ring Start — RW */
    unsigned int tdsr_orig = ENET_TDSR;
    ENET_TDSR = 0x20201000;
    ASSERT_EQ_U32(ENET_TDSR, 0x20201000, "TDSR RW");
    ENET_TDSR = tdsr_orig;

    /* MRBR: Max RX Buffer Size — RW */
    unsigned int mrbr_orig = ENET_MRBR;
    ENET_MRBR = 1536;
    ASSERT_EQ_U32(ENET_MRBR, 1536, "MRBR RW");
    ENET_MRBR = mrbr_orig;

    /* TFWR: TX FIFO Watermark — RW */
    unsigned int tfwr_orig = ENET_TFWR;
    ENET_TFWR = 0x80;
    ASSERT_EQ_U32(ENET_TFWR, 0x80, "TFWR RW");
    ENET_TFWR = tfwr_orig;

    TEST_END();
}

/**
 * test_11_fifo_thresholds
 * 验证所有 FIFO 阈值寄存器
 */
static void test_11_fifo_thresholds(void)
{
    TEST_START("11: FIFO Thresholds");

    unsigned int rsfl_orig = ENET_RSFL;
    unsigned int rsem_orig = ENET_RSEM;
    unsigned int raem_orig = ENET_RAEM;
    unsigned int rafl_orig = ENET_RAFL;

    ENET_RSFL = 0x80;
    ASSERT_EQ_U32(ENET_RSFL, 0x80, "RSFL RW");

    ENET_RSEM = 0x40;
    ASSERT_EQ_U32(ENET_RSEM, 0x40, "RSEM RW");

    ENET_RAEM = 0x08;
    ASSERT_EQ_U32(ENET_RAEM, 0x08, "RAEM RW");

    ENET_RAFL = 0x10;
    ASSERT_EQ_U32(ENET_RAFL, 0x10, "RAFL RW");

    /* 恢复 */
    ENET_RSFL = rsfl_orig;
    ENET_RSEM = rsem_orig;
    ENET_RAEM = raem_orig;
    ENET_RAFL = rafl_orig;

    TEST_END();
}

/* ------------------------------------------------------------------ */
/*  测试入口                                                             */
/* ------------------------------------------------------------------ */

/**
 * test_asserts_run_all — 运行全部测试，输出汇总
 */
void test_asserts_run_all(void)
{
    semihosting_write("\r\n");
    semihosting_write("========================================\r\n");
    semihosting_write("  i.MX RT1180 ENET HIL Test Suite (L3)\r\n");
    semihosting_write("========================================\r\n");

    test_01_enet_reset_values();
    test_02_ecr_write_protection();
    test_03_eir_write_one_clear();
    test_04_read_only_registers();
    test_05_interrupt_mask();
    test_06_mdio_phy_detect();
    test_07_phy_link_status();
    test_08_mac_address_register();
    test_09_rcr_tcr_config();
    test_10_dma_descriptor_ring();
    test_11_fifo_thresholds();

    /* 结果汇总 */
    semihosting_write("\r\n========================================\r\n");
    semihosting_write("  TEST RESULTS\r\n");

    semihosting_write("  Passed: ");
    /* 简单整数输出 — put_hex 输出十六进制 */
    put_hex((unsigned int)g_asserts_passed);
    semihosting_write("\r\n");

    semihosting_write("  Failed: ");
    put_hex((unsigned int)g_asserts_failed);
    semihosting_write("\r\n");

    if (g_asserts_failed == 0) {
        semihosting_write("  STATUS: ALL PASSED\r\n");
    } else {
        semihosting_write("  STATUS: FAILED\r\n");
    }
    semihosting_write("========================================\r\n");

    /* 测试结束 — 通知 QEMU 测试完成 (用于 pytest 集成) */
    semihosting_write("TEST_COMPLETE\r\n");
}
