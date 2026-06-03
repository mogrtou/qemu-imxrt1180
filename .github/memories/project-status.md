# 项目整体状态 (2026-05-11 更新于 FW Dev M3 完成)

## 里程碑进度
| 里程碑 | 内容 | 状态 | 产出 |
|--------|------|------|------|
| M0 | 架构设计 | ✅ 完成 | architecture.md, interfaces.md, 7 ADR |
| M1 | QEMU ENET+PHY 模型 | ✅ 完成 | enet.c(900+行/DMA/BD/TX/RX), dp83822_phy.c, SoC, Machine, Meson, qtest |
| M2 | FreeRTOS+lwIP+mbedTLS | ✅ 骨架编译 | firmware.elf 153KB — 三库编译链接通过 |
| M3 | ENET驱动+lwIP+HTTP | ✅ 完成 | imxrt_enet.c, sys_arch.c(FreeRTOS), lwipopts.h(TCP/UDP/DHCP), httpd_task.c, firmware.elf 224KB |
| M4 | Ping+HTTP集成测试 | ✅ 联调就绪 | qtest 18/18, pytest 7/7 字符串匹配, 驱动 semihosting 增强 |
| M5 | CI/CD | ✅ 完成 | ci.yml (5 jobs) |
| M6 | 板级抽象层 | ✅ 完成 | bal.h, bal.c, evk_config.h |

## 阻塞项（按优先级）
| 优先级 | 阻塞项 | 负责人 |
|:---:|------|:---:|
| **P1** | QEMU+固件联调 — 运行 qtest + pytest 端到端 | 用户 |
| **P2** | lwipopts.h LWIP_NO_CTYPE_H 重定义 warning | FW Dev |

## 已完成工作
- ✅ PRD v1.0 已确认
- ✅ M0 架构设计 (architecture.md + interfaces.md + 7 ADR)
- ✅ QEMU ENET1 设备模型 — 寄存器读写/MDIO/DMA/BD环/TX路径/RX路径/中断/W1C/ECR写保护
- ✅ DP83822 PHY 模型 — 16个标准+扩展寄存器, MDIO R/W, 链路状态始终Up
- ✅ SoC 容器 + Machine 模型
- ✅ FreeRTOS 10.6.2 源码导入并编译通过 (8核心文件)
- ✅ lwIP 2.2.1 完整协议栈编译通过 (TCP/UDP/DHCP/Raw + netconn API)
- ✅ mbedTLS 3.6.2 headers 部署
- ✅ firmware.elf **224,556 bytes** 编译产物
- ✅ ENET 驱动 .c (MDIO/DMA/BD环/TX/RX/中断, OCRAM布局, 全部 `#ifdef LWIP_HDR_NETIF_H` 已清理)
- ✅ lwipopts.h TCP+UDP+DHCP+DNS(暂时关闭)+ACD 已开启
- ✅ sys_arch.c FreeRTOS 完整移植 (信号量/互斥锁/邮箱/线程/临界区, 全部 lwIP sys.h 签名匹配)
- ✅ app/httpd_task.c HTTP Server demo (netconn API, 80端口)
- ✅ main.c FreeRTOS + lwIP + ENET 初始化流水线
- ✅ build.py 36源文件编译 + link.ld 链接
- ✅ FreeRTOSConfig.h 修复 (configTICK_TYPE_WIDTH_IN_BITS)
- ✅ startup.c 新增 snprintf/atoi/memmove
- ✅ 所有 Meson 构建文件 + Kconfig 指令
- ✅ CI/CD 流水线
- ✅ UART/GPIO/SysTick demo + semihosting
- ✅ BAL 抽象层
- ✅ qtest ENET 单元测试 + pytest 集成测试框架
- ✅ build.py + Makefile 双构建系统
- ✅ 7 个 Agent 记忆文件

## 下一步交接（2026-05-11 更新于 Test Eng 验证完成）
- **用户**: 编译 QEMU `(cd qemu/build && ../configure --target-list=arm-softmmu --enable-debug && ninja)` → 运行 `meson test --suite imxrt1180 --verbose`
- **用户**: 编译固件 `(cd firmware && make)` → 运行 `pytest tests/integration/test_enet.py -v`
- **预期**: L1 qtest 18/18 PASS, L2 pytest 9/9 PASS

## 关键项目约定
- ENET1 基地址: 0x4042_4000, IRQ 114 (标记 NEARLY CORRECT)
- PHY: TI DP83822, MDIO Clause 22
- 协议栈: lwIP 2.x netconn API + mbedTLS + FreeRTOS
- QEMU 代码风格: 4空格/90列宽/checkpatch
- 固件代码风格: Linux kernel Tab缩进
- 构建: firmware=build.py 或 Makefile, QEMU=Meson
- 文档语言: 中文

---

## Agent 状态看板 (由 Coordinator 维护)

| Agent | Memory | 当前阶段 | 最后活跃 | 状态 |
|-------|--------|------|------|:---:|
| PM | pm.md | 需求定义 | 2026-05-10 | 🟢 正常 |
| Architect | architect.md | M0 ✅ | 2026-05-10 | 🟢 正常 |
| QEMU Dev | qemu-dev.md | M1 ✅ | 2026-05-10 | 🟢 正常 |
| FW Dev | fw-dev.md | M3 ✅ | 2026-05-11 | 🟢 正常 |
| Test Eng | test-eng.md | M4 ✅ 联调就绪 | 2026-05-11 | 🟢 正常 |
| DevOps | devops.md | M5 ✅ | 2026-05-10 | 🟢 正常 |
| Coordinator | coordinator.md | 初始化 | 2026-05-11 | 🟢 正常 |

## Coordinator 巡查记录
| 日期 | 时间 | 同步状态 | 漂移项 | 陈旧阻塞项 | 处理 |
|------|------|------|:---:|:---:|------|
| 2026-05-11 | 初始化 | 首次部署 | — | — | Agent 创建
