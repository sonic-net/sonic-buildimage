#!/usr/bin/env python3
"""Boot-time wrapper around asic_init.sh.

  --stage pre-pddf   (default, from pre_pddf_init.sh): reset the ASIC, unless
                     a warm or fast reboot wants the dataplane preserved.
  --stage pre-driver (from the opennsl-modules.service drop-in): run a reset
                     the pre-pddf stage deferred.

Interrupts are disabled at the PCI level on the preserved paths: with the
driver reloaded post-kexec, an IRQ can fire before userspace remaps the BARs
and panic the kernel.
"""

import argparse
import os
import re
import sys
import syslog

from nexthop import pcie_lib

ASIC_INIT_SCRIPT = "/usr/local/bin/asic_init.sh"
PROC_CMDLINE = "/proc/cmdline"

# Stage handoff marker; /run is tmpfs so it cannot survive into the next boot.
DEFERRED_RESET_MARKER = "/run/nexthop-asic-init-deferred"

WARM_BOOT_CMDLINE_RE = re.compile(r"(?:^|\s)SONIC_BOOT_TYPE=warm(?:\s|$)")
FAST_BOOT_CMDLINE_RE = re.compile(r"(?:^|\s)SONIC_BOOT_TYPE=fast-reboot(?:\s|$)")


def log_info(msg: str) -> None:
    syslog.syslog(syslog.LOG_INFO, msg)


def log_err(msg: str) -> None:
    syslog.syslog(syslog.LOG_ERR, msg)


def _cmdline_matches(pattern: re.Pattern) -> bool:
    try:
        with open(PROC_CMDLINE) as f:
            cmdline = f.read()
    except OSError as e:
        log_err(f"Cannot read {PROC_CMDLINE}: {e}")
        return False
    return bool(pattern.search(cmdline))


def is_warm_boot_post_kexec() -> bool:
    return _cmdline_matches(WARM_BOOT_CMDLINE_RE)


def is_fast_reboot_post_kexec() -> bool:
    return _cmdline_matches(FAST_BOOT_CMDLINE_RE)


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


def _log_disable(bdf: str, label: str, disable_fn, boot_label: str = "Warm boot") -> None:
    """Run one pcie_lib interrupt-disable function and log the outcome. The
    pcie_lib functions are syslog-free and either return a PciWordChange,
    return None when the capability is absent, or raise on setpci failure --
    this wrapper owns all the logging and never propagates.
    """
    try:
        change = disable_fn(bdf)
    except Exception as e:
        log_err(f"{boot_label}: {label} failed: {e}")
        return
    if change is None:
        log_info(f"{boot_label}: {label} skipped (capability not present)")
        return
    log_info(f"{boot_label}: {label} (was: 0x{change.old:04x}, now: 0x{change.new:04x})")


def disable_asic_pci_interrupts(bdf: str, boot_label: str = "Warm boot") -> None:
    """Disable INTx, MSI-X, and MSI on the ASIC endpoint. The driver may
    have left interrupts enabled from before kexec; if it loads with them
    still enabled, an IRQ can fire before userspace maps the PIO memory
    and panic the kernel.
    """
    log_info(f"{boot_label}: Disabling all PCI interrupts")
    _log_disable(bdf, "INTx disabled via PCI Command", pcie_lib.disable_intx, boot_label)
    _log_disable(bdf, "MSI-X disabled", pcie_lib.disable_msix, boot_label)
    _log_disable(bdf, "MSI disabled", pcie_lib.disable_msi, boot_label)
    log_info(
        f"{boot_label}: PCI interrupts disabled, "
        "module load deferred to opennsl-modules.service"
    )


def handle_warm_boot_post_kexec(boot_label: str = "Warm boot") -> bool:
    """Returns True iff interrupt cleanup succeeded for at least one ASIC
    and the caller should skip asic_init.sh at this stage.
    """
    candidate_bdfs = get_asic_bdfs()
    if not candidate_bdfs:
        log_err(f"{boot_label}: Cannot determine ASIC BDF")
        return False

    present_bdfs = [bdf for bdf in candidate_bdfs if asic_present_on_pci_bus(bdf)]
    if not present_bdfs:
        log_err(f"{boot_label}: no ASIC found at any of {candidate_bdfs}")
        return False

    for bdf in present_bdfs:
        log_info(f"{boot_label}: ASIC found at {bdf}")
        disable_asic_pci_interrupts(bdf, boot_label)
    return True


def mark_reset_deferred() -> bool:
    try:
        with open(DEFERRED_RESET_MARKER, "w"):
            pass
    except OSError as e:
        log_err(f"Cannot write {DEFERRED_RESET_MARKER}: {e}")
        return False
    return True


def take_deferred_reset_marker() -> bool:
    """Consume the marker so a service restart cannot re-reset the ASIC."""
    try:
        os.unlink(DEFERRED_RESET_MARKER)
    except FileNotFoundError:
        return False
    except OSError as e:
        log_err(f"Cannot remove {DEFERRED_RESET_MARKER}: {e}")
        return False
    return True


def run_pre_driver_stage(extra_args: list[str]) -> int:
    if not take_deferred_reset_marker():
        log_info("pre-driver stage: no deferred-reset marker, nothing to do")
        return 0
    log_info("Fast reboot post-kexec: running deferred asic_init.sh before driver load")
    os.execv(ASIC_INIT_SCRIPT, [ASIC_INIT_SCRIPT, *extra_args])


def run_pre_pddf_stage(extra_args: list[str]) -> int:
    if is_warm_boot_post_kexec():
        log_info(
            "Warm boot post-kexec: disabling stale interrupts, skipping asic_init.sh"
        )
        if handle_warm_boot_post_kexec():
            return 0
        log_err("Warm boot interrupt disable failed, falling back to full ASIC init")
    elif is_fast_reboot_post_kexec():
        log_info(
            "Fast reboot post-kexec: disabling stale interrupts, "
            "deferring ASIC reset to just before driver load"
        )
        if handle_warm_boot_post_kexec("Fast reboot") and mark_reset_deferred():
            return 0
        log_err("Fast reboot deferral failed, falling back to full ASIC init")

    log_info("Cold boot: running asic_init.sh")
    os.execv(ASIC_INIT_SCRIPT, [ASIC_INIT_SCRIPT, *extra_args])


def main(argv: list[str]) -> int:
    syslog.openlog("asic_init_wrapper")

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--stage",
        choices=["pre-pddf", "pre-driver"],
        default="pre-pddf",
    )
    args, extra_args = parser.parse_known_args(argv[1:])

    if args.stage == "pre-driver":
        return run_pre_driver_stage(extra_args)
    else:
        return run_pre_pddf_stage(extra_args)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
