#!/usr/bin/env python3

import functools
import re
import subprocess
import types
import yaml
from collections.abc import Mapping
from dataclasses import dataclass
from enum import Enum

PLATFORM_FOLDER = "/usr/share/sonic/platform"


class PcieDeviceType(str, Enum):
    ASIC = "asic"


# Maps each device type to the regex its pcie-variables.yaml variable names
# follow, so a variable can be classified by name alone. An ASIC's
# upstream-bridge secondary bus number is exposed as `asic_bus` (single-ASIC
# platforms) or `asic_<N>_bus` (multi-slot platforms).
_DEVICE_TYPE_VAR_PATTERNS: Mapping[PcieDeviceType, re.Pattern] = types.MappingProxyType({
    PcieDeviceType.ASIC: re.compile(r"^asic(_\d+)?_bus$"),
})


def device_type_for_var_name(var_name: str) -> PcieDeviceType | None:
    """Classify a pcie-variables.yaml variable name by its naming convention.

    Returns the matching PcieDeviceType, or None if the name matches no
    known device-type pattern. For example, an ASIC's upstream-bridge
    secondary bus number is exposed as `asic_bus` (single-ASIC platforms) or
    `asic_<N>_bus` (multi-slot platforms), both of which map to
    PcieDeviceType.ASIC; unrelated names like `cpu_card_fpga_bdf` map to None.
    """
    for device_type, pattern in _DEVICE_TYPE_VAR_PATTERNS.items():
        if pattern.match(var_name):
            return device_type
    return None


@functools.cache
def get_cmd_output(cmd: str) -> str:
    result = subprocess.run(["/bin/bash", "-c", cmd], capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"'{cmd}' -- command failed")

    return result.stdout.decode("utf-8").strip()


def get_var_name_to_cmd_map(vars_filepath) -> dict[str, str]:
    """
    Reads a yaml file containing a list of variables in the format of (name, lookup_command) pairs.

    For example:
    - name: "foo_bus"
      lookup_command: "setpci -s 00:02.1 0x19.b"
    - name: "bar_bus"
      lookup_command: "echo 'e5'"
    - name: "baz_bdf"
      lookup_command: "setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0'"

    Returns a dict mapping the variable name to the lookup_command.
    """
    result = dict()

    with open(vars_filepath, "r") as f:
        config = yaml.safe_load(f)
        for entry in config:
            name = entry.get("name")
            cmd = entry.get("lookup_command")
            if not name or not cmd:
                raise ValueError(
                    f"{vars_filepath} -- invalid format: each entry must contain 'name' and 'lookup_command'"
                )
            elif name in result:
                raise ValueError(f"{vars_filepath} -- duplicate variable name '{name}'")
            result[name] = cmd

    return result


def get_pcie_variables(vars_filepath, vars_to_get: set[str] | None = None) -> dict[str, str]:
    """
    Reads a yaml file containing a list of variables in the format of (name, lookup_command) pairs.

    For example:
    - name: "foo_bus"
      lookup_command: "setpci -s 00:02.1 0x19.b"
    - name: "bar_bus"
      lookup_command: "echo 'e5'"
    - name: "baz_bdf"
      lookup_command: "setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0'"

    Returns a dict mapping the variable name to the output of the lookup_command.
    If `vars_to_get` is provided, only returns the variables in `vars_to_get`.
    Otherwise, returns all variables.

    These variables are intended to be used for feeding the jinja2 templates,
    e.g. pddf-device.json.j2 and pcie.yaml.j2, as PCIe buses of the devices
    behind root ports can only be determined after boot.
    """
    all_vars = get_var_name_to_cmd_map(vars_filepath)

    return {
        name: get_cmd_output(cmd)
        for name, cmd in all_vars.items()
        if vars_to_get is None or name in vars_to_get
    }


def get_cpu_card_fpga_bdf(vars_filepath=f"{PLATFORM_FOLDER}/pcie-variables.yaml") -> str | None:
    return get_pcie_variables(vars_filepath, vars_to_get={"cpu_card_fpga_bdf"}).get(
        "cpu_card_fpga_bdf"
    )


def get_switchcard_fpga_bdf(vars_filepath=f"{PLATFORM_FOLDER}/pcie-variables.yaml") -> str | None:
    return get_pcie_variables(vars_filepath, vars_to_get={"switchcard_fpga_bdf"}).get(
        "switchcard_fpga_bdf"
    )


def get_switchcard_fpga_0_bdf(vars_filepath=f"{PLATFORM_FOLDER}/pcie-variables.yaml") -> str | None:
    return get_pcie_variables(vars_filepath, vars_to_get={"switchcard_fpga_0_bdf"}).get(
        "switchcard_fpga_0_bdf"
    )


def setpci_read(bdf: str, offset_with_width: str) -> str:
    """Run `setpci -s <bdf> <offset.width>` and return the raw hex string
    (bare hex digits, no leading 0x).

    Raises RuntimeError if the command fails or returns empty output, and
    propagates FileNotFoundError if setpci is not installed. Callers are
    responsible for logging and deciding how to react.
    """
    result = subprocess.run(
        ["setpci", "-s", bdf, offset_with_width],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"setpci read {bdf} {offset_with_width} failed: {result.stderr.strip()}"
        )
    value = result.stdout.strip()
    if not value:
        raise RuntimeError(f"setpci read {bdf} {offset_with_width} returned empty output")
    return value


def setpci_write(bdf: str, offset_with_width: str, value: str) -> None:
    """Run `setpci -s <bdf> <offset.width>=<hex>`.

    Raises RuntimeError if the command fails, and propagates
    FileNotFoundError if setpci is not installed.
    """
    result = subprocess.run(
        ["setpci", "-s", bdf, f"{offset_with_width}={value}"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"setpci write {bdf} {offset_with_width}={value} failed: "
            f"{result.stderr.strip()}"
        )


def apply_word_mask(old: int, *, set_mask: int = 0, clear_mask: int = 0) -> int:
    """OR in `set_mask`, then AND out `clear_mask` from a config-space word."""
    return (old | set_mask) & ~clear_mask


def pci_device_present(bdf: str) -> bool:
    """Return True if `lspci -n` reports a device at the given BDF.

    Raises RuntimeError if lspci fails, and propagates FileNotFoundError if
    lspci is not installed.
    """
    result = subprocess.run(["lspci", "-n"], capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"lspci -n failed: {result.stderr.strip()}")
    return bdf in result.stdout


# Generic PCI config-space register definitions, from include/uapi/linux/pci_regs.h.
PCI_COMMAND = 0x04  # 16-bit Command register
PCI_COMMAND_INTX_DISABLE = 0x0400  # bit 10: INTx Emulation Disable
PCI_CAPABILITY_LIST = 0x34  # 8-bit pointer to the first capability
PCI_CAP_ID_MSI = 0x05  # Message Signalled Interrupts capability
PCI_CAP_ID_MSIX = 0x11  # MSI-X capability
PCI_MSI_FLAGS = 0x02  # Message Control register, offset within the MSI cap
PCI_MSI_FLAGS_ENABLE = 0x0001  # bit 0: MSI enable
PCI_MSIX_FLAGS = 0x02  # Message Control register, offset within the MSI-X cap
PCI_MSIX_FLAGS_ENABLE = 0x8000  # bit 15: MSI-X enable


@dataclass(frozen=True)
class PciWordChange:
    """The before/after value of a 16-bit PCI config-space word write."""

    old: int
    new: int


def find_pci_capability(bdf: str, cap_id: int) -> int | None:
    """Walk the PCI capabilities list of `bdf` and return the config-space
    offset of the capability whose ID is `cap_id`, or None if not present.

    The list starts at PCI_CAPABILITY_LIST; each node stores its capability ID
    at offset +0 and the pointer to the next node at +1, with the low two bits
    of every pointer reserved (masked off). Raises (via setpci_read) if setpci
    fails. Bounded against malformed/looping lists.
    """
    # Capabilities are dword-aligned, so the low two bits of every pointer are
    # reserved -- mask them off (& 0xFC == & ~0x3) to get the real offset.
    ptr = int(setpci_read(bdf, f"0x{PCI_CAPABILITY_LIST:02x}.b"), 16) & 0xFC
    seen: set[int] = set()
    while ptr and ptr not in seen:
        seen.add(ptr)
        if int(setpci_read(bdf, f"0x{ptr:02x}.b"), 16) == cap_id:
            return ptr
        ptr = int(setpci_read(bdf, f"0x{ptr + 1:02x}.b"), 16) & 0xFC
    return None


def _modify_pci_word(bdf: str, offset: int, *, set_mask: int = 0, clear_mask: int = 0) -> PciWordChange:
    """Read the 16-bit config word at `offset`, apply the masks, write it back,
    and return the before/after values. Raises (via setpci) on failure.
    """
    word = f"0x{offset:02x}.w"
    old = int(setpci_read(bdf, word), 16)
    new = apply_word_mask(old, set_mask=set_mask, clear_mask=clear_mask)
    setpci_write(bdf, word, f"{new:04x}")
    return PciWordChange(old=old, new=new)


def disable_intx(bdf: str) -> PciWordChange:
    """Set the INTx Disable bit in the PCI Command register (always present)."""
    return _modify_pci_word(bdf, PCI_COMMAND, set_mask=PCI_COMMAND_INTX_DISABLE)


def disable_msi(bdf: str) -> PciWordChange | None:
    """Clear the MSI Enable bit. Returns None if the device has no MSI cap."""
    cap = find_pci_capability(bdf, PCI_CAP_ID_MSI)
    if cap is None:
        return None
    return _modify_pci_word(bdf, cap + PCI_MSI_FLAGS, clear_mask=PCI_MSI_FLAGS_ENABLE)


def disable_msix(bdf: str) -> PciWordChange | None:
    """Clear the MSI-X Enable bit. Returns None if the device has no MSI-X cap."""
    cap = find_pci_capability(bdf, PCI_CAP_ID_MSIX)
    if cap is None:
        return None
    return _modify_pci_word(bdf, cap + PCI_MSIX_FLAGS, clear_mask=PCI_MSIX_FLAGS_ENABLE)
