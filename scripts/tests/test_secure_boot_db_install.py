import re
import subprocess
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize("install_env", ["onie", "sonic", "build"])
@pytest.mark.parametrize("uefi", [False, True])
def test_db_auth_extraction_only_runs_on_uefi_targets(tmp_path, install_env, uefi):
    efi = tmp_path / "efi"
    if uefi:
        efi.mkdir()
    source = (ROOT / "installer/install.sh").read_text()
    block = source.split("# Enroll the UEFI Secure Boot db certificate BEFORE", 1)[1]
    block = block.split("# Decompress the file", 1)[0]
    block = "# Enroll the UEFI Secure Boot db certificate BEFORE" + block
    log = tmp_path / "calls"
    result = subprocess.run(
        ["sh", "-c", """
set -e
install_env="$1"
log="$2"
unzip() { printf 'extract\\n' >> "$log"; }
enroll_secure_boot_db() { printf 'enroll\\n' >> "$log"; }
""" + block.replace("/sys/firmware/efi", str(efi)), "test", install_env, str(log)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr
    calls = log.read_text().splitlines() if log.exists() else []
    assert calls == (["extract", "enroll"] if uefi and install_env != "build" else [])


def test_db_enrollment_does_not_mount_efivarfs(tmp_path):
    source = (ROOT / "installer/default_platform.conf").read_text()
    function = re.search(r"(?ms)^enroll_secure_boot_db\(\)\n\{.*?^\}", source).group()
    efivars = tmp_path / "efivars"
    efivars.mkdir()
    boot = tmp_path / "host/image-test/boot"
    boot.mkdir(parents=True)
    auth = boot / "DB.auth"
    auth.write_bytes(b"signed db update")
    log = tmp_path / "calls"
    result = subprocess.run(
        ["sh", "-c", """
set -e
image_dir=image-test
log="$2"
mountpoint() { printf 'mountpoint\\n' >> "$log"; return 1; }
mount() { printf 'mount\\n' >> "$log"; return 1; }
write_efi_auth_var() { printf 'write\\n' >> "$log"; }
""" + function.replace("/sys/firmware/efi/efivars", str(efivars))
         + '\nenroll_secure_boot_db "$1"\n', "test", str(tmp_path / "host"), str(log)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr
    assert log.read_text().splitlines() == ["write"]
    stored = list((tmp_path / "host/db-auth").glob("*.auth"))
    assert len(stored) == 1
    assert stored[0].read_bytes() == auth.read_bytes()
