/**
 * httpd_task.c — lwIP HTTP Server demo (M3)
 *
 * 使用 lwIP netconn API 实现简单的 HTTP/1.0 服务器。
 * - FreeRTOS 任务中运行
 * - 监听 80 端口
 * - 响应 GET / → 显示系统运行时间和网络状态
 * - 响应 404 → 标准 404 页面
 *
 * 依赖:
 *   FreeRTOS (vTaskDelay, xTaskGetTickCount)
 *   lwIP netconn API (netconn_new, netconn_bind, netconn_listen, ...)
 *   drivers/imxrt_enet.h (imxrt_enet_get_link_status)
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/tcp.h"

#include "bal/config/evk_config.h"

/* ==========================================================================
 * HTTP 响应模板
 * ========================================================================== */

static const char HTTP_200_HEADER[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection: close\r\n"
    "\r\n";

static const char HTTP_404_HEADER[] =
    "HTTP/1.0 404 Not Found\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection: close\r\n"
    "\r\n";

static const char HTML_FOOTER[] =
    "</body>\n</html>\n";

/* ==========================================================================
 * HTML 响应生成
 * ========================================================================== */

/**
 * httpd_send_200 — 发送首页 HTML
 */
static void httpd_send_200(struct netconn *conn)
{
    char buf[1024];
    int len;

    len = snprintf(buf, sizeof(buf),
        "<!DOCTYPE html>\n"
        "<html>\n<head>\n"
        "<title>i.MX RT1180 HTTP Server</title>\n"
        "<style>body{font-family:Arial;margin:40px;}"
        "h1{color:#005588;}.info{background:#f0f0f0;padding:10px;"
        "border-radius:5px;}</style>\n"
        "</head>\n<body>\n"
        "<h1>i.MX RT1180 HTTP Server</h1>\n"
        "<div class='info'>\n"
        "<p><b>Board:</b> %s</p>\n"
        "<p><b>Uptime:</b> %lu seconds</p>\n"
        "<p><b>Link:</b> UP (QEMU simulation)</p>\n"
        "<p><b>Stack:</b> lwIP 2.2.1 + FreeRTOS</p>\n"
        "<p><b>Demo:</b> QEMU Simulation</p>\n"
        "</div>\n"
        "<p>This page is served by an i.MX RT1180 MCU running inside QEMU.</p>\n"
        "%s",
        BOARD_NAME,
        xTaskGetTickCount() / configTICK_RATE_HZ,
        HTML_FOOTER
    );

    netconn_write(conn, HTTP_200_HEADER, sizeof(HTTP_200_HEADER) - 1, NETCONN_NOCOPY);
    if (len > 0 && len < (int)sizeof(buf)) {
        netconn_write(conn, buf, (size_t)len, NETCONN_COPY);
    }
}

/**
 * httpd_send_404 — 发送 Not Found
 */
static void httpd_send_404(struct netconn *conn)
{
    const char *body = "<!DOCTYPE html>\n<html>\n<head>\n"
        "<title>404 Not Found</title>\n"
        "</head>\n<body>\n<h1>404 Not Found</h1>\n"
        "<p>The requested resource was not found on this server.</p>\n"
        "</body>\n</html>\n";

    netconn_write(conn, HTTP_404_HEADER, sizeof(HTTP_404_HEADER) - 1, NETCONN_NOCOPY);
    netconn_write(conn, body, strlen(body), NETCONN_COPY);
}

/* ==========================================================================
 * HTTP 请求解析
 * ========================================================================== */

/**
 * httpd_parse_request — 简单 HTTP 请求行解析
 * 只支持 GET /path HTTP/1.0 或 HTTP/1.1
 *
 * @param req   HTTP 请求字符串
 * @param path  输出: URL 路径
 * @return      0=GET /, 1=其他路径, -1=非 GET 请求
 */
static int httpd_parse_request(const char *req, char *path, size_t path_sz)
{
    if (!req || !path) return -1;

    /* 检查方法: GET */
    if (strncmp(req, "GET ", 4) != 0) {
        return -1;
    }

    /* 提取路径直到空格 */
    const char *start = req + 5;  /* 跳过 "GET /" */
    const char *end = start;
    while (*end && *end != ' ' && *end != '\r' && *end != '\n') {
        end++;
    }

    size_t len = (size_t)(end - start);
    if (len >= path_sz) len = path_sz - 1;
    if (len > 0) {
        memcpy(path, start, len);
        path[len] = '\0';
    } else {
        /* 空路径视为 "/" */
        path[0] = '/';
        path[1] = '\0';
    }

    return (path[0] == '\0' || strncmp(path, "/", 1) == 0) ? 0 : 1;
}

/* ==========================================================================
 * HTTP 连接处理
 * ========================================================================== */

/**
 * httpd_handle_connection — 处理单个 HTTP 连接
 */
static void httpd_handle_connection(struct netconn *conn)
{
    struct netbuf *inbuf;
    char *req_data;
    uint16_t req_len;
    char path[64];
    err_t err;

    /* 接收 HTTP 请求 */
    err = netconn_recv(conn, &inbuf);
    if (err != ERR_OK) {
        goto cleanup;
    }

    /* 获取请求数据 */
    err = netbuf_data(inbuf, (void **)&req_data, &req_len);
    if (err != ERR_OK || !req_data || req_len == 0) {
        netbuf_delete(inbuf);
        goto cleanup;
    }

    /* 解析请求路径 */
    int result = httpd_parse_request(req_data, path, sizeof(path));

    /* 释放接收缓冲区 */
    netbuf_delete(inbuf);

    /* 路由到处理函数 */
    if (result == 0) {
        httpd_send_200(conn);
    } else {
        httpd_send_404(conn);
    }

cleanup:
    /* 关闭连接 */
    netconn_close(conn);
    netconn_delete(conn);
}

/* ==========================================================================
 * HTTP Server 主任务
 * ========================================================================== */

/**
 * httpd_task — HTTP Server FreeRTOS 任务入口
 *
 * 创建 TCP 监听 socket 在 80 端口, 循环 accept 连接。
 *
 * @param arg  lwIP netif 指针 (用于等待网络就绪)
 */
void httpd_task(void *arg)
{
    struct netconn *server_conn, *client_conn;
    err_t err;

    struct netif *netif = (struct netif *)arg;

    /* ── 1. 等待网络接口就绪 ── */
    while (!netif_is_up(netif)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* ── 2. 等待获取 IP 地址 ── */
    while (netif->ip_addr.addr == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* ── 3. 创建 TCP 服务器 ── */
    server_conn = netconn_new(NETCONN_TCP);
    if (!server_conn) {
        /* netconn_new 失败 — 无法恢复 */
        vTaskDelete(NULL);
        return;
    }

    err = netconn_bind(server_conn, IP_ADDR_ANY, 80);
    if (err != ERR_OK) {
        netconn_delete(server_conn);
        vTaskDelete(NULL);
        return;
    }

    err = netconn_listen(server_conn);
    if (err != ERR_OK) {
        netconn_delete(server_conn);
        vTaskDelete(NULL);
        return;
    }

    /* ── 4. 主循环: accept 并处理连接 ── */
    while (1) {
        err = netconn_accept(server_conn, &client_conn);
        if (err == ERR_OK && client_conn) {
            httpd_handle_connection(client_conn);
        }
        /* 短暂 yield, 避免饿死其他任务 */
        taskYIELD();
    }
}
