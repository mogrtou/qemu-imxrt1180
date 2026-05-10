#!/usr/bin/env python3
"""
独立 ENET 寄存器测试 — 使用 qtest 协议验证 i.MX RT1180 ENET 外设。

完全独立于 QEMU 源码树:
  - 不需要编译进 QEMU 的 meson 构建系统
  - 仅通过 stdin/stdout 与 qemu-system-arm 的 -qtest stdio 协议通信
  - 无需 libqtest、glib 等 QEMU 内部库

用法:
    # 设置 QEMU 二进制路径 (或放在 PATH 中)
    export QEMU_BINARY=./qemu/build/qemu-system-arm

    # 运行全部测试
    python -m pytest test_enet_standalone.py -v

    # 运行特定测试
    python -m pytest test_enet_standalone.py -v -k "reset"

依赖:
    pip install pytest
"""

import os
import pytest

from qtest_client import QTestClient, QTestError


# ==================================================================
#  ENET 寄存器偏移 (from docs/interfaces.md §1.1)
# ==================================================================
ENET_BASE = 0x40424000

REG = {
    "ECR":        0x0000,
    "EIR":        0x0004,
    "EIMR":       0x0008,
    "RDAR":       0x0010,
    "TDAR":       0x0014,
    "ECR_MAGIC":  0x0024,
    "MMFR":       0x0040,
    "MSCR":       0x0044,
    "MIBC":       0x0048,
    "RCR":        0x0064,
    "TCR":        0x0084,
    "PALR":       0x00C4,
    "PAUR":       0x00C8,
    "OPD":        0x00E4,
    "IAUR":       0x00EC,
    "IALR":       0x00F0,
    "GAUR":       0x00F4,
    "GALR":       0x00F8,
    "TFWR":       0x0100,
    "RDSR":       0x0144,
    "TDSR":       0x0154,
    "MRBR":       0x0160,
    "RSFL":       0x0184,
    "RSEM":       0x018C,
    "RAEM":       0x0190,
    "RAFL":       0x0194,
    "TSEM":       0x0198,
    "TAEM":       0x019C,
    "TAFL":       0x01A0,
    "TIPG":       0x01A4,
    "ATCR":       0x01C0,
    "ATVR":       0x01C4,
    "ATOFF":      0x01C8,
    "ATPER":      0x01CC,
    "ATCOR":      0x01D0,
    "ATINC":      0x01D4,
    "ATSTMP":     0x01D8,
    "TGSR":       0x0200,
    "TCSR0":      0x0208,
    "TCCR0":      0x020C,
    "TCSR1":      0x0210,
    "TCCR1":      0x0214,
    "TCSR2":      0x0218,
    "TCCR2":      0x021C,
    "TCSR3":      0x0220,
    "TCCR3":      0x0224,
}

# 期望复位值 (from docs/interfaces.md §1.1)
RESET_VALS = {
    "ECR":        0xF0000100,
    "EIR":        0x00000000,
    "EIMR":       0x00000000,
    "RDAR":       0x00000000,
    "TDAR":       0x00000000,
    "ECR_MAGIC":  0x00000000,
    "MMFR":       0x00000000,
    "MSCR":       0x00000040,
    "MIBC":       0xC0000000,
    "RCR":        0x05EE0001,
    "TCR":        0x00000010,
    "PALR":       0x00000000,
    "PAUR":       0x00008808,
    "OPD":        0x00010001,
    "IAUR":       0x00000000,
    "IALR":       0x00000000,
    "GAUR":       0x00000000,
    "GALR":       0x00000000,
    "TFWR":       0x00000000,
    "RDSR":       0x00000000,
    "TDSR":       0x00000000,
    "MRBR":       0x00000000,
    "RSFL":       0x00000000,
    "RSEM":       0x00000000,
    "RAEM":       0x00000004,
    "RAFL":       0x00000004,
    "TSEM":       0x00000060,
    "TAEM":       0x00000008,
    "TAFL":       0x00000008,
    "TIPG":       0x0000000C,
    "ATCR":       0x00000000,
    "ATVR":       0x00000000,
    "ATOFF":      0x00000000,
    "ATPER":      0x00000000,
    "ATCOR":      0x00000000,
    "ATINC":      0x00000000,
    "ATSTMP":     0x00000000,
    "TGSR":       0x00000000,
    "TCSR0":      0x00000000,
    "TCCR0":      0x00000000,
    "TCSR1":      0x00000000,
    "TCCR1":      0x00000000,
    "TCSR2":      0x00000000,
    "TCCR2":      0x00000000,
    "TCSR3":      0x00000000,
    "TCCR3":      0x00000000,
}

# EIR/EIMR 中断位
EIR_TXF    = 1 << 23
EIR_TXB    = 1 << 21
EIR_RXF    = 1 << 25
EIR_RXB    = 1 << 24
EIR_MII    = 1 << 27
EIR_EBERR  = 1 << 22
EIR_LC     = 1 << 2
EIR_RL     = 1 << 4
EIR_UN     = 1 << 5


# ==================================================================
#  Fixtures
# ==================================================================

def _get_qemu_binary():
    """获取 QEMU 二进制路径"""
    return os.environ.get("QEMU_BINARY", "qemu-system-arm")


@pytest.fixture(scope="module")
def qtest():
    """模块级 fixture: 启动 QEMU qtest 进程，所有测试共享一个实例"""
    qemu = _get_qemu_binary()
    client = QTestClient(qemu, "imxrt1180-evk")
    client.start()
    yield client
    client.stop()


def _reg_addr(name: str) -> int:
    """返回寄存器的绝对地址"""
    return ENET_BASE + REG[name]


def enet_read(qtest, name: str) -> int:
    """读 ENET 寄存器"""
    return qtest.readl(_reg_addr(name))


def enet_write(qtest, name: str, value: int):
    """写 ENET 寄存器"""
    qtest.writel(_reg_addr(name), value)


# ==================================================================
#  Test: 冒烟测试 — 确认 QEMU 启动
# ==================================================================

class TestSmoke:
    def test_qtest_running(self, qtest):
        """QEMU 进程应成功启动"""
        assert qtest.is_running(), "QEMU qtest process should be running"

    def test_memory_accessible(self, qtest):
        """ENET 寄存器空间应可读"""
        val = enet_read(qtest, "ECR")
        assert isinstance(val, int), f"Expected int, got {type(val)}"


# ==================================================================
#  Test: 复位值验证 — 全部 39 个寄存器
# ==================================================================

class TestResetValues:
    """验证上电复位后所有 ENET 寄存器的初始值"""

    CORE_REGS = ["ECR", "EIR", "EIMR", "RDAR", "TDAR", "ECR_MAGIC",
                  "MMFR", "MSCR", "MIBC"]
    MAC_REGS = ["RCR", "TCR", "PALR", "PAUR", "OPD", "IAUR", "IALR",
                "GAUR", "GALR", "TFWR"]
    DMA_REGS = ["RDSR", "TDSR", "MRBR"]
    FIFO_REGS = ["RSFL", "RSEM", "RAEM", "RAFL", "TSEM", "TAEM",
                 "TAFL", "TIPG"]
    IEEE1588_REGS = ["ATCR", "ATVR", "ATOFF", "ATPER", "ATCOR", "ATINC",
                     "ATSTMP", "TGSR", "TCSR0", "TCCR0", "TCSR1", "TCCR1",
                     "TCSR2", "TCCR2", "TCSR3", "TCCR3"]

    @pytest.mark.parametrize("reg_name", CORE_REGS)
    def test_reset_core(self, qtest, reg_name):
        val = enet_read(qtest, reg_name)
        expected = RESET_VALS[reg_name]
        assert val == expected, \
            f"{reg_name}@0x{REG[reg_name]:04X}: expected 0x{expected:08X}, got 0x{val:08X}"

    @pytest.mark.parametrize("reg_name", MAC_REGS)
    def test_reset_mac(self, qtest, reg_name):
        val = enet_read(qtest, reg_name)
        expected = RESET_VALS[reg_name]
        assert val == expected, \
            f"{reg_name}@0x{REG[reg_name]:04X}: expected 0x{expected:08X}, got 0x{val:08X}"

    @pytest.mark.parametrize("reg_name", DMA_REGS)
    def test_reset_dma(self, qtest, reg_name):
        val = enet_read(qtest, reg_name)
        expected = RESET_VALS[reg_name]
        assert val == expected, \
            f"{reg_name}@0x{REG[reg_name]:04X}: expected 0x{expected:08X}, got 0x{val:08X}"

    @pytest.mark.parametrize("reg_name", FIFO_REGS)
    def test_reset_fifo(self, qtest, reg_name):
        val = enet_read(qtest, reg_name)
        expected = RESET_VALS[reg_name]
        assert val == expected, \
            f"{reg_name}@0x{REG[reg_name]:04X}: expected 0x{expected:08X}, got 0x{val:08X}"

    @pytest.mark.parametrize("reg_name", IEEE1588_REGS)
    def test_reset_1588(self, qtest, reg_name):
        val = enet_read(qtest, reg_name)
        expected = RESET_VALS[reg_name]
        assert val == expected, \
            f"{reg_name}@0x{REG[reg_name]:04X}: expected 0x{expected:08X}, got 0x{val:08X}"


# ==================================================================
#  Test: RW Round-trip
# ==================================================================

class TestReadWrite:
    """验证可读写寄存器的 round-trip"""

    def test_palr_roundtrip(self, qtest):
        orig = enet_read(qtest, "PALR")
        enet_write(qtest, "PALR", 0xDEADBEEF)
        assert enet_read(qtest, "PALR") == 0xDEADBEEF
        enet_write(qtest, "PALR", orig)

    def test_paur_roundtrip(self, qtest):
        orig = enet_read(qtest, "PAUR")
        enet_write(qtest, "PAUR", 0x00001234)
        assert enet_read(qtest, "PAUR") == 0x00001234
        enet_write(qtest, "PAUR", orig)

    def test_ialr_roundtrip(self, qtest):
        enet_write(qtest, "IALR", 0xAAAAAAAA)
        assert enet_read(qtest, "IALR") == 0xAAAAAAAA
        enet_write(qtest, "IALR", 0x00000000)

    def test_gaur_roundtrip(self, qtest):
        enet_write(qtest, "GAUR", 0x55555555)
        assert enet_read(qtest, "GAUR") == 0x55555555
        enet_write(qtest, "GAUR", 0x00000000)

    def test_tdsr_roundtrip(self, qtest):
        enet_write(qtest, "TDSR", 0x20201000)
        assert enet_read(qtest, "TDSR") == 0x20201000
        enet_write(qtest, "TDSR", 0x00000000)

    def test_mrbr_roundtrip(self, qtest):
        enet_write(qtest, "MRBR", 1536)
        assert enet_read(qtest, "MRBR") == 1536
        enet_write(qtest, "MRBR", 0)

    def test_tfwr_roundtrip(self, qtest):
        enet_write(qtest, "TFWR", 0x80)
        assert enet_read(qtest, "TFWR") == 0x80
        enet_write(qtest, "TFWR", 0)

    @pytest.mark.parametrize("reg_name,test_val,restore", [
        ("RSFL", 0x80, 0x00),
        ("RSEM", 0x40, 0x00),
        ("RAEM", 0x08, 0x04),
        ("RAFL", 0x10, 0x04),
        ("TSEM", 0x80, 0x60),
        ("TAEM", 0x10, 0x08),
        ("TAFL", 0x20, 0x08),
        ("TIPG", 0x18, 0x0C),
    ])
    def test_fifo_thresh_roundtrip(self, qtest, reg_name, test_val, restore):
        enet_write(qtest, reg_name, test_val)
        assert enet_read(qtest, reg_name) == test_val, \
            f"{reg_name}: write 0x{test_val:X} → readback mismatch"
        enet_write(qtest, reg_name, restore)


# ==================================================================
#  Test: 特殊行为
# ==================================================================

class TestSpecialBehavior:
    """验证 RO 寄存器、W1C 行为、ECR 写保护"""

    def test_rdsr_read_only(self, qtest):
        """RDSR 是 RO 寄存器，写入应被忽略"""
        orig = enet_read(qtest, "RDSR")
        enet_write(qtest, "RDSR", 0xDEADBEEF)
        assert enet_read(qtest, "RDSR") == orig, \
            "RDSR should ignore writes (read-only)"

    def test_atstmp_read_only(self, qtest):
        """ATSTMP 是 RO 寄存器"""
        orig = enet_read(qtest, "ATSTMP")
        enet_write(qtest, "ATSTMP", 0xDEADBEEF)
        assert enet_read(qtest, "ATSTMP") == orig, \
            "ATSTMP should ignore writes (read-only)"

    def test_ecr_write_protect(self, qtest):
        """ECR 需要先写 ECR_MAGIC 解锁才能写入"""
        orig = enet_read(qtest, "ECR")

        # 无解锁写 → 应被忽略 (重置值)
        enet_write(qtest, "ECR", 0xFFFF0000)
        assert enet_read(qtest, "ECR") != 0xFFFF0000, \
            "ECR should be write-protected without unlock"

        # 解锁
        enet_write(qtest, "ECR_MAGIC", 0x5A5A5A5A)

        # 解锁后写 → 应生效
        enet_write(qtest, "ECR", 0x00000005)  # ETHEREN | EN1588
        assert enet_read(qtest, "ECR") == 0x00000005, \
            "ECR should be writable after unlock"

        # 写后应自动重新锁定
        enet_write(qtest, "ECR", 0x0000FFFF)
        assert enet_read(qtest, "ECR") != 0x0000FFFF, \
            "ECR should re-lock after write"

        # 恢复
        enet_write(qtest, "ECR_MAGIC", 0x5A5A5A5A)
        enet_write(qtest, "ECR", orig)

    def test_eir_w1c(self, qtest):
        """EIR 是 W1C — 写0无效，写1清除对应位"""
        # 初始值应为 0
        assert enet_read(qtest, "EIR") == 0

        # 写 0 不改变
        enet_write(qtest, "EIR", 0x00000000)
        assert enet_read(qtest, "EIR") == 0

        # 写全 1 清除所有 (当前无 set bits → 仍为 0)
        enet_write(qtest, "EIR", 0xFFFFFFFF)
        assert enet_read(qtest, "EIR") == 0

    def test_eimr_mask(self, qtest):
        """EIMR 读写"""
        mask = EIR_TXF | EIR_RXF | EIR_MII
        enet_write(qtest, "EIMR", mask)
        assert enet_read(qtest, "EIMR") == mask, \
            f"EIMR should read back 0x{mask:X}"
        enet_write(qtest, "EIMR", 0x00000000)


# ==================================================================
#  Test: MDIO / MMFR
# ==================================================================

class TestMDIO:
    """验证 MDIO 帧寄存器 (MMFR) 和 MDIO 速度控制 (MSCR)"""

    def test_mmfr_clause22_read_frame(self, qtest):
        """构造 Clause 22 读帧，验证 ST/OP/PA/RA 字段保持"""
        mmfr = (0x01 << 30) | (0x02 << 28) | (0x00 << 23) | (0x02 << 18)
        enet_write(qtest, "MMFR", mmfr)
        result = enet_read(qtest, "MMFR")

        # ST 位 = 01
        assert (result >> 30) & 0x3 == 0x1, "MMFR ST field mismatch"
        # OP 位 = 10 (read)
        assert (result >> 28) & 0x3 == 0x2, "MMFR OP field mismatch"
        # PA = 0
        assert (result >> 23) & 0x1F == 0x00, "MMFR PA field mismatch"
        # RA = 2 (PHYIDR1)
        assert (result >> 18) & 0x1F == 0x02, "MMFR RA field mismatch"

    def test_mscr_rw(self, qtest):
        orig = enet_read(qtest, "MSCR")
        enet_write(qtest, "MSCR", 0x3E)
        assert enet_read(qtest, "MSCR") == 0x3E
        enet_write(qtest, "MSCR", orig)

    def test_mibc_rw(self, qtest):
        orig = enet_read(qtest, "MIBC")
        enet_write(qtest, "MIBC", 0x80000000)
        assert enet_read(qtest, "MIBC") == 0x80000000
        enet_write(qtest, "MIBC", orig)


# ==================================================================
#  Test: RCR / TCR
# ==================================================================

class TestMACControl:
    """验证 RCR 和 TCR 控制寄存器"""

    def test_rcr_rmii_promiscuous(self, qtest):
        orig = enet_read(qtest, "RCR")
        # RMII mode + Promiscuous + FCE
        enet_write(qtest, "RCR", 0x05EE012D)
        assert enet_read(qtest, "RCR") == 0x05EE012D
        enet_write(qtest, "RCR", orig)

    def test_tcr_full_duplex_fce(self, qtest):
        orig = enet_read(qtest, "TCR")
        # FDEN + FCE
        enet_write(qtest, "TCR", 0x00000024)
        assert enet_read(qtest, "TCR") == 0x00000024
        enet_write(qtest, "TCR", orig)


# ==================================================================
#  Test: SoC Memory Map
# ==================================================================

class TestSoCMemoryMap:
    """验证 SoC 级内存映射 (ITCM, DTCM, OCRAM 可访问)"""

    def test_itcm_accessible(self, qtest):
        """ITCM 0x00000000 可读写"""
        qtest.writel(0x00000000, 0xDEADBEEF)
        val = qtest.readl(0x00000000)
        assert val == 0xDEADBEEF

    def test_dtcm_accessible(self, qtest):
        """DTCM 0x20000000 可读写"""
        qtest.writel(0x20000000, 0xCAFEBABE)
        val = qtest.readl(0x20000000)
        assert val == 0xCAFEBABE

    def test_ocram_accessible(self, qtest):
        """OCRAM 0x20200000 可读写"""
        qtest.writel(0x20200000, 0x12345678)
        val = qtest.readl(0x20200000)
        assert val == 0x12345678

    def test_enet_region_not_zero(self, qtest):
        """ENET 寄存器区域不应全为零 (至少 ECR 有非零复位值)"""
        val = qtest.readl(ENET_BASE + REG["ECR"])
        assert val != 0, "ECR reset value should be non-zero"
