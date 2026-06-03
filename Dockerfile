# Dockerfile — QEMU i.MX RT1180 开发环境
#
# 构建:
#   docker build -t imxrt1180-dev .
#
# 用法:
#   docker compose run qemu-dev
#   docker compose run qemu-dev bash
#
# 包含: QEMU 构建 + ARM 固件构建 + Python 测试 一站式环境

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ---- QEMU 构建依赖 ----
RUN apt-get update && apt-get install -y --no-install-recommends \
    # 构建工具链
    build-essential \
    ninja-build \
    meson \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    python-is-python3 \
    # QEMU 必备依赖
    libglib2.0-dev \
    libpixman-1-dev \
    # QEMU 可选依赖 (增强网络/测试功能)
    libslirp-dev \
    libgcrypt20-dev \
    libgnutls28-dev \
    libcap-ng-dev \
    libattr1-dev \
    libsdl2-dev \
    # ARM 固件工具链
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    # 测试工具
    python3-pytest \
    python3-pexpect \
    lcov \
    gcovr \
    ccache \
    # 网络测试工具
    iputils-ping \
    curl \
    # 常用工具
    git \
    make \
    bash-completion \
    vim \
    && rm -rf /var/lib/apt/lists/*

# ---- 配置 ccache ----
ENV CCACHE_DIR=/workspace/.ccache
ENV PATH="/usr/lib/ccache:${PATH}"

# ---- 工作目录 ----
WORKDIR /workspace

# ---- 默认入口 ----
CMD ["bash"]
