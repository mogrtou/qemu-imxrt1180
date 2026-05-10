# i.MX RT1180 以太网子系统接口契约

> **文档状态**: Phase 1 接口契约
> **作者**: Architect Agent
> **日期**: 2026-05-10
> **基于**: `docs/architecture.md`
> **下游**: QEMU Dev Agent → 实现 QEMU 设备 | FW Dev Agent → 实现固件驱动 | Test Eng Agent → 编写测试用例

---

## 1. QEMU 侧接口

### 1.1 ENET 寄存器映射

| 偏移 | 名称 | 宽度 | 访问 | 复位值 | 说明 |
|------|------|------|------|--------|------|
| `0x0000` | `ECR` | 32 | RW | `0xF000_0100` | Ethernet Control (RESET, ETHEREN, DBSWP, DBGEN, EN1588) |
| `0x0004` | `EIR` | 32 | W1C | `0x0000_0000` | Interrupt Event (TXF, TXB, RXF, RXB, MII, EBERR, LC, RL, UN) |
| `0x0008` | `EIMR` | 32 | RW | `0x0000_0000` | Interrupt Mask (对应 EIR 各位) |
| `0x0010` | `RDAR` | 32 | RW | `0x0000_0000` | RX Descriptor Active (写 1 激活 RX BD 环) |
| `0x0014` | `TDAR` | 32 | RW | `0x0000_0000` | TX Descriptor Active (写 1 激活 TX BD 环) |
| `0x0024` | `ECR_MAGIC` | 32 | RW | `0x0000_0000` | ECR 写保护解锁 (写 `0x5A5A_5A5A` 后 ECR 可写) |
| `0x0040` | `MMFR` | 32 | RW | `0x0000_0000` | MDIO Frame (PA[4:0], RA[4:0], TA, DATA[15:0]) |
| `0x0044` | `MSCR` | 32 | RW | `0x0000_0040` | MDIO Speed Control (MII_SPEED) |
| `0x0048` | `MIBC` | 32 | RW | `0xC000_0000` | MIB Control (MIB_DIS, MIB_IDLE, MIB_CLEAR) |
| `0x0064` | `RCR` | 32 | RW | `0x05EE_0001` | RX Control (LOOP, MII_MODE, PROM, RMII_MODE, FCE, MAX_FL, NLC) |
| `0x0084` | `TCR` | 32 | RW | `0x0000_0010` | TX Control (RFC_PAUSE, TFC_PAUSE, FDEN, FCE, ADDINS) |
| `0x00C4` | `PALR` | 32 | RW | `0x0000_0000` | Physical Address Lower (MAC[31:0]) |
| `0x00C8` | `PAUR` | 32 | RW | `0x0000_8808` | Physical Address Upper (MAC[47:32] + Type field) |
| `0x00E4` | `OPD` | 32 | RW | `0x0001_0001` | Opcode/Pause Duration |
| `0x00EC` | `IAUR` | 32 | RW | `0x0000_0000` | Descriptor Individual Upper Address |
| `0x00F0` | `IALR` | 32 | RW | `0x0000_0000` | Descriptor Individual Lower Address |
| `0x00F4` | `GAUR` | 32 | RW | `0x0000_0000` | Descriptor Group Upper Address |
| `0x00F8` | `GALR` | 32 | RW | `0x0000_0000` | Descriptor Group Lower Address |
| `0x0100` | `TFWR` | 32 | RW | `0x0000_0000` | Transmit FIFO Watermark |
| `0x0144` | `RDSR` | 32 | RO | `0x0000_0000` | RX Descriptor Ring Start (BD 环基地址) |
| `0x0154` | `TDSR` | 32 | RW | `0x0000_0000` | TX Descriptor Ring Start (BD 环基地址) |
| `0x0160` | `MRBR` | 32 | RW | `0x0000_0000` | Max RX Buffer Size |
| `0x0184` | `RSFL` | 32 | RW | `0x0000_0000` | RX FIFO Section Full Threshold |
| `0x018C` | `RSEM` | 32 | RW | `0x0000_0000` | RX FIFO Section Empty Threshold |
| `0x0190` | `RAEM` | 32 | RW | `0x0000_0004` | RX FIFO Almost Empty Threshold |
| `0x0194` | `RAFL` | 32 | RW | `0x0000_0004` | RX FIFO Almost Full Threshold |
| `0x0198` | `TSEM` | 32 | RW | `0x0000_0060` | TX FIFO Section Empty Threshold |
| `0x019C` | `TAEM` | 32 | RW | `0x0000_0008` | TX FIFO Almost Empty Threshold |
| `0x01A0` | `TAFL` | 32 | RW | `0x0000_0008` | TX FIFO Almost Full Threshold |
| `0x01A4` | `TIPG` | 32 | RW | `0x0000_000C` | Transmit Inter-Packet Gap |
| `0x01C0` | `ATCR` | 32 | RW | `0x0000_0000` | Adjustable Timer Control (1588) |
| `0x01C4` | `ATVR` | 32 | RW | `0x0000_0000` | Adjustable Timer Value (1588) |
| `0x01C8` | `ATOFF` | 32 | RW | `0x0000_0000` | Timer Offset (1588) |
| `0x01CC` | `ATPER` | 32 | RW | `0x0000_0000` | Timer Period (1588) |
| `0x01D0` | `ATCOR` | 32 | RW | `0x0000_0000` | Timer Correction (1588) |
| `0x01D4` | `ATINC` | 32 | RW | `0x0000_0000` | Timer Increment (1588) |
| `0x01D8` | `ATSTMP` | 32 | RO | `0x0000_0000` | Timestamp of Last 1588 Frame |
| `0x0200` | `TGSR` | 32 | RW | `0x0000_0000` | Timer Global Status (1588) |
| `0x0208` | `TCSR0..3` | 32×4 | RW | — | Timer Control Status (1588, 4 个通道) |
| `0x0228` | `TCCR0..3` | 32×4 | RW | — | Timer Compare Capture (1588, 4 个通道) |
| `0x0284` | `RMON_T_DROP` ~ `RMON_R_P1024TO2047` | — | — | — | RMON 统计计数器 (Phase 1 可设为 read-as-zero) |

> ⚠️ **NEARLY CORRECT**: 上表基于 i.MX 系列 ENET 公共寄存器布局 (FEC/ENET)。RT1180 确切寄存器和偏移需参考手册后修正。Phase 1 优先实现加粗的核心寄存器。

### 1.2 EIR / EIMR 位定义 (中断事件)

| 位 | 名称 | 说明 |
|----|------|------|
| 23 | `TXF` | Transmit Frame — 一帧发送完成 |
| 21 | `TXB` | Transmit Buffer — TX BD 用完 |
| 25 | `RXF` | Receive Frame — 一帧接收完成 |
| 24 | `RXB` | Receive Buffer — RX BD 不够 |
| 27 | `MII` | MDIO 传输完成 |
| 22 | `EBERR` | Ethernet Bus Error (BD DMA 错误) |
| 2 | `LC` | Late Collision |
| 4 | `RL` | Collision Retry Limit |
| 5 | `UN` | Transmit FIFO Underrun |

> 中断逻辑: `irq = (EIR & EIMR) != 0`

### 1.3 MMFR 寄存器字段 (MDIO 帧)

| 位 | 名称 | 说明 |
|----|------|------|
| 31:30 | `ST` | Start (01 为 Clause 22) |
| 29:28 | `OP` | Opcode (10=读, 01=写) |
| 27:23 | `PA` | PHY Address (0-31) |
| 22:18 | `RA` | Register Address (0-31) |
| 17:16 | `TA` | Turnaround (写入时为 10, 读取时由 PHY 驱动) |
| 15:0 | `DATA` | 16-bit 数据 |

### 1.4 ENET Device 属性 (QOM Properties)

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `phy-addr` | `uint8` | `0` | PHY MDIO 地址 (DP83822 典型为 0 或 1) |
| `mac-address` | `str` | 自动生成 | MAC 地址，格式 `"02:00:00:00:00:01"` |
| `tx-ring-size` | `uint32` | 8 | TX Buffer Descriptor 环长度 |
| `rx-ring-size` | `uint32` | 8 | RX Buffer Descriptor 环长度 |
| `max-frame-size` | `uint32` | 1522 | 最大帧长 (含 VLAN tag) |

### 1.5 QEMU 网络后端连接

```bash
# TAP 后端 (需要 root)
qemu-system-arm -M imxrt1180-evk \
  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
  -device imxrt1180-enet,netdev=net0

# SLIRP 用户模式 (无需 root)
qemu-system-arm -M imxrt1180-evk \
  -netdev user,id=net0,hostfwd=tcp::8080-:80 \
  -device imxrt1180-enet,netdev=net0
```

---

## 2. DP83822 PHY MDIO 寄存器映射

| 地址 | 名称 | 宽度 | 访问 | 复位值 | 说明 |
|------|------|------|------|--------|------|
| `0x00` | `BMCR` | 16 | RW | `0x3100` | Basic Mode Control |
| `0x01` | `BMSR` | 16 | RO | `0x7849` | Basic Mode Status |
| `0x02` | `PHYIDR1` | 16 | RO | `0x2000` | PHY ID 1 (TI OUI[21:6]) |
| `0x03` | `PHYIDR2` | 16 | RO | `0xA221` | PHY ID 2 (OUI[5:0] + Model + Rev) |
| `0x04` | `ANAR` | 16 | RW | `0x0DE1` | Auto-Negotiation Advertisement |
| `0x05` | `ANLPAR` | 16 | RO | `0x0001` | Auto-Neg Link Partner Ability |
| `0x10` | `PHYSTS` | 16 | RO | `0x0000` | PHY Status (TI extended) |
| `0x11` | `MICR` | 16 | RW | `0x0000` | MII Interrupt Control |
| `0x12` | `MISR` | 16 | RO | `0x0000` | MII Interrupt Status |
| `0x17` | `RCSR` | 16 | RW | `0x0140` | RMII and Bypass Control |
| `0x19` | `LEDCR` | 16 | RW | `0x0440` | LED Control |
| `0x1F` | `PHYCR` | 16 | RW | `0x0000` | PHY Control (DP83822 extended) |

### PHY 行为模型 (Phase 1 简化)

- **链路状态**: 始终报告 Link Up (`BMSR[2] = 1, PHYSTS[0] = 1`)
- **自协商**: 固定报告 100M Full Duplex (`ANLPAR[10:7] = 0101`)
- **MDIO 读写**: 遵循 Clause 22 协议，由 ENET MMFR 驱动
- **PHY ID**: 硬编码 TI DP83822 (`PHYIDR1=0x2000, PHYIDR2=0xA221`)
- **未来扩展**: 链路状态可通过 QOM 属性 `link-status` 控制

---

## 3. 固件侧接口

### 3.1 ENET lwIP 驱动 API (`firmware/drivers/imxrt_enet.h`)

```c
/* 初始化 — 由 BAL_Init() 调用 */
err_t imxrt_enet_init(struct netif *netif);

/* 发送 — lwIP 调用 */
err_t imxrt_enet_output(struct netif *netif, struct pbuf *p);

/* 接收 — ISR 中断下半部调用 */
void imxrt_enet_input(struct netif *netif);

/* 中断服务例程 — 注册到 NVIC ENET IRQ (114) */
void ENET_IRQHandler(void);

/* 查询链路状态 */
uint32_t imxrt_enet_get_link_status(void);

/* 获取 MAC 地址 */
void imxrt_enet_get_mac_addr(uint8_t mac[6]);

/* 设置 MAC 地址 */
void imxrt_enet_set_mac_addr(struct netif *netif, const uint8_t mac[6]);
```

### 3.2 Buffer Descriptor 结构体

```c
/* ENET 标准 BD 结构 (LE, 与 i.MX ENET 硬件一致) */
typedef struct __attribute__((packed)) {
    uint16_t status;       /* R/E/W/L/M/BC/MC/LG/NO/CR/OV/TR */
    uint16_t length;       /* Data length (TX: 待发送; RX: buffer 大小) */
    uint32_t data_ptr;     /* 数据缓冲区指针 (物理地址) */
} imxrt_enet_bd_t;

/* BD 环基址对齐: 32 字节 */
/* BD 环位于 OCRAM 中，非 cacheable 区域 */

/* BD Status 位定义 */
#define ENET_BD_TX_R      (1 << 15) /* Ready (1=ENET owns) */
#define ENET_BD_TX_TO1    (1 << 14) /* TX Option 1 (wrap) */
#define ENET_BD_TX_W      (1 << 13) /* Wrap (ring end) */
#define ENET_BD_TX_TO2    (1 << 12) /* TX Option 2 (last) */
#define ENET_BD_TX_L      (1 << 11) /* Last in frame */
#define ENET_BD_TX_TC     (1 << 10) /* TX CRC */

#define ENET_BD_RX_E      (1 << 15) /* Empty (1=ENET owns) */
#define ENET_BD_RX_W      (1 << 13) /* Wrap */
#define ENET_BD_RX_L      (1 << 11) /* Last in frame */
#define ENET_BD_RX_M      (1 << 8)  /* Miss (promiscuous) */
#define ENET_BD_RX_BC     (1 << 7)  /* Broadcast */
#define ENET_BD_RX_MC     (1 << 6)  /* Multicast */
#define ENET_BD_RX_LG     (1 << 5)  /* Frame length violation */
#define ENET_BD_RX_NO     (1 << 4)  /* Non-octet aligned frame */
#define ENET_BD_RX_CR     (1 << 2)  /* CRC error */
#define ENET_BD_RX_OV     (1 << 1)  /* Overrun */
#define ENET_BD_RX_TR     (1 << 0)  /* Truncated */
```

### 3.3 板级抽象层 (BAL) 接口 (`firmware/bal/bal.h`)

```c
/* === BAL 初始化 === */
void BAL_Init(void);                    /* 总入口，初始化所有板级外设 */

/* === ENET 相关 === */
void BAL_ENET_Init(void);               /* 配置 ENET pinmux, 时钟, PHY */
void BAL_ENET_ResetPHY(void);           /* 硬件复位 DP83822 PHY */
void BAL_ENET_GetMACAddr(uint8_t *mac); /* 获取 MAC 地址 (OTP/Flash/生成) */
uint32_t BAL_ENET_GetClockHz(void);     /* 返回 ENET 时钟频率 (Hz) */

/* === 时钟 === */
void BAL_Clock_Init(void);              /* 初始化系统时钟树 */
uint32_t BAL_Clock_GetSystemCoreClock(void); /* 系统核心时钟 */
uint32_t BAL_Clock_GetENETClock(void);       /* ENET 外设时钟 */

/* === 调试 === */
void BAL_Debug_Init(void);              /* 调试通道初始化 (UART / semihosting) */
void BAL_Debug_PutChar(char c);         /* 输出单个字符 */
void BAL_Debug_PutString(const char *s); /* 输出字符串 */
```

### 3.4 板级配置文件模板 (`firmware/bal/config/`)

```c
/* evk_config.h — NXP RT1180 EVK */
#ifndef BAL_CONFIG_H
#define BAL_CONFIG_H

/* 板卡标识 */
#define BOARD_NAME              "NXP i.MX RT1180 EVK"
#define BOARD_VARIANT           "EVK"

/* 内存布局 */
#define ITCM_BASE               (0x00000000UL)
#define ITCM_SIZE               (256 * 1024)
#define DTCM_BASE               (0x20000000UL)
#define DTCM_SIZE               (256 * 1024)
#define OCRAM_BASE              (0x20200000UL)
#define OCRAM_SIZE              (512 * 1024)

/* ENET 配置 */
#define ENET_BASE_ADDR          (0x40424000UL)
#define ENET_IRQ_N              114
#define ENET_PHY_ADDR           0       /* DP83822 MDIO address */

/* PHY 配置 */
#define PHY_MDIO_CLAUSE         22      /* IEEE 802.3 Clause 22 */
#define PHY_RESET_GPIO          -1      /* -1 = no GPIO reset */
#define PHY_TYPE                "TI_DP83822"

/* 时钟 */
#define ENET_CLOCK_HZ           (50000000UL) /* 50 MHz RMII REF_CLK */
#define CORE_CLOCK_HZ           (600000000UL)
#define SYSTICK_CLOCK_HZ        (CORE_CLOCK_HZ)

/* 调试 */
#define DEBUG_UART_BASE         (0x40070000UL) /* LPUART1 */
#define DEBUG_UART_BAUD         115200
#define DEBUG_USE_SEMIHOSTING   1        /* 1=QEMU semihosting, 0=UART */

/* lwIP 配置 (lwipopts.h 引用) */
#define LWIP_IPADDR_STATIC      0        /* 0=DHCP, 1=Static IP */
#define LWIP_IPADDR0            10
#define LWIP_IPADDR1            0
#define LWIP_IPADDR2            2
#define LWIP_IPADDR3            15
#define LWIP_NETMASK0           255
#define LWIP_NETMASK1           255
#define LWIP_NETMASK2           255
#define LWIP_NETMASK3           0
#define LWIP_GW0                10
#define LWIP_GW1                0
#define LWIP_GW2                2
#define LWIP_GW3                2

#endif /* BAL_CONFIG_H */
```

---

## 4. QEMU ↔ 固件 通信协议

### 4.1 ARM Semihosting 协议

| 操作 | 代码 | 固件调用 | 用途 |
|------|------|----------|------|
| `SYS_WRITEC` | `0x03` | `__BKPT(0xAB)` + R0=0x03, R1=字符 | 输出单个字符到 stderr |
| `SYS_WRITE0` | `0x04` | 同上, R1=字符串指针 | 输出调试字符串 |
| `SYS_READC` | `0x07` | 同上, R0=0x07 | 读取一个字符 (阻塞) |

QEMU 启动参数: `-semihosting -semihosting-config enable=on,target=native`

### 4.2 QEMU Monitor 交互

固件可写入特定地址来触发 QEMU 动作（如 EXIT 用于测试），由 Test Eng 在 pytest 中利用:

```
写 0x10000000 ← 'E' 'X' 'I' 'T' → QEMU 退出 (qemu_test_exit)
写 0x10000004 ← 任意值 → 测试检查点标记
```

### 4.3 网络测试约定

| 测试 | 约定 |
|------|------|
| Ping 验证 | 固件 IP 固定 `10.0.2.15` (SLIRP 默认) 或 DHCP 获取 (TAP 后端) |
| HTTP 验证 | HTTP 端口 80，根路径返回包含 "i.MX RT1180" 的 HTML |
| MQTT 验证 | 连接到运行在宿主机 localhost:1883 的 Mosquitto Broker |
| 超时等待 | 测试脚本最多等待 30 秒等待网络就绪 |

---

## 5. 构建接口

### 5.1 固件 Makefile 目标

```makefile
# 编译目标切换
BOARD ?= evk    # evk | custom

# 编译
make BOARD=evk all

# 清理
make clean

# 运行在 QEMU 中
make run-qemu    # 启动 QEMU + 加载固件 + semihosting

# 调试
make debug-qemu  # 启动 QEMU + GDB stub (port 1234)

# Flash (真实硬件, 后续)
make flash       # 使用 OpenOCD / J-Link
```

### 5.2 QEMU 集成构建

```
# 在 QEMU 源码树中:
./configure --target-list=arm-softmmu
make -j$(nproc)

# 新增文件位置:
hw/arm/imxrt1180_soc.c
hw/arm/imxrt1180_evk.c
hw/net/imxrt1180_enet.c
hw/net/imxrt1180_dp83822_phy.c
include/hw/arm/imxrt1180_soc.h
include/hw/net/imxrt1180_enet.h
include/hw/net/imxrt1180_dp83822_phy.h
```

---

## 6. 测试接口

### 6.1 pytest 集成测试脚本结构

```python
# tests/integration/test_enet.py
import pytest
import subprocess
import time
import requests

@pytest.fixture
def qemu_instance():
    """启动 QEMU 实例，返回 (process, ip)"""
    proc = subprocess.Popen([
        "qemu-system-arm",
        "-M", "imxrt1180-evk",
        "-kernel", "firmware/build/imxrt1180_enet_demo.elf",
        "-netdev", "user,id=net0,hostfwd=tcp::8080-:80",
        "-device", "imxrt1180-enet,netdev=net0",
        "-semihosting",
        "-nographic",
        "-serial", "stdio"
    ])
    time.sleep(5)  # 等待启动
    yield proc, "127.0.0.1"
    proc.terminate()

def test_ping(qemu_instance):
    """验证 ICMP Ping 可达"""
    proc, ip = qemu_instance
    result = subprocess.run(["ping", "-n", "4", "-w", "2000", ip],
                            capture_output=True, text=True)
    assert result.returncode == 0, f"Ping failed: {result.stderr}"

def test_http_server(qemu_instance):
    """验证 HTTP Server 可访问"""
    proc, ip = qemu_instance
    resp = requests.get(f"http://{ip}:8080/", timeout=10)
    assert resp.status_code == 200
    assert "i.MX RT1180" in resp.text
```

---

> **→ 下一步**: QEMU Dev Agent 使用第 1 章寄存器映射实现设备模型；FW Dev Agent 使用第 3 章 API 和结构体实现驱动和固件。
