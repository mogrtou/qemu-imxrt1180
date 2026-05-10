# i.MX RT1180 以太网子系统架构设计

> **文档状态**: Phase 1 设计
> **作者**: Architect Agent
> **日期**: 2026-05-10
> **基于**: `docs/prd.md` (PM Agent)
> **下游**: QEMU Dev Agent, FW Dev Agent, Test Eng Agent

---

## 1. 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                    Host PC (Windows/Linux)                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              QEMU Process (arm-softmmu)                │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │            Machine: imxrt1180-evk                 │  │  │
│  │  │  ┌───────────────────────────────────────────┐  │  │  │
│  │  │  │           SoC: imxrt1180-soc               │  │  │  │
│  │  │  │                                           │  │  │  │
│  │  │  │  ┌──────────┐  ┌──────────────────────┐  │  │  │  │
│  │  │  │  │ Cortex-M7│  │   ENET1 (SysBus)     │  │  │  │  │
│  │  │  │  │ (armv7m) │  │   imxrt1180-enet     │  │  │  │  │
│  │  │  │  │ NVIC     │◄─┤   irq ───────────────►│  │  │  │  │
│  │  │  │  │ SysTick  │  │   mmio ─── regs       │  │  │  │  │
│  │  │  │  └──────────┘  │   mdio ─── PHY        │  │  │  │  │
│  │  │  │                │   nic ──── net backend │  │  │  │  │
│  │  │  │  ┌──────────┐  └──────────┬───────────┘  │  │  │  │
│  │  │  │  │ LPUART1  │             │              │  │  │  │
│  │  │  │  │ (debug)  │         MDIO bus           │  │  │  │
│  │  │  │  └──────────┘             │              │  │  │  │
│  │  │  │                    ┌──────┴──────┐       │  │  │  │
│  │  │  │                    │ DP83822 PHY │       │  │  │  │
│  │  │  │                    │ (MDIO regs) │       │  │  │  │
│  │  │  │                    └──────┬──────┘       │  │  │  │
│  │  │  │                           │              │  │  │  │
│  │  │  │                    MII/RMII data path     │  │  │  │
│  │  │  │                           │              │  │  │  │
│  │  │  └───────────────────────────┼──────────────┘  │  │  │
│  │  │                              │                 │  │  │
│  │  └──────────────────────────────┼─────────────────┘  │  │
│  │                   QEMU net subsystem                  │  │
│  │              ┌────────┐    ┌──────────┐               │  │
│  │              │  TAP   │    │  SLIRP   │               │  │
│  │              └───┬────┘    └────┬─────┘               │  │
│  └──────────────────┼─────────────┼─────────────────────┘  │
│                     │             │                         │
│              /dev/tap0      user-mode NAT                   │
│                     │             │                         │
│              Host Network Stack                             │
│                     │                                       │
│              ping / curl → <RT1180_IP>                      │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. QEMU 设备模型架构

### 2.1 类型层次 (QOM Tree)

```
OBJECT (root)
├── machine (imxrt1180-evk)
│   ├── soc (imxrt1180-soc)
│   │   ├── cpu (armv7m, "cortex-m7")
│   │   │   ├── nvic (armv7m_nvic)
│   │   │   └── systick (armv7m_systick)
│   │   ├── enet1 (imxrt1180-enet)
│   │   │   └── phy (imxrt1180-dp83822-phy)  ← 嵌入为子对象
│   │   ├── lpuart1 (imxrt1180-lpuart)
│   │   ├── ocram (memory-region)
│   │   ├── itcm  (memory-region)
│   │   └── dtcm  (memory-region)
│   └── net-client (nic)  ← QEMU 网络后端
```

> **Phase 2 扩展**: `enet2` 作为 `enet1` 的同级节点加入 SoC。

### 2.2 新增 QOM 类型

| 类型宏 | 类型名 | 父类型 | 来源文件 |
|---------|--------|--------|----------|
| `TYPE_IMXRT1180_SOC` | `"imxrt1180-soc"` | `TYPE_SYS_BUS_DEVICE` | `hw/arm/imxrt1180_soc.c` |
| `TYPE_IMXRT1180_ENET` | `"imxrt1180-enet"` | `TYPE_SYS_BUS_DEVICE` | `hw/net/imxrt1180_enet.c` |
| `TYPE_IMXRT1180_DP83822_PHY` | `"imxrt1180-dp83822-phy"` | `TYPE_OBJECT` | `hw/net/imxrt1180_dp83822_phy.c` |
| `TYPE_IMXRT1180_EVK` | ``"imxrt1180-evk"`` | `TYPE_MACHINE` | `hw/arm/imxrt1180_evk.c` |

### 2.3 ENET 设备内部结构

```
┌──── imxrt1180-enet ────────────────────────────────────┐
│                                                        │
│  MemoryRegion (0x1000 size, DEVICE_NATIVE_ENDIAN)       │
│  ┌──────────────────────────────────────────────────┐  │
│  │  MAC Registers (ECR, EIMR, EIR, PALR, PAUR...)   │  │
│  │  MII/MDIO Registers (MSCR, MMFR, MSR)            │  │
│  │  DMA / BD Registers (TDAR, RDAR, TDSR, RDSR...)  │  │
│  │  RMON Counters (optional, read-as-zero)           │  │
│  │  IEEE 1588 Regs (optional, read-as-zero)          │  │
│  └──────────────────────────────────────────────────┘  │
│                                                        │
│  IRQ lines:                                            │
│  ┌──────┐  ┌─────────┐  ┌──────────┐                 │
│  │ irq  │  │ irq_rx  │  │ irq_tx   │  (合并为单IRQ) │
│  └──┬───┘  └────┬────┘  └────┬─────┘                 │
│     └───────────┴────────────┘                         │
│                 │                                      │
│              NVIC IRQ 114                              │
│                                                        │
│  MDIO Link → imxrt1180-dp83822-phy                     │
│  ┌──────────────────────────────────────────────────┐  │
│  │  PHY 对象嵌入 (或指针引用)                        │  │
│  │  mdio_read(phy_addr, reg) → 返回 PHY 寄存器值     │  │
│  │  mdio_write(phy_addr, reg, val) → 更新 PHY 寄存器 │  │
│  │  link_status 回调 → 更新 ENET MSR                 │  │
│  └──────────────────────────────────────────────────┘  │
│                                                        │
│  NICConf ──→ net/net.c                                │
│  ┌──────────────────────────────────────────────────┐  │
│  │  NICState *nic                                    │  │
│  │  qemu_send_packet(nic, buf, len)                  │  │
│  │  接收回调: enet_receive(nc, buf, len)              │  │
│  │  can_receive: enet_can_receive(nc)                │  │
│  │  link_status_changed: 链路状态变化通知             │  │
│  └──────────────────────────────────────────────────┘  │
│                                                        │
│  Buffer Descriptor Rings:                              │
│  ┌──────────────────────────────────────────────────┐  │
│  │  TX Ring (TDAR): 固件在 OCRAM 中维护 BD 环        │  │
│  │  RX Ring (RDAR): 固件在 OCRAM 中维护 BD 环        │  │
│  │  DMA 通过 cpu_physical_memory_read/write 访问     │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

### 2.4 ENET ↔ PHY 数据流

```
【发送路径】:
  QEMU net backend → nic_receive() → enet_receive()
    → 读取 RX BD (从 RAM DMA)
    → 填充数据到 BD 指向的 buffer
    → 写回 BD 状态
    → 拉高 RX IRQ

【接收路径】:
  固件写 TX BD → 固件写 TDAR
    → ENET 检测 TDAR 写入
    → 从 RAM DMA 读取 TX BD + buffer
    → qemu_send_packet(nic, ...)
    → 写回 TX BD 状态
    → 拉高 TX IRQ

【MDIO 路径】:
  固件写 MMFR 寄存器 (PHY addr, reg addr, data)
    → enet_mmfr_write()
    → phy->mdio_write(phy_addr, reg_addr, data)
  固件读 MMFR 寄存器
    → enet_mmfr_read()
    → phy->mdio_read(phy_addr, reg_addr) → 返回数据
```

---

## 3. 内存映射

### 3.1 系统级内存映射 (Phase 1)

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| ITCM | `0x0000_0000` | 256 KB | 指令 TCM |
| DTCM | `0x2000_0000` | 256 KB | 数据 TCM |
| OCRAM | `0x2020_0000` | 512 KB | 片上 RAM（lwIP 堆栈、BD 环、网络 buffer） |
| Peripherals | `0x4000_0000` | 2 MB | 外设寄存器空间 |
| ENET1 | `0x4042_4000` | 4 KB | ENET1 MAC 寄存器 **(NEARLY CORRECT)** |
| LPUART1 | `0x4007_0000` | 4 KB | 调试串口 |
| NVIC | `0xE000_E000` | — | ARM 私有外设（由 armv7m 提供） |
| SysTick | 同上 | — | ARM 私有外设（由 armv7m 提供） |

> ⚠️ **NEARLY CORRECT**: ENET 基地址 `0x4042_4000` 来自 i.MX RT 系列公开文档推断。RT1180 确切地址需参考手册验证。

### 3.2 Phase 2 预留

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| ENET2 | `0x4042_8000` | 4 KB | ENET2 MAC 寄存器 **(预估)** |
| Switch | `0x4043_0000` | 4 KB | 片上 L2 Switch 寄存器 **(预估)** |

---

## 4. 中断路由

| 设备 | NVIC IRQ 编号 | 优先级 | 说明 |
|------|---------------|--------|------|
| ENET1 | **114** | 可配置 | ENET MAC 综合中断（TX/RX/错误合并）**(NEARLY CORRECT)** |
| ENET1 1588 Timer | 115 | — | Phase 2 / Won't have |
| ENET2 | 116 | 可配置 | Phase 2 预留 |
| LPUART1 | 20 | 可配置 | 调试输出 |
| SysTick | -1 (内部) | 可配置 | FreeRTOS 时钟节拍 |

> ⚠️ **NEARLY CORRECT**: NVIC IRQ 编号基于 i.MX RT1060 参考。RT1180 确切编号需参考手册。

---

## 5. 固件架构

### 5.1 分层架构

```
┌──────────────────────────────────────────────────────────┐
│                    main.c (入口)                          │
│  1. HAL_Init()           — 系统时钟、NVIC 优先级组       │
│  2. BAL_Init()           — 板级外设初始化 (pinmux, PHY)  │
│  3. FreeRTOS_Init()      — 启动调度器                    │
│  4. lwip_init_task()     — 网络任务入口                  │
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│           Application Tasks (FreeRTOS Tasks)              │
│  ┌────────────┐  ┌─────────────┐  ┌──────────────────┐  │
│  │ httpd_task │  │ mqtt_task   │  │ network_monitor  │  │
│  │  (F9)      │  │  (F12)      │  │   (optional)     │  │
│  └─────┬──────┘  └──────┬──────┘  └────────┬─────────┘  │
│        │                │                   │            │
├────────┴────────────────┴───────────────────┴────────────┤
│                   lwIP Stack (lwip/)                      │
│  ┌──────────────────────────────────────────────────┐    │
│  │  netconn API (推荐) 或 socket API                 │    │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌───────┐  │    │
│  │  │ HTTP │ │MQTT  │ │DHCP  │ │ DNS  │ │ altcp │  │    │
│  │  │ app  │ │ app  │ │(F11) │ │(F17) │ │(TLS) │  │    │
│  │  └──────┘ └──────┘ └──────┘ └──────┘ └──┬───┘  │    │
│  │  ┌──────────────────────────────────────┐│      │    │
│  │  │         TCP / UDP                    ││      │    │
│  │  │         IPv4 (ICMP, ARP)             ││      │    │
│  │  │         netif (network interface)    ││      │    │
│  │  └────────────────┬─────────────────────┘│      │    │
│  └───────────────────┼──────────────────────┼──────┘    │
│                      │                      │           │
├──────────────────────┼──────────────────────┼───────────┤
│            mbedTLS (mbedtls/)               │           │
│  ┌──────────────────┼──────────────────────┼──────┐    │
│  │  TLS/DTLS        │  crypto              │      │    │
│  └──────────────────┴──────────────────────┘      │    │
│                                                   │    │
├───────────────────────────────────────────────────┼────┤
│        ENET Driver (firmware/drivers/imxrt_enet.c)      │
│  ┌────────────────────────────────────────────────┐    │
│  │  low_level_init(netif)   — MAC/PHY 初始化       │    │
│  │  low_level_output(netif, pbuf) — 发包           │    │
│  │  low_level_input(netif)      — 收包 (IRQ 驱动)  │    │
│  │  enet_isr()                  — 中断处理          │    │
│  │  BD Ring 管理 (TX/RX 描述符)                    │    │
│  └───────────────────┬────────────────────────────┘    │
│                      │                                  │
├──────────────────────┼──────────────────────────────────┤
│      Board Abstraction Layer (firmware/bal/)             │
│  ┌───────────────────┴────────────────────────────┐    │
│  │  bal_config.h        — 板级编译配置             │    │
│  │  bal_enet_pinmux.c   — ENET pinmux              │    │
│  │  bal_clock.c         — 时钟树配置               │    │
│  │  bal_phy.c           — PHY 复位/地址配置        │    │
│  │  ┌──────────────┐  ┌──────────────────┐        │    │
│  │  │ evk_config.h │  │ custom_config.h  │        │    │
│  │  │ (NXP EVK)    │  │ (用户自定义板)    │        │    │
│  │  └──────────────┘  └──────────────────┘        │    │
│  └────────────────────────────────────────────────┘    │
│                                                         │
├─────────────────────────────────────────────────────────┤
│              FreeRTOS Kernel (FreeRTOS/)                 │
│  ┌──────────────────────────────────────────────────┐  │
│  │  tasks, queues, semaphores, timers, event groups │  │
│  │  port.c (ARM Cortex-M7, ARMv7E-M)               │  │
│  │  portmacro.h                                     │  │
│  │  FreeRTOSConfig.h                                │  │
│  │  SysTick ISR → xPortSysTickHandler()             │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
├─────────────────────────────────────────────────────────┤
│          CMSIS / Startup (firmware/cmsis/)               │
│  ┌──────────────────────────────────────────────────┐  │
│  │  startup_imxrt1180.c  — 复位向量, 中断向量表     │  │
│  │  system_imxrt1180.c   — SystemInit(), 时钟初始化 │  │
│  │  linker_imxrt1180.ld  — 链接脚本                  │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
├─────────────────────────────────────────────────────────┤
│                 Hardware (QEMU / Silicon)                │
│  Cortex-M7 | ITCM | DTCM | OCRAM | ENET | DP83822       │
└─────────────────────────────────────────────────────────┘
```

### 5.2 固件源码目录结构

```
firmware/
├── CMakeLists.txt / Makefile
├── main.c                          # 入口函数
├── FreeRTOSConfig.h
├── lwipopts.h                      # lwIP 配置
├── mbedtls_config.h                # mbedTLS 配置
│
├── cmsis/                          # CMSIS 层
│   ├── startup_imxrt1180.c
│   ├── system_imxrt1180.c
│   └── linker/
│       ├── imxrt1180_evk.ld
│       └── imxrt1180_custom.ld
│
├── bal/                            # 板级抽象层
│   ├── bal.h                       # BAL 公共接口
│   ├── bal_enet.c                  # ENET pinmux/时钟/配置
│   ├── bal_phy.c                   # PHY 复位和配置
│   ├── bal_clock.c                 # 时钟树
│   └── config/
│       ├── evk_config.h            # NXP EVK 板配置
│       └── custom_config.h.tmpl    # 用户自定义板模板
│
├── drivers/                        # 外设驱动
│   └── imxrt_enet.c                # ENET lwIP 驱动
│       └── imxrt_enet.h            # 驱动头文件
│
├── apps/                           # 应用层
│   ├── http_server.c               # HTTP Server 任务
│   └── mqtt_client.c               # MQTT Client 任务 (Should Have)
│
├── third_party/                    # 第三方库 (git submodules)
│   ├── lwip/                       # lwIP 2.x
│   ├── mbedtls/                    # mbedTLS 3.x
│   └── FreeRTOS/                   # FreeRTOS Kernel
│
└── tests/                          # 固件单元测试 (可选)
    └── test_enet_driver.c
```

### 5.3 任务设计

| 任务 | 优先级 | 栈大小 | 职责 |
|------|--------|--------|------|
| `enet_rx_task` | 高 (3) | 1024 | 等待 ENET RX 信号量 → 调用 `low_level_input()` |
| `lwip_tcpip_task` | 中 (2) | 2048 | lwIP 内核线程 (tcpip_thread) |
| `httpd_task` | 低 (1) | 2048 | HTTP Server 监听线程 |
| `mqtt_task` | 低 (1) | 2048 | MQTT Client 线程 (Should Have) |
| `idle_task` | 0 | configMINIMAL | FreeRTOS 空闲任务 |

### 5.4 数据流：一次 HTTP 请求

```
Host PC                          QEMU RT1180                        固件
──────                          ──────────────                      ────
  │                                  │                                │
  │  curl http://10.0.2.15/         │                                │
  │────── TCP SYN ──────────────────►│                                │
  │                                  │── qemu_recv_packet() ─────────►│
  │                                  │   enet_receive()               │
  │                                  │   DMA to BD buffer             │
  │                                  │   IRQ 114 ────────────────────►│
  │                                  │                   enet_isr()   │
  │                                  │                   portYIELD()  │
  │                                  │                   enet_rx_task │
  │                                  │                   low_level_input()
  │                                  │                   pbuf → lwIP  │
  │                                  │                   tcp_input()  │
  │                                  │                   httpd 处理   │
  │                                  │   ◄── low_level_output()       │
  │                                  │   ◄── qemu_send_packet()       │
  │◄── TCP SYN-ACK ──────────────── │                                │
  │                                  │                                │
  │  (... TCP 握手完成 ...)          │                                │
  │                                  │                                │
  │────── GET / HTTP/1.1 ───────────►│                                │
  │                                  │──── (同上收包路径) ────────────►│
  │                                  │                   httpd 生成   │
  │                                  │                    HTML 响应   │
  │◄── HTTP 200 + HTML ──────────── │                                │
  │                                  │                                │
```

---

## 6. Phase 2 扩展点

| 扩展点 | 位置 | 说明 |
|--------|------|------|
| ENET2 设备 | `imxrt1180_soc.c` | 创建第二个 `imxrt1180-enet` 实例，映射到独立基地址 |
| 双 MAC 中断 | NVIC IRQ 116 | ENET2 单独中断线 |
| Switch 对象 | 新类型 `imxrt1180-enet-switch` | 连接到两个 ENET 的 MDIO/MII，管理端口转发 |
| netif 多实例 | `firmware/drivers/imxrt_enet.c` | lwIP 支持多 netif (netif_add 两次) |
| BAL 扩展 | `bal/config/` | 新增自定义板配置 |

---

## 7. 并行化分析

```
                    M0: 架构设计 (本文档)
                   /                    \
          M1: QEMU 设备模型          M2: 固件移植
          (QEMU Dev Agent)          (FW Dev Agent)
          ┌─────────────────┐       ┌─────────────────┐
          │ imxrt1180_soc   │       │ FreeRTOS port    │
          │ imxrt1180_enet  │       │ lwIP + mbedTLS   │
          │ dp83822_phy     │       │ startup/CMSIS    │
          │ imxrt1180_evk   │       │ linker scripts   │
          └────────┬────────┘       └────────┬────────┘
                   │                         │
                   └──────────┬──────────────┘
                              │
                    M3: ENET 驱动 & 集成
                   (FW Dev + QEMU Dev 协作)
                   ┌────────────────────┐
                   │ imxrt_enet.c       │← 需要 M1 提供的寄存器定义
                   │ 网络后端对接       │← QEMU Dev 提供 net backend 参数
                   └────────┬──────────┘
                            │
                    M4: 集成测试 (Test Eng)
                   ┌────────────────────┐
                   │ pytest + ping + curl│← 需要 M3 完成
                   └────────┬──────────┘
                            │
                    M5: CI/CD (DevOps)
                   ┌────────────────────┐
                   │ GitHub Actions      │
                   └────────────────────┘
```

**并行点**: M1 和 M2 可完全并行开始，因为它们通过 `docs/interfaces.md` 中的接口契约解耦。

---

> **→ 下一步**: QEMU Dev Agent 参考第 2 章实现设备模型，FW Dev Agent 参考第 5 章实现固件。
