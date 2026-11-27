# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Report a PSU's firmware version for fwutil and the platform API.

Reads the PSU's firmware version register (0xD9 unless --register says
otherwise) with a PMBus block read over i2c-dev and formats it as
major.minor.patch. The Delta DPS-1500AB's 0xD9 holds no version, so without
--register it reports MFR_REVISION (0x9B) for that model, which carries the
primary and secondary firmware revisions as ASCII. It reads the bus directly
rather than a PDDF sysfs attribute, so it does not depend on the PSU driver.

Run as `python3 -m nexthop.psu_fw_version` so it works both on the host and in
the pmon container. It uses only the standard library because pmon has no
click.
"""

import argparse
import errno
import os
import subprocess
import syslog

# Largest SMBus block payload (the kernel's I2C_SMBUS_BLOCK_MAX).
SMBUS_BLOCK_MAX = 32

# Manufacturer-specific PMBus command holding the firmware version.
DEFAULT_FW_VERSION_REGISTER = 0xD9

PMBUS_MFR_MODEL = 0x9A
PMBUS_MFR_REVISION = 0x9B

# MFR_MODEL prefixes of PSUs whose firmware version is the ASCII MFR_REVISION.
# Not the DPS-1800AB: its MFR_REVISION is a hardware revision ("S0").
MFR_REVISION_MODELS = ("DPS1500AB",)


class NoDeviceError(OSError):
    """Nothing answered at the address, e.g. the PSU slot is empty."""


def format_version(data: bytes) -> str:
    """Decode the firmware version block payload.

    Byte 0: patch, byte 1: minor, byte 2 (bits 0-6): major.
    """
    if len(data) < 3:
        raise ValueError(f"expected at least 3 bytes, got {len(data)}")
    return f"{data[2] & 0x7F}.{data[1]}.{data[0]}"


def format_revision(data: bytes) -> str:
    """Decode an ASCII MFR_REVISION block, e.g. b"05 06"."""
    text = data.decode("ascii").strip()
    if not text or not text.isprintable():
        raise ValueError(f"not a printable revision: {data!r}")
    return text


def _read(bus: int, address: int, register: int, length: int) -> list[int]:
    """Write `register`, then read `length` bytes.

    Uses -f because the PSU driver owns the address; the read has no side
    effects and the adapter lock serializes it with the driver's own transfers.
    """
    out = subprocess.run(
        ["i2ctransfer", "-f", "-y", str(bus),
         f"w1@0x{address:02x}", f"0x{register:02x}", f"r{length}"],
        check=False, capture_output=True, text=True, timeout=3,
    )
    if out.returncode != 0:
        if out.stderr.strip().endswith(os.strerror(errno.ENXIO)):
            raise NoDeviceError(out.stderr.strip())
        raise OSError(out.stderr.strip() or f"i2ctransfer exited with {out.returncode}")
    try:
        raw = [int(p, 16) for p in out.stdout.split()]
    except ValueError:
        raise ValueError(f"unexpected i2ctransfer output {out.stdout.strip()!r}") from None
    if not raw:
        raise ValueError("i2ctransfer returned no bytes")
    return raw


def read_block(bus: int, address: int, register: int) -> bytes:
    """Return the payload of the block at `register`.

    Like the pddf_psu driver, reads the length byte first and then the block,
    so the read stops at the end of the block.
    """
    count = min(_read(bus, address, register, 1)[0], SMBUS_BLOCK_MAX)
    if count == 0:
        return b""
    raw = _read(bus, address, register, count + 1)
    return bytes(raw[1:1 + raw[0]])


def read_version(bus: int, address: int, register: int | None = None) -> str:
    """Return the PSU firmware version, or "N/A" if it can't be read.

    With no `register`, the PSU's MFR_MODEL picks it.
    """
    device = f"{bus}-{address:04x}"
    decode = format_version
    try:
        if register is None:
            register = DEFAULT_FW_VERSION_REGISTER
            model = read_block(bus, address, PMBUS_MFR_MODEL).decode("ascii", "replace").strip()
            if model.startswith(MFR_REVISION_MODELS):
                register, decode = PMBUS_MFR_REVISION, format_revision
        data = read_block(bus, address, register)
    except NoDeviceError as e:
        # An empty PSU slot NACKs; psud already reports it as not present.
        syslog.syslog(syslog.LOG_DEBUG, f"No device at {device}: {e}")
        return "N/A"
    except (OSError, subprocess.SubprocessError, ValueError) as e:
        syslog.syslog(syslog.LOG_ERR, f"Failed to read firmware version for {device}: {e}")
        return "N/A"

    try:
        return decode(data)
    except ValueError as e:
        syslog.syslog(syslog.LOG_ERR, f"Failed to decode firmware version {data.hex()!r} for {device}: {e}")
        return "N/A"


def main(argv=None) -> None:
    parser = argparse.ArgumentParser(description="Report a PSU's firmware version")
    parser.add_argument("--bus", type=int, required=True, help="I2C bus number, e.g. 87")
    parser.add_argument("--address", type=lambda x: int(x, 0), required=True, help="I2C device address, e.g. 0x58")
    parser.add_argument("--register", type=lambda x: int(x, 0),
                        help="Firmware version register (default: by PSU model, "
                             f"0x{DEFAULT_FW_VERSION_REGISTER:02X} for most)")
    args = parser.parse_args(argv)
    # i2ctransfer refuses the addresses the I2C spec reserves (0x00-0x07, 0x78-0x7f).
    if not 0x08 <= args.address <= 0x77:
        parser.error(f"--address 0x{args.address:x} is outside 0x08-0x77")
    if args.register is not None and not 0 <= args.register <= 0xFF:
        parser.error(f"--register 0x{args.register:x} is not a PMBus command code")

    syslog.openlog("psu_fw_version")
    print(read_version(args.bus, args.address, args.register))


if __name__ == "__main__":
    main()
