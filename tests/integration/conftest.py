"""
Pytest fixtures for i.MX RT1180 integration tests.

Provides shared fixtures: QEMU binary path, firmware binary path,
QEMU process management, and semihosting output capture.
"""

import os
import subprocess
import time
import signal
from pathlib import Path

import pytest


# ------------------------------------------------------------------
#  Path fixtures
# ------------------------------------------------------------------

@pytest.fixture(scope="session")
def project_root():
    """Return the project root directory (f:/qemu-imxrt1180)."""
    return Path(os.environ.get(
        "IMXRT1180_PROJECT_ROOT",
        Path(__file__).resolve().parent.parent.parent
    ))


@pytest.fixture(scope="session")
def qemu_binary(project_root):
    """Return path to QEMU arm-softmmu binary."""
    qemu_path = os.environ.get(
        "QEMU_BINARY",
        project_root / "qemu" / "build" / "qemu-system-arm"
    )
    path = Path(qemu_path)
    if not path.exists():
        pytest.skip(f"QEMU binary not found: {path}")
    return str(path)


@pytest.fixture(scope="session")
def firmware_enet(project_root):
    """Return path to the ENET demo firmware ELF.

    Note: This firmware may not exist yet during Phase 1 bring-up.
    Tests that depend on it should skip gracefully.
    """
    fw_path = os.environ.get(
        "FW_ENET_ELF",
        project_root / "firmware" / "build" / "firmware.elf"
    )
    path = Path(fw_path)
    if not path.exists():
        pytest.skip(f"Firmware binary not found: {path}")
    return str(path)


# ------------------------------------------------------------------
#  QEMU process fixtures
# ------------------------------------------------------------------

class QemuInstance:
    """Wrapper around a QEMU subprocess for integration tests."""

    def __init__(self, proc, log_path=None):
        self.proc = proc
        self.log_path = log_path

    def wait_for_string(self, needle, timeout=30):
        """Poll QEMU stdout/semihosting log until needle appears.

        Returns True if found, False on timeout.
        """
        if not self.log_path:
            return False
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                return False
            try:
                content = Path(self.log_path).read_text()
                if needle in content:
                    return True
            except (OSError, UnicodeDecodeError):
                pass
            time.sleep(0.5)
        return False

    def terminate(self):
        """Gracefully terminate QEMU, force-kill if needed."""
        if self.proc.poll() is None:
            self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()


@pytest.fixture
def qemu_no_kernel(qemu_binary, tmp_path):
    """Launch QEMU without a kernel — for register-level smoke tests.

    QEMU should start, print semihosting banner or monitor prompt,
    then be terminated. This verifies the machine model loads.
    """
    log_file = tmp_path / "qemu_smoke.log"
    with open(log_file, "w") as log_f:
        proc = subprocess.Popen(
            [
                qemu_binary,
                "-M", "imxrt1180-evk",
                "-semihosting",
                "-nographic",
                "-serial", "stdio",
                "-monitor", "none",
            ],
            stdout=log_f,
            stderr=subprocess.STDOUT,
        )
    instance = QemuInstance(proc, log_path=log_file)
    yield instance
    instance.terminate()


@pytest.fixture
def qemu_with_kernel(qemu_binary, firmware_enet, tmp_path):
    """Launch QEMU with the ENET firmware ELF + user-mode net backend.

    Yields a QemuInstance; cleans up on teardown.
    """
    log_file = tmp_path / "qemu_enet.log"
    with open(log_file, "w") as log_f:
        proc = subprocess.Popen(
            [
                qemu_binary,
                "-M", "imxrt1180-evk",
                "-kernel", firmware_enet,
                "-netdev", "user,id=net0,hostfwd=tcp::18080-:80",
                "-device", "imxrt1180-enet,netdev=net0,mac-address=02:00:00:00:00:01",
                "-semihosting",
                "-nographic",
                "-serial", "stdio",
                "-monitor", "none",
            ],
            stdout=log_f,
            stderr=subprocess.STDOUT,
        )
    instance = QemuInstance(proc, log_path=log_file)
    yield instance
    instance.terminate()


# ------------------------------------------------------------------
#  Utility helpers
# ------------------------------------------------------------------

def ping_host(ip_addr, count=4, timeout=2):
    """Ping a host; returns (success: bool, output: str)."""
    cmd = ["ping", "-n" if os.name == "nt" else "-c",
           str(count), "-w", str(timeout * 1000),
           ip_addr]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True,
                                timeout=timeout * count + 5)
        return result.returncode == 0, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, "ping timed out"
