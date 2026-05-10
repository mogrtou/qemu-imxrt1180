# PRD: i.MX RT1180 以太网功能 & lwIP 协议栈移植

> **文档状态**: 已确认
> **作者**: PM Agent
> **日期**: 2026-05-10
> **下游交接**: Architect Agent → `docs/architecture.md` + `docs/interfaces.md`

---

## 1. 背景与目标

### 背景
i.MX RT1180 是 NXP 跨界 MCU，内置 ENET 以太网控制器（支持双 MAC + 片上 L2 Switch），可外接 TI DP83822 PHY 实现 10/100 Mbps 以太网通信。当前项目处于 Phase 0，需要定义首个核心功能——以太网网络能力。

### 目标
1. 在 QEMU 中建模 RT1180 ENET 控制器 + DP83822 PHY，使模拟器具备网络通信能力。
2. 移植 lwIP + mbedTLS 协议栈到 FreeRTOS，实现完整 TCP/IP 及其应用层。
3. 固件同时支持 QEMU 模拟和真实硬件（NXP EVK / 用户自定义板）。
4. 验证场景：主机 Ping 通模拟设备 + HTTP Server 可访问。

---

## 2. 用户故事

| ID | 角色 | 需求 | 目的 |
|----|------|------|------|
| US1 | 固件开发者 | 在 QEMU 中运行带有完整 TCP/IP 协议栈的 RT1180 固件 | 无需真实硬件即可开发调试网络应用 |
| US2 | 测试工程师 | 在 QEMU 中从本机 Ping 通模拟的 RT1180 设备 | 验证 L2/L3 网络连通性 |
| US3 | 应用开发者 | 在模拟 RT1180 上部署 HTTP Server | 验证应用层协议栈可用性 |
| US4 | 硬件工程师 | 使用同一份固件在自定义板卡上运行 | 设计与验证阶段代码复用 |
| US5 | 系统集成者 | 后续启用双 MAC + Switch 功能 | 支持工业网络冗余/分段场景 |

---

## 3. 功能需求

### 3.1 分阶段规划

Phase 1 (本项目): 单 ENET MAC (ENET1) + DP83822 PHY
lwIP + mbedTLS + FreeRTOS
Ping + HTTP Server 验证
─────────────────────────────
Phase 2 (后续): 双 MAC (ENET1 + ENET2) + 片上 L2 Switch
增强网络拓扑能力

---

### 3.2 Must Have (Phase 1)

| ID | 需求 | 说明 |
|----|------|------|
| **F1** | QEMU ENET1 设备模型 | 建模 RT1180 ENET MAC 控制器（寄存器级），含 DMA、Buffer Descriptor、MDIO 接口 |
| **F2** | QEMU DP83822 PHY 模型 | 建模 TI DP83822 以太网 PHY（至少覆盖 MDIO 寄存器访问 + 链路状态上报） |
| **F3** | QEMU 网络后端 | 支持 TAP 后端（接入真实网络）和 user-mode/SLIRP 后端（便捷调试） |
| **F4** | lwIP 2.x 移植 | 在 FreeRTOS + Cortex-M7 (ARMv7E-M) 上运行 lwIP 最新稳定版（netconn 或 socket API） |
| **F5** | mbedTLS 移植 | 最新稳定版 mbedTLS，与 lwIP altcp 层集成 |
| **F6** | lwIP ENET 驱动 | 编写 RT1180 ENET 的 lwIP 驱动层（初始化、收发、中断处理），可同时工作在 QEMU 模拟和真实硬件 |
| **F7** | FreeRTOS 移植 | Cortex-M7 的 FreeRTOS 移植（含 SysTick 时钟），作为固件 RTOS 基座 |
| **F8** | ICMP Ping 可达 | 主机可通过 `ping <IP>` 收到模拟 RT1180 的 Echo Reply |
| **F9** | HTTP Server | 运行 lwIP httpd，主机浏览器/curl 可访问并获取页面 |
| **F10** | ARM Semihosting | 使用 semihosting 输出调试日志，方便开发排错 |

---

### 3.3 Should Have (Phase 1)

| ID | 需求 | 说明 |
|----|------|------|
| **F11** | DHCP 客户端 | lwIP DHCP，自动获取 IP 地址 |
| **F12** | MQTT 客户端 | lwIP 内置 mqtt app，可连接 MQTT Broker 发布/订阅消息 |
| **F13** | 板级抽象层 | 固件中设计 Board Abstraction Layer，通过配置切换 EVK / 自定义板卡（pinmux、时钟、PHY 地址等） |
| **F14** | qtest 单元测试 | ENET 寄存器读写测试（复位值、关键寄存器行为） |
| **F15** | 文档 | ENET 设备模型接口文档 (`docs/interfaces.md` 中 ENET 章节) |

---

### 3.4 Could Have (Phase 1)

| ID | 需求 | 说明 |
|----|------|------|
| **F16** | HTTPS Server | mbedTLS 证书 + lwIP altcp TLS 层实现 HTTPS |
| **F17** | DNS 客户端 | lwIP DNS，支持域名解析 |
| **F18** | 多并发连接 | HTTP Server 同时处理多个 TCP 连接 |
| **F19** | 吞吐量测试 | iperf 类似工具验证 TCP/UDP 吞吐量（目标 > 数十 Mbps in QEMU） |

---

### 3.5 Won't Have (Phase 1 / 本迭代)

| ID | 需求 | 说明 |
|----|------|------|
| **W1** | ENET2 设备模型 | 第二个 MAC 控制器 — 推迟到 Phase 2 |
| **W2** | 片上 L2 Switch | 双端口以太网交换功能 — 推迟到 Phase 2 |
| **W3** | IEEE 1588 / TSN | 精确时间同步协议 — 待需求明确 |
| **W4** | 真实硬件验证 | 用户自定义板准备就绪后进行 |
| **W5** | EtherCAT / PROFINET | 工业以太网协议 — 无当前需求 |

---

## 4. 非功能需求

| ID | 类别 | 要求 |
|----|------|------|
| **NF1** | 代码规范 | QEMU 部分遵循上游 `CODING_STYLE`（4 空格、90 列宽、checkpatch 通过）；固件遵循 Linux kernel 风格（Tab 缩进） |
| **NF2** | 文档语言 | 所有文档使用中文 |
| **NF3** | 可移植性 | 固件通过 `#ifdef` / HAL 抽象同时支持 QEMU 和真实硬件，编译目标通过配置切换 |
| **NF4** | 构建系统 | 固件使用 Makefile + arm-none-eabi-gcc；QEMU 集成使用 QEMU 原生 Meson 构建 |
| **NF5** | 测试自动化 | 集成测试通过 pytest 脚本自动化：启动 QEMU → 等待网络就绪 → ping → HTTP GET → 断言 |
| **NF6** | CI/CD | GitHub Actions 自动构建 QEMU 镜像 + 编译固件 + 运行集成测试 |

---

## 5. 验收标准（按用户故事）

### US1 — 固件在 QEMU 中运行网络协议栈
- ✅ QEMU 启动 RT1180 机器后，lwIP 初始化成功（semihosting 日志确认）
- ✅ lwIP 成功获取 IP（DHCP 或静态配置）
- ✅ 可在 QEMU monitor 中查看 ENET 寄存器状态

### US2 — 从主机 Ping 通模拟设备
- ✅ `ping <RT1180_IP> -c 4` 返回 4 次成功回复，延迟 < 10ms（QEMU 内）
- ✅ Wireshark / tcpdump 在 TAP 接口上可抓取 ICMP 包
- ✅ ARP 解析正常（`arp -a` 可见对应条目）

### US3 — HTTP Server 可访问
- ✅ `curl http://<RT1180_IP>/` 返回 HTTP 200 + HTML 页面
- ✅ 页面包含设备信息（如 "i.MX RT1180 lwIP HTTP Server"）

### US4 — 固件在自定义板上可用
- ✅ 同一份固件源码，通过切换板级配置文件，可编译出适配自定义板的二进制
- ✅ 自定义板配置项包括：PHY 地址、时钟频率、pinmux 设置、内存布局

### US5 — Phase 2 可平滑升级
- ✅ ENET 设备模型架构预留双 MAC 扩展点（Architect 设计评审）
- ✅ 固件网络初始化代码不硬编码单 MAC 假设

---

## 6. 里程碑计划

| 里程碑 | 内容 | 预估产出 | 依赖 |
|--------|------|----------|------|
| **M0** | 架构设计 | `architecture.md` + `interfaces.md`（ENET 章节） | PRD 确认 |
| **M1** | QEMU ENET + PHY 模型 | C 源码、qtest 用例 | M0 完成 |
| **M2** | FreeRTOS + lwIP + mbedTLS 移植 | 固件源码、Makefile | M1 完成 |
| **M3** | ENET 驱动 + 网络后端对接 | 固件可收发网络包 | M1 + M2 |
| **M4** | Ping + HTTP Server 集成测试 | pytest 脚本 + 固件 demo | M3 |
| **M5** | CI/CD 流水线 | `.github/workflows/` | M4 |
| **M6** | 板级抽象层 | 抽象接口 + EVK 配置 | M3 |

---

## 7. 开放问题 / 风险

| ID | 问题 | 影响 | 应对 |
|----|------|------|------|
| **Q1** | RT1180 ENET 寄存器细节未知（用户无参考手册） | ENET 模型可能与真实硬件有偏差 | Architect 先基于公开的 EVK 手册 + i.MX ENET 公共知识建模，标注 NEARLY CORRECT 区域，后续修正 |
| **Q2** | 双 ENET + Switch 的 Switch 拓扑结构未明确 | Phase 2 设计可能需要返工 | Phase 1 预留架构扩展点，Phase 2 前需用户提供 Switch 使用场景细节 |
| **Q3** | 自定义板硬件设计细节待定 | 板级抽象层设计可能不匹配 | 先以 NXP EVK 为基线建模，BAL 接口尽可能通用化，等硬件细节就绪后再适配 |
| **Q4** | 用户是否持有 RT1180 参考手册？ | 开发精度受限 | 建议获取 NXP RT1180 Reference Manual（通常需 NDA），至少获取 ENET 章节 |

---

## 8. 术语表

| 术语 | 说明 |
|------|------|
| ENET | i.MX RT1180 内置以太网 MAC 控制器 |
| DP83822 | TI 10/100 Mbps 以太网 PHY 收发器 |
| lwIP | Lightweight IP — 嵌入式开源 TCP/IP 协议栈 |
| mbedTLS | ARM 维护的开源 TLS 库 |
| TAP | QEMU 虚拟网络后端，将虚拟机接入宿主机网络 |
| SLIRP | QEMU user-mode 网络后端（NAT 模式，无需 root 权限） |
| BAL | Board Abstraction Layer — 板级抽象层 |
| MDIO | Management Data Input/Output — 以太网 PHY 管理接口 |
| MII / RMII | Media Independent Interface / Reduced MII — MAC-PHY 数据接口 |
| BD | Buffer Descriptor — ENET DMA 的缓冲区描述符 |

---

> **→ 下一步**: 请切换到 **Architect Agent**，开始架构设计与接口定义。
> Architect Agent 将输出 `docs/architecture.md` 和 `docs/interfaces.md`。