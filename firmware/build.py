#!/usr/bin/env python3
"""build.py — i.MX RT1180 QEMU 固件跨平台构建脚本 (替代 Makefile)

用法:
  python build.py              # 编译 firmware.elf (相当于 make)
  python build.py clean        # 清理构建产物
  python build.py run-qemu     # 编译并启动 QEMU
  python build.py debug-qemu   # 编译并启动 QEMU + GDB stub

环境:
  - 需要 Python 3.7+
  - 需要 arm-none-eabi-gcc 在 PATH 中
  - 需要 qemu-system-arm 在 PATH 中 (仅 run-qemu / debug-qemu)
"""

import os
import sys
import subprocess
import shutil
import glob

# ============================================================================
# 配制
# ============================================================================

CROSS_PREFIX = os.environ.get("CROSS", "arm-none-eabi-")
CC = f"{CROSS_PREFIX}gcc"
OBJCOPY = f"{CROSS_PREFIX}objcopy"
OBJDUMP = f"{CROSS_PREFIX}objdump"
SIZE = f"{CROSS_PREFIX}size"

QEMU = os.environ.get("QEMU", "qemu-system-arm")
QEMU_MACHINE = "imxrt1180-evk"

BUILD_DIR = "build"
TARGET_ELF = os.path.join(BUILD_DIR, "firmware.elf")

# 源文件列表
SOURCES = [
    # 核心启动
    "startup.c",
    "main.c",

    # BAL 板级抽象层
    "bal/bal.c",

    # ENET 驱动
    "drivers/imxrt_enet.c",

    # 应用层
    "app/httpd_task.c",

    # FreeRTOS Kernel (10.6.2)
    "FreeRTOS/Source/tasks.c",
    "FreeRTOS/Source/queue.c",
    "FreeRTOS/Source/list.c",
    "FreeRTOS/Source/timers.c",
    "FreeRTOS/Source/event_groups.c",
    "FreeRTOS/Source/stream_buffer.c",
    "FreeRTOS/Source/portable/MemMang/heap_4.c",
    "FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c",

    # lwIP 2.2.1 — core
    "lwip/src/core/def.c",
    "lwip/src/core/mem.c",
    "lwip/src/core/memp.c",
    "lwip/src/core/pbuf.c",
    "lwip/src/core/init.c",
    "lwip/src/core/netif.c",
    "lwip/src/core/ip.c",
    "lwip/src/core/inet_chksum.c",
    "lwip/src/core/timeouts.c",
    "lwip/src/core/stats.c",
    "lwip/src/core/sys.c",

    # lwIP — IPv4
    "lwip/src/core/ipv4/ip4.c",
    "lwip/src/core/ipv4/ip4_addr.c",
    "lwip/src/core/ipv4/ip4_frag.c",
    "lwip/src/core/ipv4/icmp.c",
    "lwip/src/core/ipv4/etharp.c",
    "lwip/src/core/ipv4/dhcp.c",
    "lwip/src/core/ipv4/igmp.c",
    "lwip/src/core/ipv4/acd.c",

    # lwIP — TCP + UDP + Raw
    "lwip/src/core/tcp.c",
    "lwip/src/core/tcp_in.c",
    "lwip/src/core/tcp_out.c",
    "lwip/src/core/udp.c",
    "lwip/src/core/raw.c",

    # lwIP — DNS (暂时关闭, Phase 2 启用)
    # "lwip/src/core/dns.c",

    # lwIP — netconn API
    "lwip/src/api/api_lib.c",
    "lwip/src/api/api_msg.c",
    "lwip/src/api/err.c",
    "lwip/src/api/netbuf.c",
    "lwip/src/api/netdb.c",
    "lwip/src/api/netifapi.c",
    "lwip/src/api/tcpip.c",

    # lwIP — netif (ethernet)
    "lwip/src/netif/ethernet.c",

    # lwIP — arch 适配
    "lwip/src/arch/sys_arch.c",
]

# 查找 ARM GCC 系统头文件路径 (CI 上 -ffreestanding 可能阻止搜索)
def _find_arm_sysroot():
    try:
        # Method 1: ask GCC for its sysroot
        result = subprocess.run([CC, "-print-sysroot"], capture_output=True, text=True, check=True)
        sysroot = result.stdout.strip()
        if sysroot and os.path.isdir(sysroot):
            inc = os.path.join(sysroot, "include")
            if os.path.isdir(inc):
                return ["-isystem", inc]
    except Exception:
        pass
    # Method 2: common paths on Ubuntu/Debian
    for path in ["/usr/lib/arm-none-eabi/include", "/usr/arm-none-eabi/include"]:
        if os.path.isdir(path):
            return ["-isystem", path]
    return []

# 编译标志
CFLAGS = [
    "-mcpu=cortex-m7", "-mthumb", "-mfloat-abi=softfp", "-mfpu=fpv5-d16",
    "-ffreestanding",
    "-O0", "-g3", "-Wall", "-Wextra",
    "-Wno-unused-function", "-Wno-unused-parameter",
    "-Wno-missing-field-initializers",
    "-std=c11",
    "-I.", "-Ibal", "-Ibal/config", "-Idrivers",
    "-IFreeRTOS/include",
    "-IFreeRTOS/Source/include",
    "-IFreeRTOS/Source/portable/GCC/ARM_CM7/r0p1",
    "-Ilwip/include",
    "-Ilwip/include/arch",
    "-Ilwip/src/include",
    "-Ilwip/src/include/ipv4",
    "-Ilwip/src/arch",
    "-Imbedtls/include",
    "-Imbedtls/include/mbedtls",
] + _find_arm_sysroot()

# 链接标志
LDFLAGS = [
    "-T", "link.ld",
    "-nostdlib",
    f"-Wl,-Map={os.path.join(BUILD_DIR, 'firmware.map')}",
]

# QEMU 标志
QEMU_FLAGS = [
    "-M", QEMU_MACHINE,
    "-kernel", TARGET_ELF,
    "-semihosting",
    "-nographic",
    "-serial", "stdio",
]


def run_cmd(cmd, desc=None):
    """运行命令, 失败时退出"""
    label = desc or " ".join(cmd)
    print(f"  {label}")
    result = subprocess.run(cmd, capture_output=False)
    if result.returncode != 0:
        print(f"ERROR: 命令失败 (exit {result.returncode}): {' '.join(cmd)}")
        sys.exit(result.returncode)
    return result


def check_tool(name):
    """检查工具是否可用"""
    if shutil.which(name) is None:
        print(f"ERROR: 找不到 '{name}', 请先安装 ARM GNU Toolchain")
        print("  下载: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads")
        sys.exit(1)


def compile_one(src):
    """编译单个 .c 文件 → .o"""
    obj = os.path.join(BUILD_DIR, src.replace("/", "_").replace("\\", "_") + ".o")
    # 仅当源文件更新时才重编译
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src):
        return obj
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    run_cmd([CC] + CFLAGS + ["-c", src, "-o", obj], desc=f"CC {src}")
    return obj


def build():
    """编译所有源文件并链接"""
    check_tool(CC)
    os.makedirs(BUILD_DIR, exist_ok=True)

    print("=" * 60)
    print("  固件构建")
    print("=" * 60)

    # 编译每个源文件
    objs = []
    for src in SOURCES:
        if not os.path.exists(src):
            print(f"WARNING: 源文件 '{src}' 不存在, 跳过")
            continue
        objs.append(compile_one(src))

    if not objs:
        print("ERROR: 没有可编译的源文件")
        sys.exit(1)

    # 链接 — 直接在 build/ 目录下执行, 路径用对象名
    saved_cwd = os.getcwd()
    try:
        os.chdir(BUILD_DIR)
        obj_names = [os.path.basename(o) for o in objs]
        run_cmd([CC, "-mcpu=cortex-m7", "-mthumb", "-mfloat-abi=softfp",
                 "-mfpu=fpv5-d16", "-nostdlib",
                 "-T", ".." + os.sep + "link.ld",
                 "-o", "firmware.elf"] + obj_names,
                desc="LINK")
        run_cmd([SIZE, "firmware.elf"], desc="SIZE")
    finally:
        os.chdir(saved_cwd)


def clean():
    """清理构建产物"""
    if os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
        print(f"已删除: {BUILD_DIR}/")
    else:
        print("已清理 (无构建产物)")


def run_qemu():
    """在 QEMU 中运行"""
    build()
    check_tool(QEMU)
    print()
    print("=" * 60)
    print("  启动 (Ctrl-A X 退出)")
    print("=" * 60)
    run_cmd([QEMU] + QEMU_FLAGS)


def debug_qemu():
    """在 QEMU GDB stub 模式下运行"""
    build()
    check_tool(QEMU)
    print()
    print("=" * 60)
    print("  启动 (GDB stub 端口 1234)")
    print("  另开终端: arm-none-eabi-gdb build/firmware.elf")
    print("  (gdb) target remote :1234")
    print("=" * 60)
    run_cmd([QEMU] + QEMU_FLAGS + ["-S", "-gdb", "tcp::1234"])


def disasm():
    """生成反汇编"""
    build()
    disasm_file = os.path.join(BUILD_DIR, "firmware.disasm")
    run_cmd([OBJDUMP, "-d", TARGET_ELF], desc="OBJDUMP")
    print(f"反汇编: {disasm_file}")


def main():
    if len(sys.argv) < 2:
        build()
    elif sys.argv[1] == "clean":
        clean()
    elif sys.argv[1] == "run-qemu":
        run_qemu()
    elif sys.argv[1] == "debug-qemu":
        debug_qemu()
    elif sys.argv[1] == "disasm":
        disasm()
    elif sys.argv[1] in ("help", "-h", "--help"):
        print(__doc__)
    else:
        print(f"未知命令: {sys.argv[1]}")
        print("可用: build clean run-qemu debug-qemu disasm help")
        sys.exit(1)


if __name__ == "__main__":
    main()
