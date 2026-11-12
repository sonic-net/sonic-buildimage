#!/usr/bin/env python3
"""
PSU hot-swap monitor for ln4332z-v1.

The PSUs are bound to the generic pmbus driver once, at boot, by
ln4332z-v1-init.sh. The driver reads VOUT_MODE (the LINEAR16 exponent of
every output-voltage value) only when it probes, and the init script reads the
model / serial for pmon only once as well. Swap a PSU for one whose firmware
uses another exponent and both go stale:

    W-CRPS800N2    VOUT_MODE 0x17 (2^-9)
    D801M12-2M4A   VOUT_MODE 0x15 (2^-11)

With the W-CRPS800N2's exponent the D801M12's 12.35 V read as 49.38 V and
system-health marked the PSU "voltage is out of range"; show platform psustatus
still named the old model and serial. A reboot cleared both.

This daemon puts that right without a reboot. Every POLL_INTERVAL it reads, per
PSU, the live VOUT_MODE / READ_VOUT / MFR_SERIAL straight from the PSU and
compares them with what the driver and the cache report. When the serial has
changed, or the driver's vout1 disagrees with the voltage decoded with the
live exponent, the PSU's pmbus device is re-bound (so the driver probes the PSU
that is actually there) and its model / serial cache is rewritten.

Only the two PSU slots at 0x5b are ever accessed: the driver and the init
script already talk to both, present or not. No other address is touched - a
transfer to an address that does not answer wedges the CP2112 bridge.
"""

import glob
import logging
import logging.handlers
import os
import subprocess
import sys
import time

# ----------------------------- Configuration -----------------------------

POLL_INTERVAL = 10

PSU_ADDR = 0x5b
PSU_MUX_ADDR = 0x70              # PCA9548 on the system CP2112 bridge
PSU_MUX_CHANNELS = (0, 1)        # PSU-1, PSU-2

PMBUS_VOUT_MODE = 0x20
PMBUS_READ_VOUT = 0x8b
PMBUS_MFR_MODEL = 0x9a
PMBUS_MFR_SERIAL = 0x9e

PMBUS_DRIVER = "/sys/bus/i2c/drivers/pmbus"
I2C_DEVICES = "/sys/bus/i2c/devices"
# written by ln4332z-v1-init.sh, read by sonic_platform/psu.py in pmon
CACHE_DIR = "/var/run/platform_cache/ln4332z-v1"

# The driver's vout1 and the live decode are two reads a moment apart, so a
# real mismatch has to clear both an absolute and a relative margin. A stale
# exponent is off by a power of two - 4x in the case above.
VOUT_TOLERANCE_V = 0.5
VOUT_TOLERANCE_REL = 0.10
# below this the PSU delivers no output and there is nothing to compare
VOUT_MIN_V = 1.0

# Settle time before the new driver instance is read back.
REBIND_SETTLE = 2

# ---------------------------------------------------------------------------

log = logging.getLogger("psu_monitor")

from sonic_platform.bridges import bridge_bus, mux_child


def i2cget(bus, reg, mode):
    """Read one PMBus command at PSU_ADDR, or None if the PSU does not answer.

    -f: the address is bound to the pmbus driver.
    """
    try:
        out = subprocess.run(["i2cget", "-f", "-y", str(bus), hex(PSU_ADDR),
                              hex(reg), mode],
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                             text=True, timeout=5)
    except Exception:
        return None
    if out.returncode != 0:
        return None
    return out.stdout.strip()


def read_string(bus, reg):
    """Printable part of an SMBus block read (first byte = length)."""
    raw = i2cget(bus, reg, "s")
    if not raw:
        return None
    try:
        data = [int(x, 16) for x in raw.split()]
    except ValueError:
        return None
    return bytes(c for c in data[1:] if 32 <= c < 127).decode().strip() or None


def read_live_vout(bus):
    """Output voltage decoded with the exponent the PSU reports now."""
    mode = i2cget(bus, PMBUS_VOUT_MODE, "b")
    vout = i2cget(bus, PMBUS_READ_VOUT, "w")
    if mode is None or vout is None:
        return None
    mode = int(mode, 16)
    if mode >> 5:              # not LINEAR16 - nothing to compare against
        return None
    exp = mode & 0x1f
    if exp > 15:
        exp -= 32
    return int(vout, 16) * 2.0 ** exp


def device_name(bus):
    return "{}-{:04x}".format(bus, PSU_ADDR)


def read_driver_vout(bus):
    """vout1 as the bound pmbus driver reports it, in volts."""
    for label in glob.glob(os.path.join(I2C_DEVICES, device_name(bus),
                                        "hwmon", "hwmon*", "in*_label")):
        try:
            with open(label) as f:
                if f.read().strip() != "vout1":
                    continue
            with open(label[:-len("_label")] + "_input") as f:
                return int(f.read().strip()) / 1000.0
        except (OSError, ValueError):
            return None
    return None


def vout_mismatch(driver, live):
    if driver is None or live is None or live < VOUT_MIN_V:
        return False
    return abs(driver - live) > max(VOUT_TOLERANCE_V, live * VOUT_TOLERANCE_REL)


def rebind(bus):
    name = device_name(bus)
    for action in ("unbind", "bind"):
        try:
            with open(os.path.join(PMBUS_DRIVER, action), "w") as f:
                f.write(name)
        except OSError as e:
            # unbind of an unbound device fails; bind is what matters
            if action == "bind":
                log.error("cannot bind %s to pmbus: %s", name, e)
                return False
    time.sleep(REBIND_SETTLE)
    return True


def cache_path(index, what):
    return os.path.join(CACHE_DIR, "psu{}_{}".format(index + 1, what))


def read_cache(index, what):
    try:
        with open(cache_path(index, what)) as f:
            return f.read().strip()
    except OSError:
        return None


def write_cache(index, what, value):
    try:
        os.makedirs(CACHE_DIR, exist_ok=True)
        with open(cache_path(index, what), "w") as f:
            f.write((value or "") + "\n")
    except OSError as e:
        log.warning("cannot write %s: %s", cache_path(index, what), e)


def check_psu(index, bus):
    serial = read_string(bus, PMBUS_MFR_SERIAL)
    if serial is None:
        # PSU absent or not answering: nothing to compare, keep the cache
        return

    cached = read_cache(index, "serial")
    replaced = cached != serial
    live = read_live_vout(bus)
    driver = read_driver_vout(bus)
    stale = vout_mismatch(driver, live)
    if not replaced and not stale:
        return

    name = "PSU-{}".format(index + 1)
    if replaced:
        log.info("%s replaced (serial %s -> %s) - re-binding the pmbus driver",
                 name, cached or "none", serial)
    else:
        log.info("%s: driver reports %.3f V, the PSU %.3f V - stale VOUT_MODE,"
                 " re-binding the pmbus driver", name, driver, live)

    if not rebind(bus):
        return
    write_cache(index, "model", read_string(bus, PMBUS_MFR_MODEL))
    write_cache(index, "serial", serial)

    driver = read_driver_vout(bus)
    if vout_mismatch(driver, read_live_vout(bus)):
        log.warning("%s: driver still reports %.3f V after re-binding", name, driver)
    elif driver is not None:
        log.info("%s: driver reports %.3f V", name, driver)


def main():
    handler = logging.handlers.SysLogHandler(address="/dev/log")
    handler.setFormatter(logging.Formatter("psu_monitor: %(message)s"))
    log.addHandler(handler)
    log.addHandler(logging.StreamHandler())
    log.setLevel(logging.INFO)

    while True:
        sys_bus = bridge_bus("sys")
        for index, channel in enumerate(PSU_MUX_CHANNELS):
            bus = mux_child(sys_bus, PSU_MUX_ADDR, channel) if sys_bus is not None else None
            if bus is None:
                continue
            try:
                check_psu(index, bus)
            except Exception as e:
                log.warning("PSU-%d check failed: %s", index + 1, e)
        time.sleep(POLL_INTERVAL)


if __name__ == "__main__":
    sys.exit(main())
