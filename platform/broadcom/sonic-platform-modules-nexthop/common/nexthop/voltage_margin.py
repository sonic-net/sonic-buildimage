# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Voltage margining for Nexthop platforms.

Only rails defined in `voltage_margin_config.json` in the platform device directory
can be margined.

Safety guards: An absolute [min, max] clamp is defined per rail. There is also a
VOUT_MODE sanity check and a read-back settle check that auto-reverts to nominal.
"""

import json
import os
import time

from nexthop import fpga_lib

try:
    from smbus2 import SMBus
except ImportError:
    SMBus = None

CONFIG_FILENAME = "voltage_margin_config.json"
DEFAULT_PLATFORM_DIR = "/usr/share/sonic/platform"

# AVS profiles: AVS code -> commanded VDDC level (volts) for a board/design.
# A rail selects its profile via the "avs_profile" config key; only codes present
# in the selected profile are valid.
AVS_PROFILES = {
    # nh-5010 core-VDDC AVS map
    "nh5010_q3d_vddc": {
        0x7A: 0.8500, 0x7C: 0.8375, 0x7E: 0.8250, 0x80: 0.8125,
        0x82: 0.8000, 0x84: 0.7875, 0x86: 0.7750, 0x88: 0.7625,
        0x8A: 0.7500, 0x8C: 0.7375, 0x8E: 0.7250, 0x90: 0.7125,
        0x92: 0.7000, 0x94: 0.6875, 0x96: 0.6750, 0x98: 0.6625,
        0x9A: 0.6500,
    },
}

# Default FPGA whose register holds the live AVS-commanded levels.
# Overridable per-rail via the optional "avs_fpga_name" config key.
DEFAULT_AVS_FPGA = "SWITCHCARD_FPGA"

# Device types this tool knows how to drive; selected per-rail via "device_type".
SUPPORTED_DEVICE_TYPES = ("xdpe1a2g5b",)

# PMBus registers.
PMBUS_PAGE = 0x00
PMBUS_OPERATION = 0x01
PMBUS_VOUT_MODE = 0x20
PMBUS_VOUT_MARGIN_HIGH = 0x25
PMBUS_VOUT_MARGIN_LOW = 0x26
PMBUS_READ_VOUT = 0x8B
# OPERATION: bit7 enable=1; bits[5:4] margin-select 00=nominal, 01=low, 10=high;
# bits[3:2] fault-response (ignore fault while margined).
OPERATION_NOMINAL = 0x80
OPERATION_MARGIN_LOW = 0x98
OPERATION_MARGIN_HIGH = 0xA8

# Physically-plausible Linear16 volts-per-LSB band (exponent -11..-9, nominal
# -10 => ~0.9766 mV/LSB).  Outside this => corrupt VOUT_MODE read.
_VPL_MIN = 0.0004
_VPL_MAX = 0.0021

# READ_VOUT settle tolerance / poll budget.
_SETTLE_TOLERANCE_V = 0.025
_SETTLE_POLLS = 10

# Margin levels.
LEVEL_HIGH = "high"
LEVEL_LOW = "low"
LEVEL_NOMINAL = "nominal"
LEVELS = (LEVEL_HIGH, LEVEL_LOW, LEVEL_NOMINAL)

# Dry-run simulated reads.  VOUT_MODE 0x16 => Linear16 exponent -10.  FPGA AVS
# 0x8686 => 0.775 V baseline for both nibbles.
_DRYRUN_READS = {PMBUS_PAGE: 0x00, PMBUS_VOUT_MODE: 0x16, PMBUS_OPERATION: 0x80}
_DRYRUN_FPGA_REG = 0x00008686


class VoltageMarginError(Exception):
    """Raised for any condition that should abort margining a rail."""

# --------------------------------------------------------------------------- #
# Helpers                                                                     #
# --------------------------------------------------------------------------- #
def _log(msg):
    print(msg, flush=True)


def _vlog(verbose, msg):
    if verbose:
        print(msg, flush=True)


def _platform_dir():
    try:
        from sonic_py_common import device_info

        path = device_info.get_path_to_platform_dir()
        if path:
            return path
    except Exception:
        pass
    return DEFAULT_PLATFORM_DIR


def load_config(platform_dir=None):
    """Load and minimally validate `voltage_margin_config.json`. Returns the
    `rails` dict. Raises VoltageMarginError if the file is missing or
    malformed."""
    platform_dir = platform_dir or _platform_dir()
    path = os.path.join(platform_dir, CONFIG_FILENAME)
    if not os.path.isfile(path):
        raise VoltageMarginError("no voltage-margin config at %s" % path)
    try:
        with open(path) as f:
            data = json.load(f)
    except (OSError, ValueError) as e:
        raise VoltageMarginError("cannot read %s: %s" % (path, e))
    rails = data.get("rails")
    if not isinstance(rails, dict) or not rails:
        raise VoltageMarginError("%s has no 'rails' object" % path)
    return rails


def get_rail(rails, name):
    """Return the config entry for `name`, enforcing the allowlist: a rail not
    declared in the config is refused (never margined)."""
    rail = rails.get(name)
    if rail is None:
        raise VoltageMarginError(
            "rail %r is not declared in %s (declared rails: %s)"
            % (name, CONFIG_FILENAME, ", ".join(sorted(rails))))
    dtype = rail.get("device_type")
    if dtype not in SUPPORTED_DEVICE_TYPES:
        raise VoltageMarginError(
            "rail %r has unsupported device_type %r (supported: %s)"
            % (name, dtype, ", ".join(SUPPORTED_DEVICE_TYPES)))
    return rail


def _parse_bits(spec):
    """Parse an `avs_fpga_bits` 'lo:hi' field into a (lo, hi) tuple."""
    try:
        lo_s, hi_s = str(spec).split(":")
        lo, hi = int(lo_s), int(hi_s)
    except (ValueError, AttributeError):
        raise VoltageMarginError("bad avs_fpga_bits %r (expected 'lo:hi')" % spec)
    if lo > hi:
        raise VoltageMarginError("avs_fpga_bits %r must be 'lo:hi' (lo <= hi)" % spec)
    return (lo, hi)


def _avs_table(rail):
    """Resolve a rail's AVS code->VDDC table from its 'avs_profile' key."""
    profile = rail.get("avs_profile")
    if profile is None:
        raise VoltageMarginError(
            "rail has no 'avs_profile' -- cannot decode its AVS baseline")
    table = AVS_PROFILES.get(profile)
    if table is None:
        raise VoltageMarginError(
            "unknown avs_profile %r (known: %s)"
            % (profile, ", ".join(sorted(AVS_PROFILES))))
    return table


# --------------------------------------------------------------------------- #
# AVS baseline (FPGA)                                                         #
# --------------------------------------------------------------------------- #
def read_avs_baseline_mv(rail, dry_run=False, verbose=False):
    """Live per-rail baseline (mV): read the AVS-commanded VDDC level from the
    FPGA register and map it through the rail's AVS profile."""
    table = _avs_table(rail)
    reg = int(str(rail["avs_fpga_register"]), 16)
    bits = _parse_bits(rail["avs_fpga_bits"])
    fpga_name = rail.get("avs_fpga_name", DEFAULT_AVS_FPGA)

    if dry_run:
        reg_val = _DRYRUN_FPGA_REG
        _vlog(verbose, "  [dry-run] %s 0x%02x -> 0x%08x (simulated)"
                       % (fpga_name, reg, reg_val))
    else:
        bdf = fpga_lib.name_to_bdf(fpga_name)
        if not bdf:
            raise VoltageMarginError("FPGA %r not found (cannot read AVS)" % fpga_name)
        reg_val = fpga_lib.read_32(bdf, reg)

    avs = fpga_lib.get_field(reg_val, bits)
    if avs not in table:
        raise VoltageMarginError(
            "%s 0x%02x=0x%08x -> AVS byte 0x%02x is not a known VDDC level for "
            "profile %r -- refusing (corrupt read)"
            % (fpga_name, reg, reg_val, avs, rail["avs_profile"]))
    mv = table[avs] * 1000.0
    _vlog(verbose, "  AVS byte 0x%02x -> %.1f mV baseline" % (avs, mv))
    return mv


# --------------------------------------------------------------------------- #
# SMBus access                                                                #
# --------------------------------------------------------------------------- #
class _Bus:
    """Thin SMBus wrapper. In dry-run it simulates reads and drops writes so
    the full flow can be exercised with no hardware access."""

    def __init__(self, busnum, dry_run, verbose):
        self._dry = dry_run
        self._v = verbose
        if dry_run:
            self._bus = None
        elif SMBus is None:
            raise VoltageMarginError("smbus2 is not available")
        else:
            self._bus = SMBus(busnum)

    def close(self):
        if self._bus is not None:
            self._bus.close()

    def read_byte(self, addr, reg):
        if self._dry:
            val = _DRYRUN_READS.get(reg, 0x00)
            _vlog(self._v, "  [dry-run] read byte 0x%02x -> 0x%02x" % (reg, val))
            return val
        val = self._bus.read_byte_data(addr, reg)
        _vlog(self._v, "  read byte 0x%02x -> 0x%02x" % (reg, val))
        return val

    def read_word(self, addr, reg):
        if self._dry:
            _vlog(self._v, "  [dry-run] read word 0x%02x -> 0x0000" % reg)
            return 0x0000
        val = self._bus.read_word_data(addr, reg)
        _vlog(self._v, "  read word 0x%02x -> 0x%04x" % (reg, val))
        return val

    def write_byte(self, addr, reg, val):
        if self._dry:
            _vlog(self._v, "  [dry-run] write byte 0x%02x <- 0x%02x" % (reg, val))
            return
        _vlog(self._v, "  write byte 0x%02x <- 0x%02x" % (reg, val))
        self._bus.write_byte_data(addr, reg, val)

    def write_word(self, addr, reg, val):
        if self._dry:
            _vlog(self._v, "  [dry-run] write word 0x%02x <- 0x%04x" % (reg, val))
            return
        _vlog(self._v, "  write word 0x%02x <- 0x%04x" % (reg, val))
        self._bus.write_word_data(addr, reg, val)


def _dev_id(bus, addr):
    return "%d-%04x" % (bus, addr)


def _unbind_device(bus, addr, dry_run, verbose):
    """Unbind one device from its kernel driver so the tool can drive it without
    racing the driver.  Returns the driver dir for _rebind, or None if it was not
    bound. Raises VoltageMarginError if it is bound but the unbind fails -- the
    caller must not drive a device whose driver is still attached. Note, unbinding
    does NOT power down the rail."""
    dev_id = _dev_id(bus, addr)
    link = "/sys/bus/i2c/devices/%s/driver" % dev_id
    if not os.path.islink(link):
        _vlog(verbose, "  %s not bound to a driver; nothing to unbind" % dev_id)
        return None
    drv = os.path.realpath(link)
    if dry_run:
        _vlog(verbose, "  [dry-run] would unbind %s from %s"
                       % (dev_id, os.path.basename(drv)))
        return drv
    try:
        with open(os.path.join(drv, "unbind"), "w") as f:
            f.write(dev_id)
    except OSError as e:
        raise VoltageMarginError(
            "failed to unbind %s from %s: %s -- refusing to drive it with the "
            "driver still attached" % (dev_id, os.path.basename(drv), e))
    _vlog(verbose, "  unbound %s from %s" % (dev_id, os.path.basename(drv)))
    return drv


def _rebind_device(bus, addr, drv, dry_run, verbose):
    """Rebind a previously-unbound device.  No-op if it was not bound."""
    if drv is None:
        return
    dev_id = _dev_id(bus, addr)
    bind = os.path.join(drv, "bind")
    if dry_run:
        _vlog(verbose, "  [dry-run] would rebind %s to %s"
                       % (dev_id, os.path.basename(drv)))
        return
    try:
        with open(bind, "w") as f:
            f.write(dev_id)
    except OSError as e:
        _log("  ! failed to rebind %s to %s: %s -- MANUAL RECOVERY: echo %s > %s"
             % (dev_id, os.path.basename(drv), e, dev_id, bind))
        return
    _vlog(verbose, "  rebound %s to %s" % (dev_id, os.path.basename(drv)))


def _volts_per_lsb(dev, addr):
    """Volts-per-LSB for the VOUT-family registers (Linear16). Raises if the decoded
    value is outside the physically-expected band (corrupt read)."""
    vout_mode = dev.read_byte(addr, PMBUS_VOUT_MODE)
    exponent = vout_mode & 0x1F
    if exponent & 0x10: # sign-extend the 5-bit exponent
        exponent -= 0x20
    vpl = 2.0 ** exponent
    if not (_VPL_MIN <= vpl <= _VPL_MAX):
        raise VoltageMarginError(
            "VOUT_MODE=0x%02x -> %.6f V/LSB outside expected band "
            "[%.4f, %.4f] mV/LSB -- corrupt read, refusing to compute a margin "
            "code" % (vout_mode, vpl, _VPL_MIN * 1000, _VPL_MAX * 1000))
    return vpl


# --------------------------------------------------------------------------- #
# Target computation                                                          #
# --------------------------------------------------------------------------- #
def compute_target_mv(name, rail, level, target_mv=None, offset_mv=None,
                      baseline_mv=None):
    """Absolute target (mV), clamped to the per-rail [min, max] window. Priority:
    `target_mv` (absolute) > `offset_mv` (baseline + offset, sign from level)
    > the per-rail configured absolute target for `level`."""
    if target_mv is not None:
        target = float(target_mv)
    elif offset_mv is not None:
        if baseline_mv is None:
            raise VoltageMarginError("offset margining needs an AVS baseline")
        mag = abs(offset_mv)
        target = baseline_mv + (-mag if level == LEVEL_LOW else mag)
    else:
        key = "margin_low_val_mv" if level == LEVEL_LOW else "margin_high_val_mv"
        cfg = rail.get(key)
        if cfg is None:
            raise VoltageMarginError(
                "rail %r has no %s configured -- refusing %r" % (name, key, level))
        target = float(cfg)
    lo = float(rail["margin_min_val_mv"])
    hi = float(rail["margin_max_val_mv"])
    if not (lo <= target <= hi):
        raise VoltageMarginError(
            "rail %r target %.1f mV is outside the allowed [%.1f, %.1f] mV "
            "window -- refusing" % (name, target, lo, hi))
    return target


# --------------------------------------------------------------------------- #
# Rail operations                                                             #
# --------------------------------------------------------------------------- #
def set_rail(name, rail, level, target_mv=None, offset_mv=None,
             dry_run=False, verbose=False):
    """Drive a rail to `level` (LEVEL_HIGH/LOW = margin, LEVEL_NOMINAL =
    restore). Unbinds the device, programs the PMBus margin/operation registers,
    polls READ_VOUT, auto-reverts a margin that doesn't settle, and rebinds."""
    if level not in LEVELS:
        raise VoltageMarginError("unknown level %r" % level)
    busnum = rail["i2c_bus"]
    addr = int(str(rail["i2c_addr"]), 16)
    page = rail.get("page", 0)

    if level == LEVEL_NOMINAL:
        # Restore clears margin-select only -- no target read, no margin code.
        target = None
        operation_value = OPERATION_NOMINAL
        margin_reg = None
        verb = "nominal"
    else:
        baseline_mv = None
        if offset_mv is not None:
            baseline_mv = read_avs_baseline_mv(rail, dry_run, verbose)
        target = compute_target_mv(name, rail, level, target_mv, offset_mv,
                                   baseline_mv)
        if level == LEVEL_HIGH:
            operation_value, margin_reg = OPERATION_MARGIN_HIGH, PMBUS_VOUT_MARGIN_HIGH
        else:
            operation_value, margin_reg = OPERATION_MARGIN_LOW, PMBUS_VOUT_MARGIN_LOW
        if baseline_mv is not None:
            verb = ("margin-%s %.1fmV (AVS %.1f %+0.1f)"
                    % (level, target, baseline_mv, target - baseline_mv))
        else:
            verb = "margin-%s %.1fmV" % (level, target)

    drv = _unbind_device(busnum, addr, dry_run, verbose)
    dev = _Bus(busnum, dry_run, verbose)
    try:
        if dev.read_byte(addr, PMBUS_PAGE) != page:
            dev.write_byte(addr, PMBUS_PAGE, page)

        vpl = _volts_per_lsb(dev, addr)
        margin_code = None
        if margin_reg is not None:
            margin_code = int(round((target / 1000.0) / vpl))
            dev.write_word(addr, margin_reg, margin_code)
        dev.write_byte(addr, PMBUS_OPERATION, operation_value)

        if dry_run:
            if margin_reg is None:
                _log("  [dry-run] %s (bus %d 0x%02x) -> %s: clear margin-select "
                     "(OPERATION=0x%02x)" % (name, busnum, addr, verb, operation_value))
            else:
                _log("  [dry-run] %s (bus %d 0x%02x) -> %s: code %d @ %.4f mV/LSB"
                     % (name, busnum, addr, verb, margin_code, vpl * 1000))
            return

        if target is None:
            raw = dev.read_word(addr, PMBUS_READ_VOUT)
            _log("  %s (bus %d 0x%02x) -> %s: %.1fmV"
                 % (name, busnum, addr, verb, raw * vpl * 1000))
            return

        actual_v = None
        target_v = target / 1000.0
        for _ in range(_SETTLE_POLLS):
            raw = dev.read_word(addr, PMBUS_READ_VOUT)
            actual_v = raw * vpl
            if abs(actual_v - target_v) <= _SETTLE_TOLERANCE_V:
                break
            time.sleep(0.1)

        settled = actual_v is not None and abs(actual_v - target_v) <= _SETTLE_TOLERANCE_V
        if settled:
            _log("  %s (bus %d 0x%02x) -> %s: %.1fmV (target %.1fmV)"
                 % (name, busnum, addr, verb, actual_v * 1000, target))
        else:
            last = ("%.1fmV" % (actual_v * 1000)) if actual_v is not None else "n/a"
            _log("  ! %s (bus %d 0x%02x) -> %s did NOT settle: last=%s target=%.1fmV"
                 % (name, busnum, addr, verb, last, target))
            _log("  ! reverting %s to NOMINAL for safety" % name)
            dev.write_byte(addr, PMBUS_OPERATION, OPERATION_NOMINAL)
            raise VoltageMarginError("%s did not settle; reverted to nominal" % name)
    finally:
        dev.close()
        _rebind_device(busnum, addr, drv, dry_run, verbose)


def read_rail(name, rail, dry_run=False, verbose=False):
    """Read-only: report a rail's OPERATION margin state and READ_VOUT voltage."""
    busnum = rail["i2c_bus"]
    addr = int(str(rail["i2c_addr"]), 16)
    page = rail.get("page", 0)
    state = {0: "NOMINAL", 1: "LOW", 2: "HIGH", 3: "RESERVED"}
    drv = _unbind_device(busnum, addr, dry_run, verbose)
    dev = _Bus(busnum, dry_run, verbose)
    try:
        avs_mv = (read_avs_baseline_mv(rail, dry_run, verbose)
                  if rail.get("avs_profile") is not None else None)
        if dev.read_byte(addr, PMBUS_PAGE) != page:
            dev.write_byte(addr, PMBUS_PAGE, page)
        operation = dev.read_byte(addr, PMBUS_OPERATION)
        vpl = _volts_per_lsb(dev, addr)
        if dry_run:
            avs_str = " (AVS %.1fmV)" % avs_mv if avs_mv is not None else ""
            _log("  [dry-run] %s: would read OPERATION + READ_VOUT%s"
                 % (name, avs_str))
            return
        raw = dev.read_word(addr, PMBUS_READ_VOUT)
        actual_mv = raw * vpl * 1000
        margin_bits = (operation >> 4) & 0x03
        if avs_mv is not None:
            dev_pct = (actual_mv - avs_mv) / avs_mv * 100.0
            _log("  %s (bus %d 0x%02x): %.1fmV (AVS %.1fmV, %+.2f%%), "
                 "OPERATION=0x%02x margin=%s"
                 % (name, busnum, addr, actual_mv, avs_mv, dev_pct, operation,
                    state[margin_bits]))
        else:
            _log("  %s (bus %d 0x%02x): %.1fmV, OPERATION=0x%02x margin=%s"
                 % (name, busnum, addr, actual_mv, operation, state[margin_bits]))
    finally:
        dev.close()
        _rebind_device(busnum, addr, drv, dry_run, verbose)
