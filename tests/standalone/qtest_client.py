#!/usr/bin/env python3
"""
Standalone QEMU qtest client — 不依赖 QEMU 源码树，仅与编译好的 qemu-system-arm
通过 qtest stdio 协议通信。

用法:
    from qtest_client import QTestClient
    q = QTestClient("qemu-system-arm", "-M imxrt1180-evk")
    q.start()
    val = q.readl(0x40424000)       # 读 ENET_ECR
    q.writel(0x40424000, 0x1234)    # 写 ENET_ECR
    q.stop()

qtest 协议参考: qemu/docs/devel/qtest.rst
"""

import subprocess
import re
import os
import time
from typing import Optional, Tuple


class QTestError(Exception):
    """qtest 协议错误"""
    pass


class QTestTimeout(QTestError):
    """qtest 超时"""
    pass


class QTestClient:
    """独立 qtest 客户端 — 通过 stdin/stdout 与 QEMU qtest 协议通信。

    不链接 libqtest，不依赖 QEMU 源码树。
    仅需要编译好的 qemu-system-arm 二进制。
    """

    def __init__(self, qemu_binary: str = "qemu-system-arm",
                 machine: str = "imxrt1180-evk",
                 extra_args: Optional[list] = None,
                 timeout: float = 10.0):
        """
        Args:
            qemu_binary: qemu-system-arm 路径 (可从环境变量 QEMU_BINARY 读取)
            machine:     -M 机器名
            extra_args:  额外 QEMU 参数 (如 -semihosting)
            timeout:     单次命令超时 (秒)
        """
        self.qemu_binary = qemu_binary
        self.machine = machine
        self.extra_args = extra_args or []
        self.timeout = timeout
        self._proc: Optional[subprocess.Popen] = None
        self._response_buffer = b""

    # ------ 生命周期 ------

    def start(self):
        """启动 QEMU qtest 进程"""
        cmd = [
            self.qemu_binary,
            "-M", self.machine,
            "-qtest", "stdio",
            "-accel", "qtest",
        ] + self.extra_args

        self._proc = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def stop(self):
        """停止 QEMU qtest 进程"""
        if self._proc:
            try:
                self._send_cmd("quit")
            except (QTestError, BrokenPipeError, OSError):
                pass
            try:
                self._proc.stdin.close()
            except OSError:
                pass
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
            self._proc = None

    def is_running(self) -> bool:
        """QEMU 进程是否仍在运行"""
        return self._proc is not None and self._proc.poll() is None

    # ------ qtest 协议核心 ------

    def _send_cmd(self, cmd: str) -> str:
        """发送一条 qtest 命令，读取并返回完整响应。

        qtest 协议: 一行命令 → 一行或多行响应，以空行结束。
        响应格式: "OK <data>" 或 "FAIL <reason>"
        """
        if not self._proc or self._proc.poll() is not None:
            raise QTestError("QEMU qtest process not running")

        # 发送命令 (qtest 协议要求换行分隔)
        full_cmd = (cmd + "\n").encode("ascii")
        try:
            self._proc.stdin.write(full_cmd)
            self._proc.stdin.flush()
        except (BrokenPipeError, OSError) as e:
            raise QTestError(f"Failed to send qtest command: {e}")

        # 读取响应行
        response_lines = []
        while True:
            line = self._read_line()
            if line == "":
                break  # 空行 = 响应结束
            response_lines.append(line)

        response = "\n".join(response_lines)
        if response.startswith("FAIL"):
            raise QTestError(f"qtest FAIL: {response}")
        return response

    def _read_line(self) -> str:
        """从 QEMU stdout 读取一行 (以 \\n 结尾)"""
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            if self._proc.poll() is not None:
                stderr_out = b""
                try:
                    stderr_out = self._proc.stderr.read()
                except OSError:
                    pass
                raise QTestError(
                    f"QEMU exited with code {self._proc.returncode}. "
                    f"stderr: {stderr_out[:500]!r}"
                )

            # 检查 buffer 中是否已有完整行
            if b"\n" in self._response_buffer:
                line, self._response_buffer = self._response_buffer.split(b"\n", 1)
                return line.decode("ascii", errors="replace").rstrip("\r")

            # 读取更多数据
            try:
                chunk = self._proc.stdout.read(1)
                if not chunk:
                    time.sleep(0.01)
                    continue
                self._response_buffer += chunk
            except OSError:
                time.sleep(0.01)

        raise QTestTimeout(f"Timeout ({self.timeout}s) waiting for qtest response")

    # ------ MMIO 读写 (最常用) ------

    def readb(self, addr: int) -> int:
        """读 8 位"""
        resp = self._send_cmd(f"readb 0x{addr:x}")
        return self._parse_ok_hex(resp)

    def readw(self, addr: int) -> int:
        """读 16 位"""
        resp = self._send_cmd(f"readw 0x{addr:x}")
        return self._parse_ok_hex(resp)

    def readl(self, addr: int) -> int:
        """读 32 位"""
        resp = self._send_cmd(f"readl 0x{addr:x}")
        return self._parse_ok_hex(resp)

    def readq(self, addr: int) -> int:
        """读 64 位"""
        resp = self._send_cmd(f"readq 0x{addr:x}")
        return self._parse_ok_hex(resp)

    def writeb(self, addr: int, value: int):
        """写 8 位"""
        self._send_cmd(f"writeb 0x{addr:x} 0x{value & 0xFF:x}")

    def writew(self, addr: int, value: int):
        """写 16 位"""
        self._send_cmd(f"writew 0x{addr:x} 0x{value & 0xFFFF:x}")

    def writel(self, addr: int, value: int):
        """写 32 位"""
        self._send_cmd(f"writel 0x{addr:x} 0x{value & 0xFFFFFFFF:x}")

    def writeq(self, addr: int, value: int):
        """写 64 位"""
        self._send_cmd(f"writeq 0x{addr:x} 0x{value & 0xFFFFFFFFFFFFFFFF:x}")

    # ------ IRQ 拦截 ------

    def irq_intercept_in(self, qom_path: str, irq_index: int = 0):
        """开始拦截指定设备的 IRQ。

        Args:
            qom_path: QOM 路径, 如 "/machine/soc/enet1"
            irq_index: IRQ 索引 (默认 0)
        """
        self._send_cmd(f"irq_intercept_in {qom_path} {irq_index}")

    def irq_intercept_out(self, qom_path: str, irq_index: int = 0):
        """停止拦截 IRQ"""
        self._send_cmd(f"irq_intercept_out {qom_path} {irq_index}")

    def irq_intercepted(self) -> bool:
        """查询是否有 IRQ 被拦截。返回 True = 有 IRQ 待处理。

        注意: 调用后 IRQ 状态被清除。
        """
        resp = self._send_cmd("irq_intercepted")
        # 响应: "IRQ NULL-IRQ" 或 "IRQ <qom-path> <index>"
        return not resp.startswith("IRQ NULL-IRQ")

    # ------ 时钟 ------

    def clock_step(self, nanoseconds: int):
        """推进 QEMU 虚拟时钟"""
        self._send_cmd(f"clock_step {nanoseconds}")

    # ------ 内存 ------

    def memread(self, addr: int, size: int) -> bytes:
        """读取物理内存"""
        resp = self._send_cmd(f"read 0x{addr:x} 0x{size:x}")
        return self._parse_ok_hex_bytes(resp, size)

    def memwrite(self, addr: int, data: bytes):
        """写入物理内存"""
        hex_data = " ".join(f"0x{b:02x}" for b in data)
        self._send_cmd(f"write 0x{addr:x} 0x{len(data):x} {hex_data}")

    # ------ 辅助 ------

    @staticmethod
    def _parse_ok_hex(response: str) -> int:
        """解析 "OK 0x<hex>" 响应"""
        m = re.match(r"^OK\s+(0x[0-9a-fA-F]+)", response)
        if not m:
            raise QTestError(f"Expected 'OK <hex>', got: {response}")
        return int(m.group(1), 16)

    @staticmethod
    def _parse_ok_hex_bytes(response: str, expected_size: int) -> bytes:
        """解析 "OK 0x<hex> 0x<hex>..." 响应为字节串"""
        m = re.match(r"^OK\s+(.*)", response)
        if not m:
            raise QTestError(f"Expected 'OK <data>', got: {response}")
        hex_values = re.findall(r"0x([0-9a-fA-F]+)", m.group(1))
        data = bytes(int(h, 16) for h in hex_values)
        if len(data) != expected_size:
            raise QTestError(
                f"Expected {expected_size} bytes, got {len(data)}"
            )
        return data
