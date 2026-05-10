# Makefile — i.MX RT1180 QEMU 固件构建系统
#
# 用法:
#   make                  # 编译 firmware.elf
#   make clean            # 清理构建产物
#   make run-qemu         # 编译并在 QEMU 中运行
#   make debug-qemu       # 编译并以 GDB stub 模式运行
#
# 工具链: ARM GNU Toolchain (arm-none-eabi-gcc)
# 目标:   Cortex-M7, Thumb mode, soft-float ABI

# ---- 工具链 ----
CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

# ---- 目录 ----
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/firmware.elf

# ---- 源文件 ----
# 核心启动
SRCS := startup.c main.c

# Demo 模块
SRCS += uart_demo.c systick_demo.c gpio_demo.c

# BAL 板级抽象层
SRCS += bal/bal.c

# ENET 驱动 (占位, Phase 2 启用)
# SRCS += drivers/imxrt_enet.c

# FreeRTOS kernel (Phase 2: 从官方源码导入后取消注释)
# FREERTOS_SRC := FreeRTOS/Source
# SRCS += $(FREERTOS_SRC)/tasks.c
# SRCS += $(FREERTOS_SRC)/queue.c
# SRCS += $(FREERTOS_SRC)/list.c
# SRCS += $(FREERTOS_SRC)/timers.c
# SRCS += $(FREERTOS_SRC)/portable/MemMang/heap_4.c
# SRCS += $(FREERTOS_SRC)/portable/GCC/ARM_CM7/r0p1/port.c

# lwIP (Phase 2: 从官方源码导入后取消注释)
# LWIP_SRC := lwip/src
# SRCS += $(LWIP_SRC)/core/init.c
# SRCS += $(LWIP_SRC)/core/tcp.c
# SRCS += $(LWIP_SRC)/core/tcp_in.c
# SRCS += $(LWIP_SRC)/core/tcp_out.c
# SRCS += $(LWIP_SRC)/core/udp.c
# SRCS += $(LWIP_SRC)/core/ipv4/ip4.c
# SRCS += $(LWIP_SRC)/core/ipv4/ip4_addr.c
# SRCS += $(LWIP_SRC)/core/ipv4/ip4_frag.c
# SRCS += $(LWIP_SRC)/core/ipv4/icmp.c
# SRCS += $(LWIP_SRC)/core/ipv4/dhcp.c
# SRCS += $(LWIP_SRC)/core/ipv4/autoip.c
# SRCS += $(LWIP_SRC)/core/pbuf.c
# SRCS += $(LWIP_SRC)/core/mem.c
# SRCS += $(LWIP_SRC)/core/memp.c
# SRCS += $(LWIP_SRC)/core/netif.c
# SRCS += $(LWIP_SRC)/core/stats.c
# SRCS += $(LWIP_SRC)/core/sys.c
# SRCS += $(LWIP_SRC)/core/timeouts.c
# SRCS += $(LWIP_SRC)/api/api_lib.c
# SRCS += $(LWIP_SRC)/api/api_msg.c
# SRCS += $(LWIP_SRC)/api/err.c
# SRCS += $(LWIP_SRC)/api/netbuf.c
# SRCS += $(LWIP_SRC)/api/netdb.c
# SRCS += $(LWIP_SRC)/api/netifapi.c
# SRCS += $(LWIP_SRC)/api/sockets.c
# SRCS += $(LWIP_SRC)/api/tcpip.c
# SRCS += $(LWIP_SRC)/netif/ethernet.c

# mbedTLS (Phase 2: 从官方源码导入后取消注释)

OBJS := $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

# ---- 编译标志 ----
# -mcpu=cortex-m7:     Cortex-M7 目标
# -mthumb:             Thumb-2 指令集
# -mfloat-abi=soft:    软浮点 ABI (QEMU 支持, 无需 FPU 硬件)
# -ffreestanding:      独立环境 (无标准库)
# -nostdlib:           不链接标准库
# -O0:                 无优化 (调试友好)
# -g3:                 最大调试信息
CFLAGS  := -mcpu=cortex-m7 -mthumb -mfloat-abi=soft
CFLAGS  += -ffreestanding -nostdlib
CFLAGS  += -O0 -g3 -Wall -Wextra -Werror
CFLAGS  += -std=c11
CFLAGS  += -I.
CFLAGS  += -Ibal
CFLAGS  += -Ibal/config
CFLAGS  += -Idrivers
CFLAGS  += -IFreeRTOS/include
CFLAGS  += -IFreeRTOS/portable
CFLAGS  += -Ilwip/include
CFLAGS  += -Imbedtls/include

# ---- 链接标志 ----
LDFLAGS := -T link.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/firmware.map

# ---- QEMU 参数 ----
QEMU         ?= qemu-system-arm
QEMU_MACHINE := imxrt1180-evk
QEMU_FLAGS   := -M $(QEMU_MACHINE) -kernel $(TARGET)
QEMU_FLAGS   += -semihosting -nographic -serial stdio

# ---- 默认目标 ----
.PHONY: all
all: $(TARGET)

# ---- 链接 ----
$(TARGET): $(OBJS) link.ld
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

# ---- 编译 C 文件 ----
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- 创建构建目录 ----
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# ---- 清理 ----
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# ---- 在 QEMU 中运行 ----
.PHONY: run-qemu
run-qemu: $(TARGET)
	$(QEMU) $(QEMU_FLAGS)

# ---- QEMU GDB 调试模式 ----
.PHONY: debug-qemu
debug-qemu: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -S -gdb tcp::1234

# ---- 反汇编 ----
.PHONY: disasm
disasm: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > $(BUILD_DIR)/firmware.disasm

# ---- 帮助 ----
.PHONY: help
help:
	@echo "i.MX RT1180 QEMU Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all         构建 firmware.elf (默认)"
	@echo "  clean       清理构建产物"
	@echo "  run-qemu    在 QEMU 中运行 (需 QEMU imxrt1180-evk 机器)"
	@echo "  debug-qemu  以 GDB stub 模式运行 (端口 1234)"
	@echo "  disasm      生成反汇编文件"
	@echo "  help        显示本帮助"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS      交叉编译器前缀 (默认 arm-none-eabi-)"
	@echo "  QEMU       QEMU 可执行文件 (默认 qemu-system-arm)"
