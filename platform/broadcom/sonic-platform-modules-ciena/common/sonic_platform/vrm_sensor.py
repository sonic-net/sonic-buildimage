#!/usr/bin/env python

#############################################################################
# Ciena VRM Voltage Sensor via FPGA PMBus VI_MON registers
#
# The Europa FPGA autonomously polls PMBus VRM controllers and stores
# the results in shadow registers accessible via PCI BAR0:
#
# PMBUS1 VI_MON_DATA (0x0638 - 0x0660) — 11 × 32-bit registers
# ---------------------------------------------------------------
# Monitors 4 PMBus VRM devices, each with a block of registers:
#
#   Q2C_CORE_V (J2C core voltage regulator):
#     DATA0[15:0]  = 0x4A IOUT_OC_WARN_LIMIT
#     DATA0[31:16] = 0x89 READ_IIN
#     DATA1[15:0]  = 0x88 READ_VIN
#     DATA1[31:16] = 0x8B READ_VOUT
#     DATA2[15:0]  = 0x8C READ_IOUT
#     DATA2[31:16] = 0x8D READ_TEMPERATURE_1
#
#   Q2C_TRVDD (J2C transceiver VDD):
#     DATA3[15:0]  = 0x4A IOUT_OC_WARN_LIMIT
#     DATA3[31:16] = 0xAD IC_DEVICE_ID
#     DATA4[15:0]  = 0x8B READ_VOUT
#     DATA4[31:16] = 0x8C READ_IOUT
#     DATA5[15:0]  = 0x8D READ_TEMPERATURE_1
#
#   3.3_SFP (3.3V SFP power rail):
#     DATA5[31:16] = 0x4A IOUT_OC_WARN_LIMIT
#     DATA6[15:0]  = 0x88 READ_VIN
#     DATA6[31:16] = 0x8B READ_VOUT
#     DATA7[15:0]  = 0x8C READ_IOUT
#     DATA7[31:16] = 0x8D READ_TEMPERATURE_1
#
#   3.3_QSFP (3.3V QSFP power rail):
#     DATA8[15:0]  = 0x4A IOUT_OC_WARN_LIMIT
#     DATA8[31:16] = 0x88 READ_VIN
#     DATA9[15:0]  = 0x8B READ_VOUT
#     DATA9[31:16] = 0x8C READ_IOUT
#     DATA10[15:0] = 0x8D READ_TEMPERATURE_1
#     DATA10[31:16]= Unused
#
# PMBUS2 VI_MON_DATA (0x06CC - 0x06D0) — 2 × 32-bit registers
# ---------------------------------------------------------------
# Monitors 1 PMBus VRM device:
#
#   Intel CPU VCCIN POL:
#     DATA0[15:0]  = 0x88 READ_VIN
#     DATA0[31:16] = 0x8B READ_VOUT
#     DATA1[15:0]  = 0x8C READ_IOUT
#     DATA1[31:16] = 0x8D READ_TEMPERATURE_1
#
# PMBus encoding:
#   READ_VIN, READ_IOUT, READ_TEMP1, etc. use LINEAR11:
#     bits[15:11] = signed 5-bit exponent
#     bits[10:0]  = signed 11-bit mantissa
#     Value = mantissa × 2^exponent
#
#   READ_VOUT uses LINEAR16 (unsigned 16-bit mantissa):
#     Value = mantissa × 2^N   where N comes from VOUT_MODE
#     The exponent N varies per VRM device (determined empirically):
#       Q2C_CORE_V : N = -7  (gives ~1.02 V for raw 0x0083)
#       Q2C_TRVDD  : N = -9  (gives ~0.84 V for raw 0x01AC)
#       3.3_SFP    : N = -9  (gives ~3.31 V for raw 0x069D)
#       3.3_QSFP   : N = -9  (gives ~3.36 V for raw 0x06B6)
#       CPU_VCCIN  : N = -7  (gives ~1.02 V for raw 0x0082)
#
#############################################################################

import logging
import json
import struct
from pathlib import Path

try:
    from ciena.fpga_lib import find_pci_devices, read_32
except ImportError as e:
    raise ImportError(str(e) + " - ciena.fpga_lib not found")

logger = logging.getLogger(__name__)

_vrm_sensor_dev_attr_override = None


def configure_vrm_sensor_dev_attr(dev_attr):
    """Set VRM_SENSOR dev_attr from caller-provided PDDF data.

    This lets consumers (e.g., voltage_sensor) pass already-loaded config
    and avoids re-reading pddf-device.json from disk.
    """
    global _vrm_sensor_dev_attr_override
    _vrm_sensor_dev_attr_override = dev_attr if isinstance(dev_attr, dict) else None


def _get_vrm_sensor_dev_attr(attr_name):
    """Get VRM_SENSOR.dev_attr attribute value from PDDF JSON."""
    if isinstance(_vrm_sensor_dev_attr_override, dict):
        return _vrm_sensor_dev_attr_override.get(attr_name)
    return None

def _get_vrm_sensor_int_attr(attr_name):
    """Get integer VRM_SENSOR.dev_attr value (supports hex strings)."""
    value = _get_vrm_sensor_dev_attr(attr_name)
    if value is None:
        return None
    try:
        return int(str(value), 0)
    except (ValueError, TypeError):
        return None


def _get_vrm_sensor_str_attr(attr_name):
    """Get string VRM_SENSOR.dev_attr value."""
    value = _get_vrm_sensor_dev_attr(attr_name)
    if value is None:
        return None
    return str(value)


def get_regmap_read_path():
    """Return effective regmap read path from JSON override or default constant."""
    path = _get_vrm_sensor_str_attr("REGMAP_READ")
    if path is not None:
        return path
    return None


def _get_vout_exponent(device_name):
    """Get the VOUT_MODE exponent for a device from JSON, or fall back to hardcoded constant."""
    exponents = _get_vrm_sensor_dev_attr("VOUT_EXPONENTS")
    if isinstance(exponents, dict) and device_name in exponents:
        try:
            return int(exponents[device_name])
        except (ValueError, TypeError):
            pass
    return None


def _get_vrm_devices():
    """Return the VRM_DEVICES table from JSON override, or fall back to the hardcoded constant."""
    raw = _get_vrm_sensor_dev_attr("VRM_DEVICES")
    if not isinstance(raw, dict):
        return None

    result = {}
    for device_name, device_cfg in raw.items():
        try:
            pmbus_index = int(device_cfg.get("pmbus_index", 1))
            base_offset = int(str(device_cfg.get("base_offset", "0")), 0)
            fields_raw = device_cfg.get("fields", {})
            field_map = {
                fname: (int(fval[0]), str(fval[1]))
                for fname, fval in fields_raw.items()
                if isinstance(fval, (list, tuple)) and len(fval) == 2
            }
            result[device_name] = (pmbus_index, base_offset, field_map)
        except Exception:
            continue
    return result if result else None


def _detect_fpga_pci_address():
    """Auto-detect Europa FPGA PCI address from sysfs."""
    vendor_id = _get_vrm_sensor_int_attr("FPGA_PCI_VENDOR_ID")
    device_id = _get_vrm_sensor_int_attr("FPGA_PCI_DEVICE_ID")
    if vendor_id is None or device_id is None:
        return None
    try:
        addrs = find_pci_devices(vendor_id, device_id)
        if addrs:
            return addrs[0]
    except Exception as e:
        logger.warning("FPGA PCI detection failed: %s", e)
    return None


def _read_fpga_reg(pci_address, offset):
    """Read a 32-bit FPGA register via PCI BAR0 mmap.

    Falls back to regmap-sysfs if PCI address is not available.
    Returns int or None.
    """
    if pci_address is not None:
        try:
            return read_32(pci_address, offset)
        except Exception as e:
            logger.debug("PCI read at 0x%04X failed: %s", offset, e)

    # Fallback: regmap-sysfs
    regmap_path = get_regmap_read_path()
    if regmap_path is None:
        return None
    try:
        with open(regmap_path, "rb") as f:
            f.seek(offset)
            data = f.read(4)
            if len(data) == 4:
                return struct.unpack('<I', data)[0]
    except OSError as e:
        logger.debug("regmap read at 0x%04X failed: %s", offset, e)
    return None


def _linear11_to_float(raw16):
    """Decode a PMBus LINEAR11 value (5-bit exponent + 11-bit mantissa).

    Format: bits[15:11] = signed exponent, bits[10:0] = signed mantissa.
    Result = mantissa × 2^exponent

    Returns float, or 0.0 if invalid.
    """
    if raw16 == 0 or raw16 == 0xFFFF:
        return 0.0

    exponent = (raw16 >> 11) & 0x1F
    mantissa = raw16 & 0x07FF

    # Sign-extend exponent (5 bits)
    if exponent >= 16:
        exponent -= 32

    # Sign-extend mantissa (11 bits)
    if mantissa >= 1024:
        mantissa -= 2048

    return float(mantissa) * (2.0 ** exponent)


def _linear16_to_float(raw16, vout_exp):
    """Decode a PMBus LINEAR16 VOUT value (unsigned 16-bit mantissa).

    Format: the full 16-bit word is an unsigned mantissa.
    The exponent comes from the device's VOUT_MODE register (not in the data).

    Result = mantissa × 2^vout_exp

    Args:
        raw16    (int): Unsigned 16-bit mantissa from READ_VOUT.
        vout_exp (int): Signed exponent from VOUT_MODE (e.g. -9, -7).

    Returns float, or 0.0 if invalid.
    """
    if raw16 == 0 or raw16 == 0xFFFF:
        return 0.0
    return float(raw16) * (2.0 ** vout_exp)

def read_vrm_field(pci_address, device_name, field_name):
    """Read a single PMBus field from a VRM device via FPGA VI_MON registers.

    Args:
        pci_address (str): PCI BDF string or None
        device_name (str): VRM device name (key in VRM_DEVICES)
        field_name  (str): Field name (e.g. 'READ_VOUT', 'READ_IOUT')

    Returns:
        float: Decoded value (volts, amps, or °C), or None on failure.
    """
    vrm_devices = _get_vrm_devices()
    if vrm_devices is None or device_name not in vrm_devices:
        return None

    _pmbus_idx, base_offset, field_map = vrm_devices[device_name]
    if field_name not in field_map:
        return None

    reg_index, half = field_map[field_name]
    offset = base_offset + reg_index * 4

    raw32 = _read_fpga_reg(pci_address, offset)
    if raw32 is None:
        return None

    if half == 'hi':
        raw16 = (raw32 >> 16) & 0xFFFF
    else:
        raw16 = raw32 & 0xFFFF

    # READ_VOUT uses LINEAR16 with per-device exponent;
    # all other fields use LINEAR11.
    if field_name == 'READ_VOUT':
        vout_exp = _get_vout_exponent(device_name)
        if vout_exp is None:
            return None
        return _linear16_to_float(raw16, vout_exp)
    else:
        return _linear11_to_float(raw16)


# Shared PCI address (lazy-init, cached)
_cached_pci_address = None
_pci_address_resolved = False


def _get_pci_address():
    """Get the cached FPGA PCI address."""
    global _cached_pci_address, _pci_address_resolved
    if not _pci_address_resolved:
        _cached_pci_address = _detect_fpga_pci_address()
        _pci_address_resolved = True
        if _cached_pci_address is None:
            logger.error("Europa FPGA not found — VRM sensor data unavailable")
    return _cached_pci_address
