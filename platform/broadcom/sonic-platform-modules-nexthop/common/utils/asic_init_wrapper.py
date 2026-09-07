#!/usr/bin/env python3
"""
Boot-time wrapper around asic_init.sh, called from pre_pddf_init.sh:

  - warm-reboot post-kexec (SONIC_BOOT_TYPE=warm on cmdline): preserve
    dataplane, disable stale ASIC interrupts at the PCI level, skip
    asic_init.sh.
  - Otherwise (cold boot or fast-reboot post-kexec): exec asic_init.sh
    for a full ASIC reset.

The interrupt-disable step is the reason this wrapper exists: after a
warm reboot the ASIC driver is reloaded by opennsl-modules.service, and
if interrupts are still enabled from the pre-kexec kernel an IRQ can
fire before userspace remaps and updates the BARs, panicking the kernel.
"""

import os
import re
import sys
import syslog

from nexthop import pcie_lib

ASIC_INIT_SCRIPT = "/usr/local/bin/asic_init.sh"
PROC_CMDLINE = "/proc/cmdline"

# SONIC_BOOT_TYPE=warm — warm-reboot post-kexec marker on the kernel
# cmdline. fast-reboot uses SONIC_BOOT_TYPE=fast-reboot and is
# intentionally NOT treated as warm here: fast-reboot post-kexec gets a
# full ASIC reset.
WARM_BOOT_CMDLINE_RE = re.compile(r"(?:^|\s)SONIC_BOOT_TYPE=warm(?:\s|$)")


def log_info(msg: str) -> None:
    syslog.syslog(syslog.LOG_INFO, msg)


def log_err(msg: str) -> None:
    syslog.syslog(syslog.LOG_ERR, msg)


def is_warm_boot_post_kexec() -> bool:
    try:
        with open(PROC_CMDLINE) as f:
            cmdline = f.read()
    except OSError as e:
        log_err(f"Cannot read {PROC_CMDLINE}: {e}")
        return False
    return bool(WARM_BOOT_CMDLINE_RE.search(cmdline))


def get_asic_bdfs() -> list[str]:
    """Look up every candidate ASIC BDF (e.g. "01:00.0") from the
    platform's pcie-variables.yaml. The yaml exposes each ASIC bridge's
    secondary bus number as `asic_bus` or `asic_<N>_bus`; the ASIC itself
    enumerates as device 0, function 0 on that bus.
    """
    try:
        name_to_cmd = pcie_lib.get_var_name_to_cmd_map(
            f"{pcie_lib.PLATFORM_FOLDER}/pcie-variables.yaml"
        )
    except Exception as e:
        # Catch broadly: a missing/corrupt pcie-variables.yaml must NOT
        # prevent the cold-boot fallback to asic_init.sh.
        log_err(f"Failed to read pcie-variables.yaml: {e}")
        return []

    bdfs: list[str] = []
    for name, cmd in name_to_cmd.items():
        if pcie_lib.device_type_for_var_name(name) != pcie_lib.PcieDeviceType.ASIC:
            continue
        try:
            bus = pcie_lib.get_cmd_output(cmd)
        except Exception as e:
            # Tolerate per-slot lookup failures: an unpopulated slot on a
            # multi-ASIC platform can legitimately fail to resolve.
            log_err(f"Failed to resolve {name}: {e}")
            continue
        if bus:
            bdfs.append(f"{bus}:00.0")
    return bdfs


def asic_present_on_pci_bus(bdf: str) -> bool:
    """Return True if `lspci -n` reports a device at the given BDF. Logs and
    returns False on any failure so the boot path can fall back gracefully.
    """
    try:
        return pcie_lib.pci_device_present(bdf)
    except Exception as e:
        log_err(f"lspci check for {bdf} failed: {e}")
        return False


def _log_disable(bdf: str, label: str, disable_fn) -> None:
    """Run one pcie_lib interrupt-disable function and log the outcome. The
    pcie_lib functions are syslog-free and either return a PciWordChange,
    return None when the capability is absent, or raise on setpci failure --
    this wrapper owns all the logging and never propagates.
    """
    try:
        change = disable_fn(bdf)
    except Exception as e:
        log_err(f"Warm boot: {label} failed: {e}")
        return
    if change is None:
        log_info(f"Warm boot: {label} skipped (capability not present)")
        return
    log_info(f"Warm boot: {label} (was: 0x{change.old:04x}, now: 0x{change.new:04x})")


def disable_asic_pci_interrupts(bdf: str) -> None:
    """Disable INTx, MSI-X, and MSI on the ASIC endpoint. The driver may
    have left interrupts enabled from before kexec; if it loads with them
    still enabled, an IRQ can fire before userspace maps the PIO memory
    and panic the kernel.
    """
    log_info("Warm boot: Disabling all PCI interrupts")
    _log_disable(bdf, "INTx disabled via PCI Command", pcie_lib.disable_intx)
    _log_disable(bdf, "MSI-X disabled", pcie_lib.disable_msix)
    _log_disable(bdf, "MSI disabled", pcie_lib.disable_msi)
    log_info(
        "Warm boot: PCI interrupts disabled, "
        "module load deferred to opennsl-modules.service"
    )


def handle_warm_boot_post_kexec() -> bool:
    """Returns True iff warm-boot interrupt cleanup succeeded for at
    least one ASIC and the caller should skip asic_init.sh.
    """
    candidate_bdfs = get_asic_bdfs()
    if not candidate_bdfs:
        log_err("Warm boot: Cannot determine ASIC BDF")
        return False

    present_bdfs = [bdf for bdf in candidate_bdfs if asic_present_on_pci_bus(bdf)]
    if not present_bdfs:
        log_err(f"Warm boot: no ASIC found at any of {candidate_bdfs}")
        return False

    for bdf in present_bdfs:
        log_info(f"Warm boot: ASIC found at {bdf}")
        disable_asic_pci_interrupts(bdf)
    return True


def main(argv: list[str]) -> int:
    syslog.openlog("asic_init_wrapper")

    if is_warm_boot_post_kexec():
        log_info(
            "Warm boot post-kexec: disabling stale interrupts, skipping asic_init.sh"
        )
        if handle_warm_boot_post_kexec():
            return 0
        log_err("Warm boot interrupt disable failed, falling back to full ASIC init")

    log_info("Cold boot or fast-reboot post-kexec: running asic_init.sh")
    os.execv(ASIC_INIT_SCRIPT, [ASIC_INIT_SCRIPT, *argv[1:]])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
