# ADR-005: lwIP netconn API 作为主编程接口

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

lwIP 提供三种编程 API：

| API | 特点 |
|-----|------|
| **raw API** | 零拷贝、回调驱动、无 OS 需求、代码复杂 |
| **netconn API** | OS 适配层、顺序编程模型、线程安全 |
| **socket API** | POSIX 兼容、可移植性最高、开销最大 |

项目使用 FreeRTOS，天然支持 netconn 和 socket API。

## Decision

**选择 netconn API 作为主要编程接口**，在需要时可不修改架构地切换到 socket API。

### 理由

- netconn API 专为 RTOS 设计，内存和 CPU 开销低
- 编程模型直观（类似 socket），团队学习成本低
- lwIP 内置 HTTPD / MQTT app 已基于 netconn 实现
- mbedTLS 通过 altcp 层与 netconn 无缝集成
- socket API 可通过 `LWIP_SOCKET=1` 开启，共存无冲突

## Consequences

### 正面
- ✅ 低开销，适合 MCU 资源约束
- ✅ 与 lwIP 内置应用层组件 (httpd, mqtt, altcp) 开箱兼容
- ✅ 阻塞式编程模型，开发效率高

### 负面
- ⚠️ 应用代码不直接可移植到 POSIX 系统（但 socket API 可共存）
- ⚠️ 每个连接需独立 netconn 结构体，有限内存开销

### 示例代码约定
```c
// HTTP Server 任务
void httpd_task(void *arg) {
    struct netconn *conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, 80);
    netconn_listen(conn);
    while (1) {
        struct netconn *newconn;
        netconn_accept(conn, &newconn);
        // 处理请求...
        netconn_delete(newconn);
    }
}
```
