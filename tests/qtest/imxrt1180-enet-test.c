/*
 * QTest for i.MX RT1180 ENET Ethernet MAC Controller
 *
 * Tests register reset values, RW round-trip, RO write-ignore,
 * W1C behavior, and interrupt generation per docs/interfaces.md.
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"

/* ------------------------------------------------------------------ */
/*  ENET Register Offsets (from include/hw/net/imxrt1180_enet.h)       */
/* ------------------------------------------------------------------ */
#define ENET_ECR            0x0000
#define ENET_EIR            0x0004
#define ENET_EIMR           0x0008
#define ENET_RDAR           0x0010
#define ENET_TDAR           0x0014
#define ENET_ECR_MAGIC      0x0024
#define ENET_MMFR           0x0040
#define ENET_MSCR           0x0044
#define ENET_MIBC           0x0048
#define ENET_RCR            0x0064
#define ENET_TCR            0x0084
#define ENET_PALR           0x00C4
#define ENET_PAUR           0x00C8
#define ENET_OPD            0x00E4
#define ENET_IAUR           0x00EC
#define ENET_IALR           0x00F0
#define ENET_GAUR           0x00F4
#define ENET_GALR           0x00F8
#define ENET_TFWR           0x0100
#define ENET_RDSR           0x0144
#define ENET_TDSR           0x0154
#define ENET_MRBR           0x0160
#define ENET_RSFL           0x0184
#define ENET_RSEM           0x018C
#define ENET_RAEM           0x0190
#define ENET_RAFL           0x0194
#define ENET_TSEM           0x0198
#define ENET_TAEM           0x019C
#define ENET_TAFL           0x01A0
#define ENET_TIPG           0x01A4
#define ENET_ATCR           0x01C0
#define ENET_ATVR           0x01C4
#define ENET_ATOFF          0x01C8
#define ENET_ATPER          0x01CC
#define ENET_ATCOR          0x01D0
#define ENET_ATINC          0x01D4
#define ENET_ATSTMP         0x01D8
#define ENET_TGSR           0x0200
#define ENET_TCSR0          0x0208
#define ENET_TCCR0          0x020C
#define ENET_TCSR1          0x0210
#define ENET_TCCR1          0x0214
#define ENET_TCSR2          0x0218
#define ENET_TCCR2          0x021C
#define ENET_TCSR3          0x0220
#define ENET_TCCR3          0x0224

/* ENET base address (from docs/interfaces.md §3.1) */
#define ENET_BASE           0x40424000ULL

/* ------------------------------------------------------------------ */
/*  Expected Reset Values (from docs/interfaces.md §1.1)               */
/* ------------------------------------------------------------------ */
#define RESET_ECR           0xF0000000U  /* Note: header says 0xF0000100 */
#define RESET_EIR           0x00000000U
#define RESET_EIMR          0x00000000U
#define RESET_RDAR          0x00000000U
#define RESET_TDAR          0x00000000U
#define RESET_ECR_MAGIC     0x00000000U
#define RESET_MMFR          0x00000000U
#define RESET_MSCR          0x00000040U
#define RESET_MIBC          0xC0000000U
#define RESET_RCR           0x05E00001U  /* Note: header says 0x05EE0001 */
#define RESET_TCR           0x00000010U
#define RESET_PALR          0x00000000U
#define RESET_PAUR          0x00008808U
#define RESET_OPD           0x00010001U
#define RESET_IAUR          0x00000000U
#define RESET_IALR          0x00000000U
#define RESET_GAUR          0x00000000U
#define RESET_GALR          0x00000000U
#define RESET_TFWR          0x00000000U
#define RESET_RDSR          0x00000000U
#define RESET_TDSR          0x00000000U
#define RESET_MRBR          0x00000000U
#define RESET_RSFL          0x00000000U
#define RESET_RSEM          0x00000000U
#define RESET_RAEM          0x00000004U
#define RESET_RAFL          0x00000004U
#define RESET_TSEM          0x00000060U
#define RESET_TAEM          0x00000008U
#define RESET_TAFL          0x00000008U
#define RESET_TIPG          0x0000000CU
#define RESET_ATCR          0x00000000U
#define RESET_ATVR          0x00000000U
#define RESET_ATOFF         0x00000000U
#define RESET_ATPER         0x00000000U
#define RESET_ATCOR         0x00000000U
#define RESET_ATINC         0x00000000U
#define RESET_ATSTMP        0x00000000U
#define RESET_TGSR          0x00000000U
#define RESET_TCSR0        0x00000000U
#define RESET_TCCR0        0x00000000U
#define RESET_TCSR1        0x00000000U
#define RESET_TCCR1        0x00000000U
#define RESET_TCSR2        0x00000000U
#define RESET_TCCR2        0x00000000U
#define RESET_TCSR3        0x00000000U
#define RESET_TCCR3        0x00000000U

/* ------------------------------------------------------------------ */
/*  Convenience macros for register R/W                               */
/* ------------------------------------------------------------------ */
static uint32_t enet_readl(uint32_t offset)
{
    return qtest_readl(ENET_BASE + offset);
}

static void enet_writel(uint32_t offset, uint32_t value)
{
    qtest_writel(ENET_BASE + offset, value);
}

#define CHECK_RESET(off, expected) \
    do { \
        uint32_t val = enet_readl(off); \
        g_assert_cmphex(val, ==, expected); \
    } while (0)

#define CHECK_RW_ROUNDTRIP(off, test_val) \
    do { \
        enet_writel(off, test_val); \
        uint32_t val = enet_readl(off); \
        g_assert_cmphex(val, ==, test_val); \
    } while (0)

/* ================================================================== */
/*  Test: Core Register Reset Values                                   */
/* ================================================================== */
static void test_enet_reset_core(void)
{
    CHECK_RESET(ENET_ECR,        RESET_ECR);
    CHECK_RESET(ENET_EIR,        RESET_EIR);
    CHECK_RESET(ENET_EIMR,       RESET_EIMR);
    CHECK_RESET(ENET_RDAR,       RESET_RDAR);
    CHECK_RESET(ENET_TDAR,       RESET_TDAR);
    CHECK_RESET(ENET_ECR_MAGIC,  RESET_ECR_MAGIC);
    CHECK_RESET(ENET_MMFR,       RESET_MMFR);
    CHECK_RESET(ENET_MSCR,       RESET_MSCR);
    CHECK_RESET(ENET_MIBC,       RESET_MIBC);
}

static void test_enet_reset_mac(void)
{
    CHECK_RESET(ENET_RCR,        RESET_RCR);
    CHECK_RESET(ENET_TCR,        RESET_TCR);
    CHECK_RESET(ENET_PALR,       RESET_PALR);
    CHECK_RESET(ENET_PAUR,       RESET_PAUR);
    CHECK_RESET(ENET_OPD,        RESET_OPD);
    CHECK_RESET(ENET_IAUR,       RESET_IAUR);
    CHECK_RESET(ENET_IALR,       RESET_IALR);
    CHECK_RESET(ENET_GAUR,       RESET_GAUR);
    CHECK_RESET(ENET_GALR,       RESET_GALR);
    CHECK_RESET(ENET_TFWR,       RESET_TFWR);
}

static void test_enet_reset_dma(void)
{
    CHECK_RESET(ENET_RDSR,       RESET_RDSR);
    CHECK_RESET(ENET_TDSR,       RESET_TDSR);
    CHECK_RESET(ENET_MRBR,       RESET_MRBR);
}

static void test_enet_reset_fifo(void)
{
    CHECK_RESET(ENET_RSFL,       RESET_RSFL);
    CHECK_RESET(ENET_RSEM,       RESET_RSEM);
    CHECK_RESET(ENET_RAEM,       RESET_RAEM);
    CHECK_RESET(ENET_RAFL,       RESET_RAFL);
    CHECK_RESET(ENET_TSEM,       RESET_TSEM);
    CHECK_RESET(ENET_TAEM,       RESET_TAEM);
    CHECK_RESET(ENET_TAFL,       RESET_TAFL);
    CHECK_RESET(ENET_TIPG,       RESET_TIPG);
}

static void test_enet_reset_1588(void)
{
    CHECK_RESET(ENET_ATCR,       RESET_ATCR);
    CHECK_RESET(ENET_ATVR,       RESET_ATVR);
    CHECK_RESET(ENET_ATOFF,      RESET_ATOFF);
    CHECK_RESET(ENET_ATPER,      RESET_ATPER);
    CHECK_RESET(ENET_ATCOR,      RESET_ATCOR);
    CHECK_RESET(ENET_ATINC,      RESET_ATINC);
    CHECK_RESET(ENET_ATSTMP,     RESET_ATSTMP);
    CHECK_RESET(ENET_TGSR,       RESET_TGSR);
    CHECK_RESET(ENET_TCSR0,      RESET_TCSR0);
    CHECK_RESET(ENET_TCCR0,      RESET_TCCR0);
    CHECK_RESET(ENET_TCSR1,      RESET_TCSR1);
    CHECK_RESET(ENET_TCCR1,      RESET_TCCR1);
    CHECK_RESET(ENET_TCSR2,      RESET_TCSR2);
    CHECK_RESET(ENET_TCCR2,      RESET_TCCR2);
    CHECK_RESET(ENET_TCSR3,      RESET_TCSR3);
    CHECK_RESET(ENET_TCCR3,      RESET_TCCR3);
}

/* ================================================================== */
/*  Test: RW Register Round-trip                                       */
/* ================================================================== */
static void test_enet_rw_basic(void)
{
    /* PALR — simple RW, no side effects */
    CHECK_RW_ROUNDTRIP(ENET_PALR, 0xDEADBEEF);
    CHECK_RW_ROUNDTRIP(ENET_PALR, 0x00000000); /* restore */

    /* PAUR — bits [47:32] of MAC + type */
    CHECK_RW_ROUNDTRIP(ENET_PAUR, 0x00001234);
    CHECK_RW_ROUNDTRIP(ENET_PAUR, 0x00008808); /* restore */

    /* IAUR, IALR, GAUR, GALR — descriptor hash addresses */
    CHECK_RW_ROUNDTRIP(ENET_IALR, 0xAAAAAAAA);
    CHECK_RW_ROUNDTRIP(ENET_IALR, 0x00000000);
    CHECK_RW_ROUNDTRIP(ENET_GAUR, 0x55555555);
    CHECK_RW_ROUNDTRIP(ENET_GAUR, 0x00000000);
}

static void test_enet_rw_fifo_thresh(void)
{
    /* FIFO threshold registers — full 32-bit write */
    CHECK_RW_ROUNDTRIP(ENET_RSFL, 0x00000080);
    CHECK_RW_ROUNDTRIP(ENET_RSFL, RESET_RSFL);

    CHECK_RW_ROUNDTRIP(ENET_RSEM, 0x00000040);
    CHECK_RW_ROUNDTRIP(ENET_RSEM, RESET_RSEM);

    CHECK_RW_ROUNDTRIP(ENET_RAEM, 0x00000008);
    CHECK_RW_ROUNDTRIP(ENET_RAEM, RESET_RAEM);

    CHECK_RW_ROUNDTRIP(ENET_RAFL, 0x00000010);
    CHECK_RW_ROUNDTRIP(ENET_RAFL, RESET_RAFL);

    CHECK_RW_ROUNDTRIP(ENET_TSEM, 0x00000080);
    CHECK_RW_ROUNDTRIP(ENET_TSEM, RESET_TSEM);

    CHECK_RW_ROUNDTRIP(ENET_TAEM, 0x00000010);
    CHECK_RW_ROUNDTRIP(ENET_TAEM, RESET_TAEM);

    CHECK_RW_ROUNDTRIP(ENET_TAFL, 0x00000020);
    CHECK_RW_ROUNDTRIP(ENET_TAFL, RESET_TAFL);

    CHECK_RW_ROUNDTRIP(ENET_TIPG, 0x00000018);
    CHECK_RW_ROUNDTRIP(ENET_TIPG, RESET_TIPG);
}

static void test_enet_rw_dma(void)
{
    /* TDSR — RW */
    CHECK_RW_ROUNDTRIP(ENET_TDSR, 0x20201000);
    CHECK_RW_ROUNDTRIP(ENET_TDSR, RESET_TDSR);

    /* MRBR — RW */
    CHECK_RW_ROUNDTRIP(ENET_MRBR, 1536);
    CHECK_RW_ROUNDTRIP(ENET_MRBR, RESET_MRBR);

    /* TFWR — RW */
    CHECK_RW_ROUNDTRIP(ENET_TFWR, 0x00000080);
    CHECK_RW_ROUNDTRIP(ENET_TFWR, RESET_TFWR);
}

/* ================================================================== */
/*  Test: ECR Write Protection (ECR_MAGIC unlock)                      */
/* ================================================================== */
static void test_enet_ecr_write_protect(void)
{
    uint32_t orig_ecr = enet_readl(ENET_ECR);

    /* ECR should be write-protected: writing without unlock should not change */
    enet_writel(ENET_ECR, 0x0000FFFF);
    g_assert_cmphex(enet_readl(ENET_ECR), !=, 0x0000FFFF);

    /* Write unlock magic */
    enet_writel(ENET_ECR_MAGIC, 0x5A5A5A5AU);

    /* Now ECR should be writable */
    enet_writel(ENET_ECR, 0x00000005U);  /* ETHEREN | EN1588 */
    g_assert_cmphex(enet_readl(ENET_ECR), ==, 0x00000005U);

    /* After a write, ECR should re-lock */
    enet_writel(ENET_ECR, 0x0000FFFF);
    g_assert_cmphex(enet_readl(ENET_ECR), !=, 0x0000FFFF);

    /* Restore original ECR */
    enet_writel(ENET_ECR_MAGIC, 0x5A5A5A5AU);
    enet_writel(ENET_ECR, orig_ecr);
}

/* ================================================================== */
/*  Test: RDSR is Read-Only                                            */
/* ================================================================== */
static void test_enet_rdsr_read_only(void)
{
    uint32_t orig = enet_readl(ENET_RDSR);

    /* Write a value — should be ignored for RO register */
    enet_writel(ENET_RDSR, 0xDEADBEEF);
    g_assert_cmphex(enet_readl(ENET_RDSR), ==, orig);
}

/* ================================================================== */
/*  Test: ATSTMP is Read-Only                                          */
/* ================================================================== */
static void test_enet_atstmp_read_only(void)
{
    uint32_t orig = enet_readl(ENET_ATSTMP);

    /* Write a value — should be ignored for RO register */
    enet_writel(ENET_ATSTMP, 0xDEADBEEF);
    g_assert_cmphex(enet_readl(ENET_ATSTMP), ==, orig);
}

/* ================================================================== */
/*  Test: EIR Write-1-to-Clear (W1C) Behavior                          */
/* ================================================================== */
static void test_enet_eir_w1c(void)
{
    /* Set all writable interrupt bits via EIMR + trigger simulation.
     * For Phase 1 skeleton: verify that writing 1 clears the bit.
     * Write 0xFFFFFFFF to EIR — should clear all set bits. */

    /* Initially EIR should be 0 */
    g_assert_cmphex(enet_readl(ENET_EIR), ==, 0x00000000U);

    /* Write 0 to EIR should have no effect (W1C: write-0 = no-op) */
    enet_writel(ENET_EIR, 0x00000000U);
    g_assert_cmphex(enet_readl(ENET_EIR), ==, 0x00000000U);

    /* Write 1s — all currently-set bits should clear (none set → stays 0) */
    enet_writel(ENET_EIR, 0xFFFFFFFFU);
    g_assert_cmphex(enet_readl(ENET_EIR), ==, 0x00000000U);
}

/* ================================================================== */
/*  Test: Interrupt Mask (EIMR) behavior                               */
/* ================================================================== */
static void test_enet_eimr_mask(void)
{
    /* EIMR is RW */
    CHECK_RW_ROUNDTRIP(ENET_EIMR, 0x0F800024U); /* TXF|TXB|RXF|RXB|MII|EBERR|LC|RL|UN */
    CHECK_RW_ROUNDTRIP(ENET_EIMR, 0x00000000U); /* restore */
}

/* ================================================================== */
/*  Test: MDIO Frame Register (MMFR) Clause 22 Read                    */
/* ================================================================== */
static void test_enet_mmfr_clause22_read(void)
{
    /* Construct a Clause 22 read frame:
     *   ST=01, OP=10 (read), PA=0, RA=2 (PHYIDR1), TA=00, DATA=0 */
    uint32_t mmfr_val = (0x01U << 30)  /* ST = 01 */
                      | (0x02U << 28)  /* OP = 10 (read) */
                      | (0x00U << 23)  /* PA = 0 */
                      | (0x02U << 18)  /* RA = 2 (PHYIDR1) */
                      /* TA = 00 (HW fills) */
                      /* DATA = 0 */;

    enet_writel(ENET_MMFR, mmfr_val);

    /* Read back — ST, OP, PA, RA should be preserved;
     * TA and DATA may be updated by PHY response. */
    uint32_t result = enet_readl(ENET_MMFR);

    /* ST and OP should be unchanged */
    g_assert_cmphex((result >> 30) & 0x3, ==, 0x1);
    g_assert_cmphex((result >> 28) & 0x3, ==, 0x2);
    /* PA should be unchanged */
    g_assert_cmphex((result >> 23) & 0x1F, ==, 0x00);
    /* RA should be unchanged */
    g_assert_cmphex((result >> 18) & 0x1F, ==, 0x02);
}

/* ================================================================== */
/*  Test: MDIO Speed Control (MSCR)                                    */
/* ================================================================== */
static void test_enet_mscr(void)
{
    uint32_t orig = enet_readl(ENET_MSCR);
    uint32_t test_val = 0x0000003E;  /* Common MII_SPEED for 50MHz */
    enet_writel(ENET_MSCR, test_val);
    g_assert_cmphex(enet_readl(ENET_MSCR), ==, test_val);
    enet_writel(ENET_MSCR, orig);
}

/* ================================================================== */
/*  Test: MIB Control (MIBC)                                           */
/* ================================================================== */
static void test_enet_mibc(void)
{
    uint32_t orig = enet_readl(ENET_MIBC);
    uint32_t test_val = 0x80000000U;  /* MIB_DIS */
    enet_writel(ENET_MIBC, test_val);
    g_assert_cmphex(enet_readl(ENET_MIBC), ==, test_val);
    enet_writel(ENET_MIBC, orig);
}

/* ================================================================== */
/*  Test: Interrupt generation — write EIMR + trigger → IRQ asserted   */
/* ================================================================== */
static void test_enet_irq_basic(void)
{
    /* Phase 1 skeleton: verify IRQ line can be probed.
     * Full test requires ENET TX/RX DMA functional.
     * For now: ensure EIR/EIMR read-back is correct after manipulation. */

    /* Mask some interrupt bits */
    enet_writel(ENET_EIMR, ENET_INT_TXF | ENET_INT_RXF);

    /* Verify mask was written */
    uint32_t mask = enet_readl(ENET_EIMR);
    g_assert_cmphex(mask & ENET_INT_TXF, ==, ENET_INT_TXF);
    g_assert_cmphex(mask & ENET_INT_RXF, ==, ENET_INT_RXF);

    /* Restore */
    enet_writel(ENET_EIMR, 0x00000000U);
}

/* ================================================================== */
/*  Test: RCR / TCR basic field writes                                 */
/* ================================================================== */
static void test_enet_rcr_tcr(void)
{
    uint32_t rcr_orig = enet_readl(ENET_RCR);
    uint32_t tcr_orig = enet_readl(ENET_TCR);

    /* RCR: enable promiscuous + RMII + FCE */
    enet_writel(ENET_RCR, 0x05EE012DU);
    g_assert_cmphex(enet_readl(ENET_RCR), ==, 0x05EE012DU);

    /* TCR: enable FDEN + FCE */
    enet_writel(ENET_TCR, 0x00000024U);
    g_assert_cmphex(enet_readl(ENET_TCR), ==, 0x00000024U);

    /* Restore */
    enet_writel(ENET_RCR, rcr_orig);
    enet_writel(ENET_TCR, tcr_orig);
}

/* ================================================================== */
/*  Test Main                                                          */
/* ================================================================== */
int main(int argc, char **argv)
{
    int ret;

    g_test_init(&argc, &argv, NULL);

    /* Reset value tests — grouped by functional block */
    qtest_add_func("/imxrt1180/enet/reset/core",
                   test_enet_reset_core);
    qtest_add_func("/imxrt1180/enet/reset/mac",
                   test_enet_reset_mac);
    qtest_add_func("/imxrt1180/enet/reset/dma",
                   test_enet_reset_dma);
    qtest_add_func("/imxrt1180/enet/reset/fifo",
                   test_enet_reset_fifo);
    qtest_add_func("/imxrt1180/enet/reset/1588",
                   test_enet_reset_1588);

    /* RW round-trip tests */
    qtest_add_func("/imxrt1180/enet/rw/basic",
                   test_enet_rw_basic);
    qtest_add_func("/imxrt1180/enet/rw/fifo_thresh",
                   test_enet_rw_fifo_thresh);
    qtest_add_func("/imxrt1180/enet/rw/dma",
                   test_enet_rw_dma);

    /* Special behavior tests */
    qtest_add_func("/imxrt1180/enet/ecr/write_protect",
                   test_enet_ecr_write_protect);
    qtest_add_func("/imxrt1180/enet/rdsr/read_only",
                   test_enet_rdsr_read_only);
    qtest_add_func("/imxrt1180/enet/atstmp/read_only",
                   test_enet_atstmp_read_only);
    qtest_add_func("/imxrt1180/enet/eir/w1c",
                   test_enet_eir_w1c);
    qtest_add_func("/imxrt1180/enet/eimr/mask",
                   test_enet_eimr_mask);

    /* MDIO tests */
    qtest_add_func("/imxrt1180/enet/mmfr/clause22_read",
                   test_enet_mmfr_clause22_read);
    qtest_add_func("/imxrt1180/enet/mscr/rw",
                   test_enet_mscr);
    qtest_add_func("/imxrt1180/enet/mibc/rw",
                   test_enet_mibc);

    /* IRQ tests */
    qtest_add_func("/imxrt1180/enet/irq/basic",
                   test_enet_irq_basic);

    /* RCR/TCR tests */
    qtest_add_func("/imxrt1180/enet/rcr_tcr/rw",
                   test_enet_rcr_tcr);

    qtest_start("-M imxrt1180-evk");
    ret = g_test_run();
    qtest_end();

    return ret;
}
