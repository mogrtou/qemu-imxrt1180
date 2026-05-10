#!/bin/bash
# 独立测试运行脚本 — 不依赖 QEMU 源码树
#
# 用法:
#   bash run_tests.sh                    # 运行全部测试
#   bash run_tests.sh test_enet          # 仅运行 ENET 测试
#   bash run_tests.sh --smoke            # 冒烟测试 (确认环境正常)
#   QEMU_BINARY=./my-qemu bash run_tests.sh  # 指定 QEMU 二进制
#
# 前提:
#   - python3, pytest 已安装
#   - qemu-system-arm 在 PATH 中, 或通过 QEMU_BINARY 指定

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export QEMU_BINARY="${QEMU_BINARY:-qemu-system-arm}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

# ---------- 检查依赖 ----------
check_prereqs() {
    info "Checking prerequisites..."

    # 检查 Python
    python3 --version > /dev/null 2>&1 || {
        error "python3 not found. Install Python 3.8+."
        exit 1
    }

    # 检查 pytest
    python3 -c "import pytest" 2>/dev/null || {
        warn "pytest not installed. Run: pip install pytest"
        exit 1
    }

    # 检查 QEMU 二进制
    if command -v "$QEMU_BINARY" > /dev/null 2>&1; then
        info "QEMU binary: $(command -v "$QEMU_BINARY")"
    else
        error "QEMU binary not found: $QEMU_BINARY"
        error "  Set QEMU_BINARY environment variable, e.g.:"
        error "  export QEMU_BINARY=/path/to/qemu/build/qemu-system-arm"
        exit 1
    fi

    # 检查 QEMU 是否支持我们的机器
    if ! "$QEMU_BINARY" -M ? 2>&1 | grep -q imxrt1180-evk; then
        warn "QEMU may not support 'imxrt1180-evk' machine."
        warn "  Expected machine: imxrt1180-evk"
        warn "  Run '$QEMU_BINARY -M ?' to list supported machines."
    fi

    info "All prerequisites OK."
}

# ---------- 冒烟测试 ----------
run_smoke() {
    info "Running smoke test (QEMU start/stop)..."

    python3 -c "
from qtest_client import QTestClient
import os, sys

qemu = os.environ.get('QEMU_BINARY', 'qemu-system-arm')
print(f'  Starting QEMU: {qemu} -M imxrt1180-evk -qtest stdio')
c = QTestClient(qemu, 'imxrt1180-evk')
c.start()
print(f'  QEMU PID: {c._proc.pid}')
assert c.is_running(), 'QEMU died immediately!'
val = c.readl(0x40424000)  # ENET ECR
print(f'  ENET ECR reset value: 0x{val:08X}')
c.stop()
print(f'  QEMU stopped.')
print('  Smoke test PASSED.')
"
    info "Smoke test OK."
}

# ---------- 解析参数 ----------
ARG="${1:-all}"

# ---------- 主流程 ----------
main() {
    echo ""
    echo "=============================="
    echo "  i.MX RT1180 独立测试套件"
    echo "=============================="
    echo ""

    check_prereqs

    case "$ARG" in
        --smoke|smoke)
            run_smoke
            ;;
        test_enet|enet)
            info "Running ENET register tests..."
            cd "$SCRIPT_DIR"
            python3 -m pytest test_enet_standalone.py -v --tb=short
            ;;
        test_soc|soc)
            info "Running SoC memory map tests..."
            cd "$SCRIPT_DIR"
            python3 -m pytest test_enet_standalone.py -v --tb=short \
                -k "TestSoCMemoryMap"
            ;;
        all|"")
            run_smoke
            info "Running all standalone tests..."
            cd "$SCRIPT_DIR"
            python3 -m pytest test_enet_standalone.py -v --tb=short
            ;;
        *)
            error "Unknown argument: $ARG"
            echo "Usage: $0 [all|test_enet|test_soc|--smoke]"
            exit 1
            ;;
    esac

    echo ""
    info "All tests completed."
}

main
