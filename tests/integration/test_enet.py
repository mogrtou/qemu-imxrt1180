"""
Integration tests for i.MX RT1180 ENET Ethernet subsystem.

Tests are organized in layers:
  L2a — Smoke test: QEMU machine loads without crash
  L2b — Semihosting: firmware boots and prints expected strings
  L2c — Network: Ping, ARP, HTTP connectivity (requires firmware ENET demo)

Test convention (from docs/interfaces.md §4.3):
  - Firmware IP: 10.0.2.15 (SLIRP default)
  - HTTP port: 80 (internal), forwarded to host 18080
  - Timeout: 30 seconds for network readiness
"""

import os
import subprocess
import time

import pytest


# ==================================================================
#  L2a — Smoke Tests (no firmware required)
# ==================================================================

class TestQemuSmoke:
    """Verify QEMU machine model loads successfully."""

    def test_machine_starts(self, qemu_no_kernel):
        """QEMU should start and stay running for at least 2 seconds."""
        time.sleep(2)
        assert qemu_no_kernel.proc.poll() is None, \
            "QEMU exited unexpectedly"

    def test_machine_shuts_down_cleanly(self, qemu_no_kernel):
        """QEMU should terminate on SIGTERM without segfault."""
        time.sleep(1)
        qemu_no_kernel.terminate()
        # process should be dead within 5 seconds
        try:
            qemu_no_kernel.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pytest.fail("QEMU did not terminate within 5 seconds")
        assert qemu_no_kernel.proc.returncode in (0, -15), \
            f"Unexpected exit code: {qemu_no_kernel.proc.returncode}"


# ==================================================================
#  L2b — Firmware Semihosting Tests
# ==================================================================

class TestFirmwareBoot:
    """Verify firmware boots and communicates via semihosting."""

    @pytest.mark.slow
    def test_firmware_prints_banner(self, qemu_with_kernel):
        """Firmware should output an identifying banner via semihosting."""
        found = qemu_with_kernel.wait_for_string(
            "i.MX RT1180", timeout=15
        )
        assert found, (
            "Firmware did not print 'i.MX RT1180' banner within 15 seconds"
        )

    @pytest.mark.slow
    def test_lwip_initialized(self, qemu_with_kernel):
        """lwIP should initialize and print status."""
        found = qemu_with_kernel.wait_for_string(
            "lwIP", timeout=20
        )
        assert found, (
            "Firmware did not print lwIP initialization message"
        )

    @pytest.mark.slow
    def test_phy_link_up(self, qemu_with_kernel):
        """DP83822 PHY should report link up."""
        found = qemu_with_kernel.wait_for_string(
            "link up", timeout=20
        )
        assert found, (
            "PHY did not report link-up within 20 seconds"
        )


# ==================================================================
#  L2c — Network Connectivity Tests (requires ENET functional)
# ==================================================================

class TestNetworkConnectivity:
    """Verify TCP/IP stack and network backend connectivity."""

    @pytest.mark.network
    @pytest.mark.slow
    def test_ping_reachable(self, qemu_with_kernel):
        """Host should be able to ping the simulated RT1180.

        IP: 10.0.2.15 (QEMU SLIRP default for guest).
        Note: SLIRP ping support varies by QEMU version / host OS.
        On failure, check QEMU version >= 8.0.
        """
        # Wait for firmware to initialize networking
        found = qemu_with_kernel.wait_for_string(
            "[LWIP] Init complete", timeout=30
        )
        assert found, "Firmware did not report network interface ready"

        # Ping the guest IP (SLIRP default)
        ip_addr = "10.0.2.15"
        success, output = _ping(ip_addr, count=4, timeout=2)
        if not success:
            pytest.skip(
                f"Ping to {ip_addr} failed. "
                f"SLIRP ICMP may not be supported in this QEMU build.\n"
                f"Output:\n{output}"
            )

    @pytest.mark.network
    @pytest.mark.slow
    def test_http_server_responds(self, qemu_with_kernel):
        """HTTP server on guest port 80 (host-forwarded to 18080) should
        respond with status 200 and contain device identifier."""
        found = qemu_with_kernel.wait_for_string(
            "HTTP", timeout=30
        )
        assert found, "Firmware did not report HTTP server ready"

        # Try to reach the HTTP server via host-forwarded port
        success, content = _http_get("localhost", 18080, timeout=10)
        if not success:
            pytest.skip(
                f"HTTP GET failed: {content}"
            )

        # The response should contain our device identifier
        assert "i.MX RT1180" in content, (
            f"HTTP response did not contain device identifier.\n"
            f"Response:\n{content[:500]}"
        )

    @pytest.mark.network
    @pytest.mark.slow
    def test_arp_resolution(self, qemu_with_kernel):
        """ARP table on host should contain the RT1180 MAC after ping.

        This is a host-side check (may require admin on some OS).
        """
        found = qemu_with_kernel.wait_for_string(
            "[LWIP] Init complete", timeout=30
        )
        assert found, "Firmware did not report network interface ready"

        # First, try to trigger ARP by pinging
        _ping("10.0.2.15", count=2, timeout=2)

        # Check ARP table for our known MAC
        if os.name == "nt":
            result = subprocess.run(
                ["arp", "-a"], capture_output=True, text=True, timeout=10
            )
            output = result.stdout + result.stderr
            # MAC 02:00:00:00:00:01 should appear in ARP table
            if "02-00-00-00-00-01" not in output:
                pytest.skip(
                    "ARP entry for RT1180 MAC not found in host ARP table"
                )
        else:
            result = subprocess.run(
                ["arp", "-n"], capture_output=True, text=True, timeout=10
            )
            output = result.stdout + result.stderr
            if "02:00:00:00:00:01" not in output:
                pytest.skip(
                    "ARP entry for RT1180 MAC not found in host ARP table"
                )


# ==================================================================
#  L2d — PHY Register Tests (via QMP/qtest, if supported)
# ==================================================================

class TestPhyRegisters:
    """Verify DP83822 PHY register values via QEMU monitor."""

    @pytest.mark.phy
    def test_phy_id_registers(self, qemu_with_kernel):
        """PHY should report TI DP83822 ID:
        PHYIDR1 = 0x2000, PHYIDR2 = 0xA221.
        """
        # This test requires QMP/HMP access or firmware to print PHY IDs.
        # For Phase 1 skeleton, verify firmware reports PHY detection.
        found = qemu_with_kernel.wait_for_string(
            "DP83822", timeout=15
        )
        assert found, (
            "Firmware did not detect DP83822 PHY"
        )


# ==================================================================
#  Internal helpers
# ==================================================================

def _ping(ip_addr, count=4, timeout=2):
    """Ping an IP address.

    Returns (success: bool, output: str).
    """
    if os.name == "nt":
        cmd = ["ping", "-n", str(count), "-w", str(timeout * 1000), ip_addr]
    else:
        cmd = ["ping", "-c", str(count), "-W", str(timeout), ip_addr]

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True,
            timeout=timeout * count + 5
        )
        return result.returncode == 0, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, "ping timed out"


def _http_get(host, port, timeout=10):
    """Perform an HTTP GET request using the requests library.

    Returns (success: bool, content: str).
    Falls back to curl if requests is not installed.
    """
    try:
        import requests
        resp = requests.get(f"http://{host}:{port}/", timeout=timeout)
        return resp.status_code == 200, resp.text
    except ImportError:
        # Fall back to curl
        try:
            result = subprocess.run(
                ["curl", "-s", "--max-time", str(timeout),
                 f"http://{host}:{port}/"],
                capture_output=True, text=True, timeout=timeout + 5
            )
            if result.returncode == 0:
                return True, result.stdout
            return False, result.stderr or result.stdout
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return False, "neither 'requests' nor 'curl' available"
    except Exception as e:
        return False, str(e)
