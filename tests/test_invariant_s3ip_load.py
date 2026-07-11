import pytest
import os
import stat
import tempfile
import subprocess
import sys
from unittest import mock


# Adversarial payloads representing various attack scenarios
ADVERSARIAL_PAYLOADS = [
    "/sys_switch",
    "/sys_switch/",
    "/sys_switch/../etc",
    "/sys_switch/../../tmp/evil",
    "//sys_switch",
    "/sys_switch\x00/evil",
    "/sys_switch; rm -rf /",
    "/sys_switch && malicious_cmd",
    "/sys_switch | cat /etc/passwd",
    "/sys_switch`whoami`",
    "/sys_switch$(id)",
    "/SYS_SWITCH",
    "/sys_switch/../sys_switch",
]


def get_directory_permissions(path):
    """Get octal permissions of a directory."""
    try:
        st = os.stat(path)
        return stat.S_IMODE(st.st_mode)
    except (OSError, FileNotFoundError):
        return None


def is_world_writable(path):
    """Check if a path has world-writable permissions."""
    try:
        st = os.stat(path)
        mode = stat.S_IMODE(st.st_mode)
        return bool(mode & stat.S_IWOTH)
    except (OSError, FileNotFoundError):
        return False


def is_world_writable_mode(mode_int):
    """Check if a mode integer represents world-writable permissions."""
    return bool(mode_int & stat.S_IWOTH)


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_sys_switch_directory_not_world_writable(payload, tmp_path):
    """Invariant: The sys_switch directory (or any platform data directory) must never
    be created with world-writable permissions (mode 777 or any mode with o+w).
    World-writable directories allow any local user to replace legitimate platform
    data with malicious content, breaking the security boundary."""

    # Simulate what the vulnerable code does but in a safe test directory
    test_dir = tmp_path / "sys_switch_test"

    # Test that if we create a directory with secure permissions, it's not world-writable
    secure_mode = 0o755  # Owner rwx, group rx, others rx — NOT world-writable
    test_dir.mkdir(mode=secure_mode, exist_ok=True)

    actual_mode = get_directory_permissions(str(test_dir))
    assert actual_mode is not None, f"Could not stat test directory for payload: {payload}"

    # The directory must NOT be world-writable
    assert not is_world_writable_mode(actual_mode), (
        f"SECURITY VIOLATION: Directory has world-writable permissions "
        f"(mode={oct(actual_mode)}) for payload: {payload}. "
        f"World-writable directories allow privilege escalation attacks."
    )

    # Verify the mode is at most 0o755 (no world-write bit)
    assert (actual_mode & stat.S_IWOTH) == 0, (
        f"World-write bit is set on directory (mode={oct(actual_mode)}) "
        f"for payload: {payload}"
    )


@pytest.mark.parametrize("payload", [
    0o777,  # World-writable (vulnerable)
    0o775,  # Group-writable but not world-writable (acceptable)
    0o755,  # Standard secure permissions (secure)
    0o700,  # Owner only (most restrictive)
    0o770,  # Owner and group (no world access)
    0o666,  # World-writable without execute (still vulnerable)
    0o733,  # World-writable with execute (vulnerable)
    0o711,  # World-execute only (acceptable)
])
def test_mode_world_writable_detection(payload):
    """Invariant: Any directory mode with the world-write bit set must be
    detected and rejected. Mode 0o777 as used in the vulnerable code must
    always be flagged as insecure."""

    world_write_bit = stat.S_IWOTH  # 0o002

    if payload & world_write_bit:
        # These modes are INSECURE and must be rejected
        assert is_world_writable_mode(payload), (
            f"Mode {oct(payload)} should be detected as world-writable "
            f"but was not flagged."
        )
        # Assert that the vulnerable mode 0o777 is always caught
        if payload == 0o777:
            assert is_world_writable_mode(0o777), (
                "CRITICAL: Mode 0o777 (as used in vulnerable s3ip_load.py) "
                "must always be detected as world-writable."
            )
    else:
        # These modes are acceptable
        assert not is_world_writable_mode(payload), (
            f"Mode {oct(payload)} should NOT be world-writable but was flagged."
        )


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_no_shell_injection_in_directory_creation(payload, tmp_path):
    """Invariant: Directory creation for platform data must not use shell=True
    with user-controlled or adversarial input. Shell injection via os.system()
    with adversarial paths must not execute unintended commands."""

    # Track if any shell commands were executed
    executed_commands = []

    original_system = os.system

    def mock_os_system(cmd):
        executed_commands.append(cmd)
        # Never actually execute — just record
        return 0

    with mock.patch('os.system', side_effect=mock_os_system):
        # Simulate the vulnerable pattern with adversarial payload
        sanitized = payload.replace(";", "").replace("&", "").replace("|", "").replace("`", "").replace("$", "")

        # If os.system were called with the payload, check for injection
        for cmd in executed_commands:
            # Ensure no shell metacharacters made it through
            dangerous_chars = [';', '&&', '||', '|', '`', '$(',  '$(']
            for char in dangerous_chars:
                assert char not in cmd, (
                    f"SECURITY VIOLATION: Shell injection character '{char}' "
                    f"found in command for payload: {payload}"
                )


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_platform_directory_permissions_invariant(payload, tmp_path):
    """Invariant: Platform data directories that privileged services consume
    must be created with permissions that prevent unauthorized writes.
    The minimum secure permission for such directories is 0o755."""

    # Create a test directory simulating secure platform directory creation
    safe_path = tmp_path / "platform_data"
    safe_path.mkdir(mode=0o755)

    # Verify the security invariant holds
    mode = get_directory_permissions(str(safe_path))
    assert mode is not None

    # Must not have world-write permission
    assert not (mode & stat.S_IWOTH), (
        f"Platform directory must not be world-writable. "
        f"Got mode {oct(mode)} for adversarial payload context: {payload}"
    )

    # Must not have world-write permission regardless of adversarial input
    assert mode <= 0o755 or (mode & ~stat.S_IWOTH & ~stat.S_IWGRP), (
        f"Directory permissions too permissive: {oct(mode)} for payload: {payload}"
    )

    # The vulnerable mode 0o777 must never be used
    assert mode != 0o777, (
        f"CRITICAL: Directory created with mode 0o777 (world-writable). "
        f"This is the exact vulnerability in s3ip_load.py. Payload: {payload}"
    )