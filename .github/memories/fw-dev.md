# FW Dev Agent 工作状态

> 最后更新: 2026-05-11 06:00

## 当前阶段
阶段：M3 ✅ 完成 — ENET 驱动 + lwIP 完整协议栈 + HTTP Server demo
产物：firmware.elf 224,556 bytes, 36源文件编译链接通过
待办：QEMU+固件联调, ENET TX/RX 端到端验证

## 已完成
- ✅ startup.c — 向量表 + Reset_Handler + snprintf/atoi/memmove 标准函数
- ✅ main.c — FreeRTOS + lwIP + ENET 初始化流水线 + DHCP + HTTP Server 任务创建
- ✅ BAL 层 (bal.h + bal.c + evk_config.h)
- ✅ ENET 驱动 (drivers/imxrt_enet.h + .c) — MDIO/DMA/BD环/TX/RX/中断, OCRAM布局
- ✅ lwipopts.h — TCP+UDP+DHCP+Raw 开启, DNS 暂时关闭, ACD 支持
- ✅ sys_arch.c — FreeRTOS 完整移植 (互斥锁/邮箱/信号量/线程/临界区, lwIP sys.h 签名完全匹配)
- ✅ sys_arch.h — 新增 valid/invalid/trypost_fromisr 函数声明
- ✅ app/httpd_task.c — HTTP Server demo (netconn API, 80端口)
- ✅ FreeRTOSConfig.h — 修复 configTICK_TYPE_WIDTH_IN_BITS
- ✅ build.py — 36源文件编译, link.ld 链接
- ✅ firmware.elf 224,556 bytes 编译通过

## 本轮 (2026-05-11) 产出
**ENET 驱动核心**：
- MDIO 读写: enet_mdio_read/write (Clause 22 帧格式, 同步等待 MII 中断)
- BD 环管理: enet_init_bd_rings (TX/RX BD 环初始化, OCRAM 布局)
- MAC 地址: enet_set_mac_addr (PALR/PAUR 寄存器)
- 链路检测: imxrt_enet_get_link_status (PHY BMSR)
- 硬件初始化: imxrt_enet_init (解锁ECR→复位→MAC→BD→RCR/TCR→MSCR→中断→使能→RDAR→PHY探测→netif注册)
- TX 路径: imxrt_enet_output (pbuf→TX BD→TDAR)
- RX 路径: imxrt_enet_input (RX BD→pbuf→netif->input)
- 中断处理: ENET_IRQHandler (RXF/TXF/MII/EBERR/TXB/RXB)

**lwIP FreeRTOS 移植**：
- sys_mutex_*: xSemaphoreCreateRecursiveMutex
- sys_mbox_*: xQueueCreate (void* 消息)
- sys_sem_*: xSemaphoreCreateCounting
- sys_thread_new: xTaskCreate
- sys_now: xTaskGetTickCount → ms 转换
- sys_arch_protect/unprotect: taskENTER/EXIT_CRITICAL
- sys_init: 空实现 (FreeRTOS 已在之前初始化)

**修复的编译问题**：
1. imxrt_enet_output 签名: int→err_t, void*→struct pbuf*
2. ECR_MAGIC 地址: 0x0024 (不是 0x0020)
3. lwipopts.h: LWIP_DNS=0 (缺少 LWIP_RAND), LWIP_DHCP_DOES_ACD_CHECK=0, LWIP_NETIF_HOSTNAME=0
4. sys_arch.h: sys_mbox_trypost_fromisr 返回 err_t, 包含 lwip/err.h
5. sys_thread_new: 返回 sys_thread_t (不是 int+输出参数)
6. sys_arch_protect: 返回 sys_prot_t (不是 void+输出参数)
7. startup.c: 添加 snprintf/atoi (httpd_task 需要)
8. FreeRTOSConfig.h: 移除 configUSE_16_BIT_TICKS, 只用 configTICK_TYPE_WIDTH_IN_BITS
