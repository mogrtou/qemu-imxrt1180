# ADR-004: TAP 作为主网络后端，SLIRP 作为调试后端

| 属性 | 值 |
|------|-----|
| **状态** | ✅ 已采纳 |
| **日期** | 2026-05-10 |
| **决策者** | Architect Agent |

## Context

QEMU 支持多种网络后端将模拟网卡接入宿主机网络：
- **TAP**: 创建虚拟网卡 `/dev/tapX`，桥接到宿主机网络
- **SLIRP (user)**: 用户态 NAT，无需 root 权限
- **Socket**: 通过 TCP/UDP Socket 连接

## Decision

**Phase 1 同时支持 TAP 和 SLIRP，TAP 为正式验证后端，SLIRP 为开发调试后端。**

在 QEMU 设备代码中使用标准 `net/net.h` API (`qemu_send_packet` / `receive` 回调)，对具体后端透明。

## Consequences

### 正面
- ✅ TAP 模式下 RT1180 与宿主机在同一子网，便于真实网络集成
- ✅ SLIRP 无需 root 权限，开发者可随时在本地测试
- ✅ QEMU 标准 net API 天然支持后端切换

### 负面
- ⚠️ TAP 模式在不同宿主 OS 上配置方式有差异（Linux: tap，Windows: 需额外驱动）
- ⚠️ SLIRP 模式下 ICMP 支持有限（依赖 QEMU 版本）

### 使用指南
```bash
# SLIRP (开发/快速测试)
-netdev user,id=net0,hostfwd=tcp::8080-:80

# TAP (集成测试)
-netdev tap,id=net0,ifname=tap0,script=no
```

### 安全注意
- SLIRP 的 hostfwd 仅在 localhost 暴露端口，不对外公开
- CI/CD 环境强制使用 SLIRP（无法创建 TAP）
