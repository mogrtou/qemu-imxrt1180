# QEMU i.MX RT1180 项目全局规范

## 项目概述
基于 QEMU upstream 构建 NXP i.MX RT1180 (Cortex-M7 + Cortex-M33) MCU 模拟器及完整测试环境。

## 代码风格

### C 代码 (QEMU 设备模型)
- 遵循 QEMU 上游编码规范 (CODING_STYLE.rst)
- 4 空格缩进，不使用 Tab
- 左花括号：控制流同行，函数定义新行
- 行宽上限 90 字符
- 变量声明在块顶部
- 通过 `scripts/checkpatch.pl` 检查

### C 代码 (固件)
- 遵循 Linux kernel 编码风格
- Tab 缩进 (8 字符宽)
- `-ffreestanding -nostdlib` 环境
- 寄存器访问使用 `volatile uint32_t *` 指针

### Python 代码
- 遵循 PEP 8
- pytest 框架
- 类型注解推荐

### 文档
- Markdown 格式
- 中文编写
- 文件放在 `docs/` 目录

## Git 规范

### Commit Message
```
<subsystem>: <简短描述>

<详细说明 (可选)>

Refs: #<issue>
Signed-off-by: <姓名> <邮箱>
```

### 分支策略
- `main`: 稳定分支
- `feature/<name>`: 功能分支
- PR 合并前需通过 CI

## 目录结构
```
f:\qemu-imxrt1180\
├── .github/                # Agent 定义与 CI 配置
├── docs/                   # 设计文档与 PRD
├── firmware/               # 测试固件源码
├── tests/
│   ├── qtest/              # QEMU qtest 单元测试
│   └── integration/        # Python 集成测试
├── Dockerfile              # 构建环境
└── docker-compose.yml      # 容器编排
```

## 技术决策
- **QEMU 基础**: 上游 master 分支
- **目标架构**: arm-softmmu
- **固件工具链**: arm-none-eabi-gcc (ARM GNU Toolchain)
- **测试框架**: libqtest + pytest
- **CI/CD**: GitHub Actions

## 通信协议
- Agent 间通过 `docs/` 目录下的文档交接
- PM → Architect: `docs/prd.md`
- Architect → 开发团队: `docs/architecture.md` + `docs/interfaces.md`
- 有问题时在文档中标注 "NEEDS CLARIFICATION"
