# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""ADM1266 power management and blackbox data processing.

Provides functionality for reading and processing blackbox fault data from
ADM1266 power management devices, including fault record parsing, reboot
cause determination, and power loss analysis.
"""

import datetime
import json
import os
from typing import Any, Callable, Dict, List
from sonic_platform_base.chassis_base import ChassisBase


# Rendering and formatting helpers

def binw(val: int, width: int) -> str:
    """Format value as binary string with specified width.

    Args:
        val: Integer value to format
        width: Number of bits in output

    Returns:
        Binary string like "0b00001010"
    """
    return f"0b{val:0{width}b}"


def hex_value(_key: str, val: int) -> str:
    """Format value as hexadecimal string.

    Args:
        _key: Field name (unused)
        val: Integer value to format

    Returns:
        Hexadecimal string like "0x1a"
    """
    return f"0x{val:x}"


def default_render(_key: str, val: Any) -> Any:
    """Default renderer returning empty string for falsy values.

    Args:
        _key: Field name (unused)
        val: Value to render

    Returns:
        Original value or empty string if falsy
    """
    return val or ""


def time_since(_key: str, data: bytes) -> str:
    """Convert ADM1266 8-byte timestamp to human-readable elapsed time.

    Parses the ADM1266 blackbox timestamp format and converts to elapsed
    time since power-on with fractional seconds preserved.

    Args:
        _key: Field name (unused)
        data: 8-byte timestamp from ADM1266 blackbox

    Returns:
        Human-readable string like "3 minutes 2.518700 seconds after power-on"
        or empty string if data is invalid
    """
    if not isinstance(data, (bytes, bytearray)) or len(data) != 8:
        return ''
    if data in (b'\x00' * 8, b'\xff' * 8):
        return ''

    secs = int.from_bytes(data[2:6], 'little')
    frac = int.from_bytes(data[0:2], 'little') / 65536.0
    total_seconds = secs + frac

    minutes_total, seconds = divmod(total_seconds, 60)
    hours_total, minutes = divmod(minutes_total, 60)
    days_total, hours = divmod(hours_total, 24)
    years, days = divmod(days_total, 365)

    def fmt(value: float, name: str) -> str:
        if value == 0:
            return ''
        return f"{int(value)} {name}" + ('s' if int(value) != 1 else '')

    parts = [
        fmt(years, "year"),
        fmt(days, "day"),
        fmt(hours, "hour"),
        fmt(minutes, "minute")
    ]
    parts = [p for p in parts if p]  # remove empty
    parts.append(f"{seconds:.6f} second" + ('' if abs(seconds - 1.0) < 1e-6 else 's'))
    return " ".join(parts) + " after power-on"

# Channel naming configuration
CHANNEL_PREFIX: Dict[str, str] = {
    'vhx': 'VH',
    'vp_ov': 'VP',
    'vp_uv': 'VP',
    'gpio_in': 'GPIO',
    'gpio_out': 'GPIO',
    'pdio_in': 'PDIO',
    'pdio_out': 'PDIO',
}

CHANNEL_WIDTH: Dict[str, int] = {
    'vhx': 8,
    'vp_ov': 16,
    'vp_uv': 16,
    'gpio_in': 16,
    'gpio_out': 16,
    'pdio_in': 16,
    'pdio_out': 16,
}


def channel_names(key: str, value: int) -> str:
    """Convert channel bitmask to human-readable channel names.

    Args:
        key: Channel type (e.g., 'vhx', 'vp_ov', 'pdio_in')
        value: Bitmask representing active channels

    Returns:
        Comma-separated channel names with binary representation,
        e.g., "VH1,VH3 (0b00000101)"
    """
    prefix = CHANNEL_PREFIX.get(key)
    if prefix is None:
        return hex_value(key, value)

    names = []
    idx = 1
    mask = value
    while mask:
        if mask & 1:
            names.append(f"{prefix}{idx}")
        mask >>= 1
        idx += 1

    width = CHANNEL_WIDTH.get(key)
    if not names:
        return binw(value, width)
    return ','.join(names) + " (" + binw(value, width) + ")"


def raw_pretty(_key: str, data: bytes) -> str:
    """Format 64-byte blackbox record as hex dump.

    Args:
        _key: Field name (unused)
        data: 64-byte blackbox data

    Returns:
        Multi-line hex dump (8 rows of 8 bytes each)
    """
    rows = []
    for i in range(0, 64, 8):
        chunk = data[i:i+8]
        rows.append(' '.join(f"{b:02x}" for b in chunk))
    return '\n'.join(rows)

# Field renderer mapping
RENDER: Dict[str, Callable[[str, Any], str]] = {}

# Numeric fields rendered as hex
for prop in ['uid', 'powerup', 'action', 'rule', 'current', 'last',
             'vhx', 'vp_ov', 'vp_uv', 'gpio_in', 'gpio_out', 'pdio_in', 'pdio_out']:
    RENDER[prop] = hex_value

# Special renderers
RENDER['timestamp'] = time_since
RENDER['dpm_fault'] = lambda _k, v: v if isinstance(v, str) else hex_value(_k, v)
RENDER['power_fault_cause'] = lambda _k, v: v if isinstance(v, str) else hex_value(_k, v)
RENDER['raw'] = raw_pretty

# Channel fields use human-friendly names
for k in ['vhx', 'vp_ov', 'vp_uv', 'gpio_in', 'gpio_out', 'pdio_in', 'pdio_out']:
    RENDER[k] = channel_names

# Output field order for display
OUTPUT_ORDER = [
    'dpm_name', 'fault_uid', 'powerup', 'action', 'rule', 'power_loss', 'current', 'last',
    'vhx', 'vp_ov', 'vp_uv', 'gpio_in', 'gpio_out', 'pdio_in', 'pdio_out',
    'dpm_fault', 'power_fault_cause', 'timestamp', 'raw'
]


def find_voltage_faults(vx_bits: int, mapping: Dict[int, str]) -> str:
    """Find power rails that experienced (under/over) voltage faults.

    Args:
        vx_bits: Voltage fault bitmask
        mapping: Mapping from bit index to rail description

    Returns:
        Comma-separated list of affected rail names, or empty string if none
    """
    reason = ""
    for vx_idx, rail_desc in mapping.items():
        if (vx_bits >> (vx_idx - 1)) & 1:
            if reason:
                reason += ", "
            reason += rail_desc
    return reason


def determine_voltage_faults(vpx_to_rail_desc: Dict[int, str],
                             vhx_to_rail_desc: Dict[int, str],
                             vhx: int, vp_ov: int, vp_uv: int) -> Dict[str, str]:
    """Determine under/over voltage faults from VHx and VPx bits

    Analyzes VH (voltage high) and VP (voltage positive) fault bits to
    identify which power rails caused the fault.

    Args:
        vpx_to_rail_desc: VPx to rail mapping descriptions
        vhx_to_rail_desc: VHx to rail mapping descriptions
        vhx: VH fault bitmask
        vp_ov: VP overvoltage fault bitmask
        vp_uv: VP undervoltage fault bitmask

    Returns:
        Dictionary containing a list of under/over voltage faults
    """
    vhx_ov_faults = find_voltage_faults(vhx & 0x0F, vhx_to_rail_desc)
    vhx_uv_faults = find_voltage_faults(vhx & 0xF0, vhx_to_rail_desc)

    vpx_ov_faults = find_voltage_faults(vp_ov, vpx_to_rail_desc)
    vpx_uv_faults = find_voltage_faults(vp_uv, vpx_to_rail_desc)

    ov_faults = []
    if vhx_ov_faults:
        ov_faults.append(vhx_ov_faults)
    if vpx_ov_faults:
        ov_faults.append(vpx_ov_faults)

    faults = {}
    if ov_faults:
        faults['over_voltage_rails'] = ','.join(ov_faults)

    uv_faults = []
    if vhx_uv_faults:
        uv_faults.append(vhx_uv_faults)
    if vpx_uv_faults:
        uv_faults.append(vpx_uv_faults)

    if uv_faults:
        faults['under_voltage_rails'] = ','.join(uv_faults)
    return faults

def is_fault(pdio_in, pdio_mask, pdio_val, gpio_in, gpio_mask, gpio_val):
    return (gpio_in & gpio_mask) == gpio_val and (pdio_in & pdio_mask) == pdio_val

def decode_power_fault_cause(dpm_signal_to_fault_cause: List[Dict[str, Any]],
                             pdio_in: int, gpio_in: int) -> (str, str, str, str):
    """Decode power fault cause from PDIO and GPIO input bits.

    Args:
        dpm_signal_to_fault_cause: List of signal pattern to fault cause mappings
        pdio_in: PDIO input bitmask
        gpio_in: GPIO input bitmask

    Returns:
        Tuple of (hw_cause, hw_desc, summary, reboot_cause) as comma-separated strings
    """
    hw_cause = []
    hw_desc = []
    summary = []
    reboot_cause = []
    for cause_dict in dpm_signal_to_fault_cause:
        if is_fault(pdio_in, cause_dict["pdio_mask"], cause_dict["pdio_value"],
                    gpio_in, cause_dict["gpio_mask"], cause_dict["gpio_value"]):
            hw_cause.append(cause_dict.get("hw_cause"))
            hw_desc.append(cause_dict.get("hw_desc"))
            summary.append(cause_dict.get("summary"))
            reboot_cause.append(cause_dict.get("reboot_cause"))
    return ",".join(hw_cause), ",".join(hw_desc), ",".join(summary), ",".join(reboot_cause)

def reboot_cause_str_to_type(cause: str) -> str:
    """Return the cause specific type defined in ChassisBase.

    Args:
        cause: Reboot cause string, may be comma-separated for multiple causes

    Returns:
        ChassisBase reboot cause constant (takes first if comma-separated)
    """
    # Handle comma-separated causes by taking the first one
    if ',' in cause:
        cause = cause.split(',')[0].strip()

    try:
        return getattr(ChassisBase, cause)
    except:
        return ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER

def get_reboot_cause_type(reboot_causes):
    """
    Choose a suitable reboot cause amongst the available ones

    Args:
        reboot_causes: List of reboot causes recorded in DPMs

    Returns:
        Chosen reboot cause type
    """
    if not reboot_causes:
        return ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER
    # Pick the first switch_card DPM's cause to report the REBOOT_CAUSE_ value.
    # We ensure that there is no loss of information since we display all the
    # selected (by earliest record uid) causes of all DPMs in the summary section
    # right below the REBOOT_CAUSE_ value and more importantly display all the
    # faults recorded in the DPMS in a supplementary command line. So the choice
    # of a single cause from amongst many is to satisfy the API return value
    # expectation.
    reboot_cause = reboot_causes[0]
    return reboot_cause_str_to_type(reboot_cause)

class Adm1266:
    """
    ADM1266 Power Management Device Interface.

    This class provides methods to read blackbox data, parse fault records,
    and determine reboot causes from ADM1266 power management devices.
    """
    def __init__(self, platform_spec):
        self.platform_spec = platform_spec
        self.nvmem_path = self.platform_spec.get_nvmem_path()

    def get_name(self):
        """Get the DPM device name."""
        return self.platform_spec.get_name()

    def read_blackbox(self) -> bytes:
        """Read the entire blackbox data blob from the nvmem sysfs file"""
        with open(self.nvmem_path, 'rb') as file:
            return file.read()

    @staticmethod
    def _get_fault_record(data: bytes) -> Dict:
        """Parse a 64-byte record (ADM1266 Table 79)."""
        if len(data) != 64:
            return {}

        def u16(off: int) -> int:
            return data[off] | (data[off + 1] << 8)

        empty = data[2] & 0x01

        rec = {
            'uid': u16(0),
            'empty': empty,
            'action': data[3],
            'rule': data[4],
            'vhx': data[5],            # VHx_OV status byte
            'current': u16(6),        # current state
            'last': u16(8),           # last state
            'vp_ov': u16(10),
            'vp_uv': u16(12),
            'gpio_in': u16(14),
            'gpio_out': u16(16),
            'pdio_in': u16(18),
            'pdio_out': u16(20),
            'powerup': u16(22),
            'timestamp': data[24:32],  # 8 bytes
            'crc': data[63],
        }
        return rec

    @staticmethod
    def _parse_blackbox(data: bytes) -> List[Dict]:
        """Parse blackbox data and return structured list of valid (non-empty) faults.
            - Skips records that are all 0xFF (erased) or all 0x00 (empty area)
            - Uses the 'empty' bit parsed from byte 2 to keep only valid records
        """
        faults: List[Dict] = []
        if not data:
            return faults

        fault_size = 64
        num_records = len(data) // fault_size
        for i in range(num_records):
            start = i * fault_size
            rec = data[start:start + fault_size]
            # Skip cleared and erased records
            if all(b == 0x00 for b in rec) or all(b == 0xFF for b in rec):
                continue
            fault_record = Adm1266._get_fault_record(rec)
            if (fault_record['empty'] & 0x01) == 0:
                fault_record['record_index'] = i
                faults.append(fault_record)
        return faults

    def get_blackbox_records(self) -> List[Dict]:
        """Get reboot causes from blackbox faults read via sysfs."""
        blackbox_data = self.read_blackbox()
        faults = Adm1266._parse_blackbox(blackbox_data)

        records: List[Dict] = []
        for fault in faults:
            rec_idx = fault.get('record_index')
            record = {
                'fault_uid': fault['uid'],
                'gpio_in': fault['gpio_in'],
                'gpio_out': fault['gpio_out'],
                'powerup': fault['powerup'],
                'timestamp': fault['timestamp'],
                'action': fault['action'],
                'rule': fault['rule'],
                'vhx': fault['vhx'],
                'vp_ov': fault['vp_ov'],
                'vp_uv': fault['vp_uv'],
                'current': fault['current'],
                'last': fault['last'],
                'pdio_in': fault['pdio_in'],
                'pdio_out': fault['pdio_out'],
                'record_index': rec_idx,
            }
            # Attach raw 64-byte chunk for this record if index is known
            if isinstance(rec_idx, int) and rec_idx >= 0:
                start = rec_idx * 64
                record['raw'] = blackbox_data[start:start+64]

            # Decode Power Fault Cause bits (from pdio_in and gpio_in)
            hw_cause, hw_desc, summary, reboot_cause = \
                decode_power_fault_cause(self.platform_spec.get_dpm_signal_to_fault_cause(),
                                         record['pdio_in'],
                                         record['gpio_in'])
            if hw_cause:
                record['power_fault_cause'] = hw_cause + ' (' + hw_desc + ')'
                record['summary'] = summary
                record['hw_cause'] = hw_cause
                record['reboot_cause'] = reboot_cause
            record['dpm_name'] = self.platform_spec.get_name()
            records.append(record)
        return records

    def get_all_faults(self) -> List[Dict]:
        """
        Return all faults recorded in the DPM

        Returns:
            List of dictionaries containing fault information
        """
        records = self.get_blackbox_records()
        # Compute a terse power-loss reason (if any) and attach it
        for rec in records:
            faults = determine_voltage_faults(self.platform_spec.get_vpx_to_rail_desc(),
                                             self.platform_spec.get_vhx_to_rail_desc(),
                                             rec.get('vhx', 0),
                                             rec.get('vp_ov', 0),
                                             rec.get('vp_uv', 0))
            if faults:
                rec.update(faults)
        return records

    def clear_blackbox(self):
        """Clear the blackbox data by writing to the nvmem path."""
        with open(self.nvmem_path, 'wb') as file:
            file.write(b"1")

class Adm1266Display:
    """
    Display formatter for ADM1266 fault records.

    This class provides methods to format and render fault records from ADM1266
    devices into human-readable messages for debugging and analysis.
    """
    MSG_ORDER = [
        'dpm_name', 'fault_uid', 'power_loss', 'dpm_fault', 'power_fault_cause',
        'powerup', 'timestamp', 'current', 'last', 'action',
        'rule', 'vhx', 'vp_ov', 'vp_uv',
        'gpio_in', 'gpio_out', 'pdio_in', 'pdio_out',
        'raw',
    ]
    def __init__(self, faults):
        self.faults = faults

    @staticmethod
    def _format_fault(fault, is_first_message):
        """Format a single fault into a printable message."""
        rendered_items = []
        for key in Adm1266Display.MSG_ORDER:
            val = fault.get(key)
            if val is None:
                continue
            if isinstance(val, str) and val in ('', '0x0'):
                continue
            rendered_items.append((key, val))

        if not rendered_items:
            return ""

        max_key = max(len(k) for k, _ in rendered_items)
        lines: List[str] = []
        for key, val in rendered_items:
            prefix = f"  {key.ljust(max_key)} = "
            if isinstance(val, str) and ('\n' in val):
                indented = val.replace('\n', '\n' + ' ' * len(prefix))
                lines.append(prefix + indented)
            else:
                lines.append(prefix + str(val))

        if not is_first_message:
            # Prepend a newline to the first attribute so each block starts on a new line
            lines[0] = "\n" + lines[0]
        return "\n".join(lines)

    @staticmethod
    def _render_faults(faults) -> List[Dict]:
        """
        Get formatted DPM fault records from blackbox data.

        Returns:
            List of dictionaries containing formatted fault information.

            Example output:
            [
                {
                    'dpm_name': 'cpu_card',
                    'fault_uid': '0x1a3f',
                    'powerup': '0x1',
                    'action': '0x0',
                    'rule': '0x0',
                    'power_loss': 'POS5V0_S0, POS3V3_S5',
                    'current': '0x0',
                    'last': '0x0',
                    'vhx': 'VH1,VH3 (0b00000101)',
                    'vp_ov': '0b0000000000000000',
                    'vp_uv': 'VP1,VP5 (0b0000000000010001)',
                    'gpio_in': '0b0000000000000000',
                    'gpio_out': '0b0000000000000000',
                    'pdio_in': 'PDIO1,PDIO8 (0b0000000010000001)',
                    'pdio_out': '0b0000000000000000',
                    'dpm_fault': 'VH fault',
                    'power_fault_cause': 'PSU input power lost',
                    'timestamp': '3 minutes 2.518700 seconds after power-on',
                    'raw': '1a 3f 01 00 00 00 05 00\n...'
                }
            ]
        """
        rendered: List[Dict] = []
        for fault in faults:
            out: Dict[str, str] = {}
            for key in OUTPUT_ORDER:
                renderer = RENDER.get(key, default_render)
                out[key] = renderer(key, fault.get(key, 0))
            rendered.append(out)
        return rendered

    def render(self, is_first=False):
        """ Render all faults recorded in a DPM into a printable message."""
        messages: List[str] = []
        rendered_faults = Adm1266Display._render_faults(self.faults)
        for fault in rendered_faults:
            message = Adm1266Display._format_fault(fault, is_first)
            messages.append(message)
        return messages

def render_all_faults(all_faults):
    """
    Render all faults from multiple DPMs into formatted messages.

    Args:
        all_faults: Dictionary mapping DPM names to lists of fault records

    Returns:
        List of formatted message strings for all faults
    """
    messages = []
    for faults in all_faults.values():
        dpm_display = Adm1266Display(faults)
        dpm_messages = dpm_display.render(not messages)  # Use implicit boolean check
        messages += dpm_messages
    return messages

def get_all_faults(pddf_plugin_path=None) -> List[Dict]:
    """
    Get all faults recorded in the DPMs.

    Args:
        pddf_plugin_path: Optional path to PDDF plugin file. If None, uses default system path.

    Returns:
        List of dictionaries containing faults of each DPM.
        Among each DPM, the list of faults are sorted by UID (earlier fault comes first).
    """
    from sonic_platform.adm1266_platform_spec import Adm1266PlatformSpec  # pylint: disable=import-outside-toplevel

    if pddf_plugin_path is None:
        pddf_plugin_path = '/usr/share/sonic/platform/pddf/pd-plugin.json'
    with open(pddf_plugin_path, encoding='utf-8') as pddf_file:
        pddf_plugin_data = json.load(pddf_file)

    all_faults = []
    adms = pddf_plugin_data.get("DPM", {})
    for adm in adms:
        platform_spec = Adm1266PlatformSpec(adm, pddf_plugin_data)
        dev = Adm1266(platform_spec)
        faults = dev.get_all_faults()
        if faults:
            dev.clear_blackbox()
            faults = sorted(faults, key=lambda f: f['fault_uid'])
            for fault in faults:
                fault['dpm_name'] = dev.get_name()
            all_faults.extend(faults)
    return all_faults

def get_reboot_cause(pddf_plugin_path=None) -> tuple[str, str] | None:
    """
    Get system reboot cause by analyzing ADM1266 blackbox data.

    Args:
        pddf_plugin_path: Optional path to PDDF plugin file. If None, uses default system path.

    Returns:
        Tuple of (reboot_cause, debug_message) where reboot_cause is a
        ChassisBase.REBOOT_CAUSE_* constant and debug_message contains detailed
        fault information.
    """
    from sonic_platform.dpm import SystemDPMLogHistory  # pylint: disable=import-outside-toplevel

    all_faults = get_all_faults(pddf_plugin_path)
    if all_faults:
        # Save all_faults history via SystemDPMLogHistory
        SystemDPMLogHistory().save('adm1266', all_faults)
        # We intentionally do not include rendered fault messages here.
        # Return the initial reboot cause and the collected summaries. The per-DPM
        # records have been saved to history files which can be loaded and
        # rendered separately if more details are needed.

        # Extract causes and summaries from all_faults (which is a list)
        causes = []
        summaries = []
        for fault in all_faults:
            if 'reboot_cause' in fault:
                causes.append(fault['reboot_cause'])
            if 'summary' in fault:
                summaries.append(fault['summary'])

        reboot_cause = get_reboot_cause_type(causes)
        # summaries is a list of strings; coalesce to single debug message
        return reboot_cause, ", ".join(summaries)

    return None
