# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0


import json
from typing import List, Dict, Callable, Any


# Rendering helpers (no hardware-width assumptions)

def binw(val, width):
    return f"0b{val:0{width}b}"

def hex_value(_key: str, val: int) -> str:
    return f"0x{val:x}"

def time_since(_key: str, data: bytes) -> str:
    """
    Convert an 8-byte ADM1266 blackbox timestamp into a human-readable
    elapsed time since power-on, preserving fractional seconds.

    :param data: 8-byte timestamp from ADM1266
    :return: Human-readable string like "0 Day(s) 0 Hour(s) 3 Minute(s) 2.518700 Second(s)"
    """
    if not isinstance(data, (bytes, bytearray)) or len(data) != 8:
        return ''
    if data == b'\x00' * 8 or data == b'\xff' * 8:
        return ''

    # Extract integer seconds from bytes [2:6] (little-endian)
    secs = int.from_bytes(data[2:6], 'little')

    # Extract fractional seconds from bytes [0:2] (16-bit fraction)
    frac = int.from_bytes(data[0:2], 'little') / 65536.0

    total_seconds = secs + frac

    # Compute years, days, hours, minutes, seconds
    minutes_total, seconds = divmod(total_seconds, 60)
    hours_total, minutes = divmod(minutes_total, 60)
    days_total, hours = divmod(hours_total, 24)
    years, days = divmod(days_total, 365)

    # Helper to format singular/plural
    def fmt(value, name):
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

# Channel naming: derive names from bit positions; use per-key prefix (no fixed widths)
CHANNEL_PREFIX: Dict[str, str] = {
    'vhx': 'VH',
    'vp_ov': 'VP', 'vp_uv': 'VP',
    'gpio_in': 'GPIO', 'gpio_out': 'GPIO',
    'pdio_in': 'PDIO', 'pdio_out': 'PDIO',
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
    if not len(names):
        return binw(value, width)
    return ','.join(names) + " (" + binw(value, width) + ")"

# Renderer map: defaults to hex; override specific keys
RENDER: Dict[str, Callable[[str, int], str]] = {}
for k in ['uid','powerup','action','rule','current','last',
          'vhx','vp_ov','vp_uv','gpio_in','gpio_out','pdio_in','pdio_out']:
    RENDER[k] = hex_value
RENDER['timestamp'] = time_since

# dpm_fault and power_fault_cause are string labels; render as-is when present
RENDER['dpm_fault'] = lambda _k, v: v if isinstance(v, str) else hex_value(_k, v)
RENDER['power_fault_cause'] = lambda _k, v: v if isinstance(v, str) else hex_value(_k, v)
RENDER['power_fault_cause'] = lambda _k, v: v if isinstance(v, str) else hex_value(_k, v)

# Channel keys use channel_names (more human friendly); keep raw hex available via get_blackbox_records
for k in ['vhx','vp_ov','vp_uv','gpio_in','gpio_out','pdio_in','pdio_out']:
    RENDER[k] = channel_names

# Fixed output key order for human-friendly view
OUTPUT_ORDER = [
    'fault_uid','powerup','action','rule','current','last',
    'vhx','vp_ov','vp_uv','gpio_in','gpio_out','pdio_in','pdio_out',
    'dpm_fault','power_fault_cause','timestamp'
]

def find_power_loss(input_bits: int, output_bits: int,
                    mapping: Dict[int, Dict[str, Any]]) -> str:
    """
    Return a terse rail name (e.g., "POS5V0_S0") if any input bit AND its mapped
    output bit are both 1. Otherwise return an empty string.
    All bit indices are zero-based.
    """
    reason = ""
    for in_idx, pdio_desc in mapping.items():
        out_idx = pdio_desc.get("pdio")
        desc = pdio_desc.get("rail")
        if ((input_bits >> in_idx) & 1) and ((output_bits >> out_idx) & 1):
            if reason:
                reason += ", "
            reason += desc
    return reason

def determine_power_loss(vp_to_pdio_desc: Dict[int, Dict[str, Any]],
                         vh_to_pdio_desc: Dict[int, Dict[str, Any]],
                         vh: int, vp_uv: int, pdio_out: int) -> str:
    reason = find_power_loss(vh, pdio_out, vh_to_pdio_desc)
    if reason:
        return reason
    return find_power_loss(vp_uv, pdio_out, vp_to_pdio_desc)

def decode_dpm_fault(dpm_table: Dict[int, str], dpm_bits: Dict[int, int],
                     pdio_in: int) -> str:
    code = 0
    for pdio_bit, fault_pos in dpm_bits.items():
        code |= ((pdio_in >> pdio_bit) & 1) << fault_pos
    return dpm_table.get(code, f"code={code}") if code != 0 else ""  # empty string for zero

def decode_power_fault_cause(power_fault_cause:  Dict[int, Dict[str, str]], pdio_in: int) -> str:
    """Return the first asserted power fault cause description based on pdio_in."""
    # Iterate in the table's order (dict preserves insertion order in Python 3.7+)
    for idx, cause_dict in power_fault_cause.items():
        if ((pdio_in >> idx) & 1):
            return cause_dict.get("cause") + " (" + cause_dict.get("desc") + ")"
    return ""

class Adm1266:
    def __init__(self, dpm_info):
        self.dpm_info = dpm_info
        self.nvmem_path = self.dpm_info.get_nvmem_path()

    def read_blackbox(self) -> bytes:
        """Read the entire blackbox data blob from the nvmem sysfs file"""
        with open(self.nvmem_path, 'rb') as f:
            return f.read()

    def get_fault_record(self, data: bytes) -> Dict:
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

    def parse_blackbox(self, data: bytes) -> List[Dict]:
        """Parse blackbox data and return structured list of valid (non-empty) faults.

        - Skips records that are all 0xFF (erased) or all 0x00 (empty area)
        - Uses the 'empty' bit parsed from byte 2 to keep only valid records
        """
        faults: List[Dict] = []
        if not len(data):
            return faults

        fault_size = 64
        num_records = len(data) // fault_size
        for i in range(num_records):
            start = i * fault_size
            rec = data[start:start + fault_size]
            # Skip cleared and erased records
            if all(b == 0x00 for b in rec) or all(b == 0xFF for b in rec):
                continue
            fault_record = self.get_fault_record(rec)
            if (fault_record['empty'] & 0x01) == 0:
                fault_record['record_index'] = i
                faults.append(fault_record)
        return faults

    def get_blackbox_records(self) -> List[Dict]:
        """Get reboot causes from blackbox faults read via sysfs."""
        blackbox_data = self.read_blackbox()
        faults = self.parse_blackbox(blackbox_data)

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
            # Decode DPM fault (from pdio_in) and add a terse label
            dpm = decode_dpm_fault(self.dpm_info.get_dpm_table(),
                                   self.dpm_info.get_dpm_signals(),
                                   record['pdio_in'])
            if dpm:
                record['dpm_fault'] = dpm
            # Decode Power Fault Cause bits (from pdio_in) and add the first matching description
            pf = decode_power_fault_cause(self.dpm_info.get_power_fault_cause(), record['pdio_in'])
            if pf:
                record['power_fault_cause'] = pf
            record['dpm_name'] = self.dpm_info.get_name()
            records.append(record)
        return records

    def get_reboot_causes(self) -> List[Dict]:
        records = self.get_blackbox_records()

        rendered: List[Dict] = []
        for rec in records:
            out: Dict[str, str] = {}
            out['dpm_name'] = rec.get('dpm_name', '')
            for key in OUTPUT_ORDER:
                renderer = RENDER.get(key, hex_value)
                out[key] = renderer(key, rec.get(key, 0))
            # Compute a terse power-loss reason (if any) and attach it
            ploss = determine_power_loss(self.dpm_info.get_vp_to_pdio_desc(),
                                         self.dpm_info.get_vh_to_pdio_desc(),
                                         rec.get('vhx', 0),
                                         rec.get('vp_uv', 0),
                                         rec.get('pdio_out', 0))
            if ploss:
                out['power_loss'] = ploss
            # Add raw 64-byte record as 8 rows of 8 bytes each (hex)
            raw = rec.get('raw')
            if isinstance(raw, (bytes, bytearray)) and len(raw) == 64:
                rows = []
                for i in range(0, 64, 8):
                    chunk = raw[i:i+8]
                    rows.append(' '.join(f"{b:02x}" for b in chunk))
                out['raw'] = '\n'.join(rows)
            rendered.append(out)
        return rendered

    def get_reboot_cause(self):
        """Return a single string each reboot cause separated by a newline.
        It preserves the order from the driver (newest -> oldest).
        """
        causes = self.get_reboot_causes()
        msg_order = [
            'dpm_name', 'fault_uid', 'power_loss', 'dpm_fault', 'power_fault_cause',
            'powerup', 'timestamp', 'current', 'last', 'action',
            'rule', 'vhx', 'vp_ov', 'vp_uv',
            'gpio_in', 'gpio_out', 'pdio_in', 'pdio_out',
            'raw',
        ]

        messages: List[str] = []
        for c in causes:
            rendered_items = []
            for k in msg_order:
                v = c.get(k)
                if v is None:
                    continue
                if isinstance(v, str) and (v == '' or v == '0x0'):
                    continue
                rendered_items.append((k, v))
            if not rendered_items:
                messages.append("")
                continue
            max_key = max(len(k) for k, _ in rendered_items)
            lines: List[str] = []
            for k, v in rendered_items:
                prefix = f"  {k.ljust(max_key)} = "
                if isinstance(v, str) and ('\n' in v):
                    indented = v.replace('\n', '\n' + ' ' * len(prefix))
                    lines.append(prefix + indented)
                else:
                    lines.append(prefix + str(v))
            # Prepend a newline to the first attribute so each block starts on a new line
            lines[0] = "\n" + lines[0]
            messages.append("\n".join(lines))
        if not messages:
            return ""
        return "\n".join(messages)

    def clear_blackbox(self):
        with open(self.nvmem_path, 'w') as f:
            f.write("1")

def get_reboot_cause():
    messages = []
    from sonic_platform.dpm_info import DpmInfo
    with open('/usr/share/sonic/platform/pddf/pd-plugin.json') as pd:
            pddf_plugin_data = json.load(pd)

    adms = pddf_plugin_data.get("DPM", {})
    for adm in adms:
        dpm_info = DpmInfo(adm, pddf_plugin_data)
        adm = Adm1266(dpm_info)
        message = adm.get_reboot_cause()
        if message:
            adm.clear_blackbox()
            messages.append(message)
    if len(messages):
        return "REBOOT_CAUSE_HARDWARE_OTHER", "\n".join(messages)
    return "REBOOT_CAUSE_UNKNOWN", ""
