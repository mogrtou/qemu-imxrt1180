# Makefile — i.MX RT1180 QEMU 固件构建系统
#
# 推荐使用 build.py (跨平台, Python 3.7+)。
# Makefile 作为备选 (需 GNU Make + arm-none-eabi-gcc)。
#
# 用法:
#   make                  # 编译 firmware.elf
#   make clean            # 清理构建产物
#   make run-qemu         # 编译并在 QEMU 中运行
#   make debug-qemu       # 编译并以 GDB stub 模式运行
#
# 工具链: ARM GNU Toolchain (arm-none-eabi-gcc)
# 目标:   Cortex-M7, Thumb mode, softfp ABI

# ---- 工具链 ----
CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

# ---- 目录 ----
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/firmware.elf

# ---- 源文件 (与 build.py 保持一致) ----
SRCS := startup.c main.c
SRCS += bal/bal.c
SRCS += drivers/imxrt_enet.c
SRCS += app/httpd_task.c

# FreeRTOS Kernel (10.6.2)
SRCS += FreeRTOS/Source/tasks.c
SRCS += FreeRTOS/Source/queue.c
SRCS += FreeRTOS/Source/list.c
SRCS += FreeRTOS/Source/timers.c
SRCS += FreeRTOS/Source/event_groups.c
SRCS += FreeRTOS/Source/stream_buffer.c
SRCS += FreeRTOS/Source/portable/MemMang/heap_4.c
SRCS += FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c

# lwIP 2.2.1 — core
SRCS += lwip/src/core/def.c
SRCS += lwip/src/core/mem.c
SRCS += lwip/src/core/memp.c
SRCS += lwip/src/core/pbuf.c
SRCS += lwip/src/core/init.c
SRCS += lwip/src/core/netif.c
SRCS += lwip/src/core/ip.c
SRCS += lwip/src/core/inet_chksum.c
SRCS += lwip/src/core/timeouts.c
SRCS += lwip/src/core/stats.c
SRCS += lwip/src/core/sys.c

# lwIP — IPv4
SRCS += lwip/src/core/ipv4/ip4.c
SRCS += lwip/src/core/ipv4/ip4_addr.c
SRCS += lwip/src/core/ipv4/ip4_frag.c
SRCS += lwip/src/core/ipv4/icmp.c
SRCS += lwip/src/core/ipv4/etharp.c
SRCS += lwip/src/core/ipv4/dhcp.c
SRCS += lwip/src/core/ipv4/igmp.c
SRCS += lwip/src/core/ipv4/acd.c

# lwIP — TCP + UDP + Raw
SRCS += lwip/src/core/tcp.c
SRCS += lwip/src/core/tcp_in.c
SRCS += lwip/src/core/tcp_out.c
SRCS += lwip/src/core/udp.c
SRCS += lwip/src/core/raw.c

# lwIP — netconn API
SRCS += lwip/src/api/api_lib.c
SRCS += lwip/src/api/api_msg.c
SRCS += lwip/src/api/err.c
SRCS += lwip/src/api/netbuf.c
SRCS += lwip/src/api/netdb.c
SRCS += lwip/src/api/netifapi.c
SRCS += lwip/src/api/tcpip.c

# lwIP — netif (ethernet)
SRCS += lwip/src/netif/ethernet.c

# lwIP — arch 适配
SRCS += lwip/src/arch/sys_arch.c

OBJS := $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS:.c=.o)))

# ---- 编译标志 (与 build.py 保持一致) ----
CFLAGS  := -mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv5-d16
CFLAGS  += -ffreestanding -nostdlib
CFLAGS  += -O0 -g3 -Wall -Wextra
CFLAGS  += -Wno-unused-function -Wno-unused-parameter
CFLAGS  += -Wno-missing-field-initializers
CFLAGS  += -std=c11
CFLAGS  += -I.
CFLAGS  += -Ibal
CFLAGS  += -Ibal/config
CFLAGS  += -Idrivers
CFLAGS  += -IFreeRTOS/include
CFLAGS  += -IFreeRTOS/Source/include
CFLAGS  += -IFreeRTOS/Source/portable/GCC/ARM_CM7/r0p1
CFLAGS  += -Ilwip/include
CFLAGS  += -Ilwip/include/arch
CFLAGS  += -Ilwip/src/include
CFLAGS  += -Ilwip/src/include/ipv4
CFLAGS  += -Ilwip/src/arch
CFLAGS  += -Imbedtls/include
CFLAGS  += -Imbedtls/include/mbedtls

# ---- 链接标志 ----
LDFLAGS := -mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv5-d16
LDFLAGS += -T link.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/firmware.map

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

# ---- 编译 C 文件 (flat obj dir, 避免路径冲突) ----
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: bal/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: drivers/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: app/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: FreeRTOS/Source/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: FreeRTOS/Source/portable/MemMang/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: lwip/src/core/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: lwip/src/core/ipv4/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: lwip/src/api/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: lwip/src/netif/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: lwip/src/arch/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

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
	@echo "推荐: python build.py (跨平台, 更简化的依赖)"
	@echo "备选: make (GNU Make + arm-none-eabi-gcc)"
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
