#!/usr/bin/env python3
"""generate_plreg - Build a standalone plreg tool from a CFPGA regmap HTML file.

Single-step tool that parses a Ciena CFPGA register map HTML and generates
a self-contained plreg CLI utility with embedded register database.

Works with any Ciena CFPGA (Europa, Rudra40, etc.).

Usage:
    ./generate_plreg.py [--device-id <hex>] <regmap_html> [output_path]

Options:
    --device-id <hex>   PCI device ID for the FPGA (e.g. 0x032c for Europa,
                        0x033b for Rudra40). Optional. Used as fallback for
                        /dev/mem access when regmap sysfs is unavailable.

Examples:
    ./generate_plreg.py rudra40_regmap.html ./plreg
    ./generate_plreg.py --device-id 0x033b rudra40_regmap.html ./plreg

The FPGA name is derived from the HTML filename (e.g. "rudra40_regmap.html" -> "RUDRA40").
The generated plreg tool uses the kernel regmap sysfs interface
(/sys/bus/platform/devices/regmap-<fpga>.0/) for register access. This works
under kernel lockdown/secure boot without /dev/mem access. Falls back to
/dev/mem if regmap sysfs is not available and --device-id was provided.
"""

import re
import sys
import os
import stat


# ============================================================================
# REGISTER EXTRACTION
# ============================================================================

# Parsing constants
FIELD_SEARCH_WINDOW = 5000      # max chars to scan for fields after a register header
MIN_REGISTER_STRIDE = 4         # minimum byte stride between registers (32-bit aligned)
DEFAULT_ARRAY_STRIDE = 4        # default stride when array has only 1 element
REGISTER_WIDTH_BITS = 32        # register width in bits
REGISTER_WIDTH_BYTES = 4        # register width in bytes

def parse_regmap_html(html_file):
    """Parse an FPGA regmap HTML file and extract register definitions.

    Returns:
        registers: dict mapping register name -> {offset, fields}
        array_registers: dict mapping template name -> (base_offset, count, stride)
    """
    registers = {}
    array_registers = {}

    with open(html_file, 'r') as f:
        content = f.read()

    # ---- Step 1: Extract array register metadata ----
    # Pattern: "0xBASE - 0xEND</td>...Array of registers: PREFIX0 - PREFIXN"
    array_pattern = (
        r'<td class=r>(0x[0-9a-fA-F]+)\s*-\s*(0x[0-9a-fA-F]+)</td>'
        r'.{0,500}?'
        r'Array of registers:\s*(\w+\D)(\d+)\s*-\s*\w+\D(\d+)'
    )
    for m in re.finditer(array_pattern, content, re.DOTALL):
        base_hex = m.group(1)
        end_hex = m.group(2)
        prefix = m.group(3)
        start_idx = int(m.group(4))
        end_idx = int(m.group(5))

        base_offset = int(base_hex, 16)
        end_offset = int(end_hex, 16)
        count = end_idx - start_idx + 1

        if count > 1:
            stride = (end_offset - base_offset) // (count - 1)
        else:
            stride = DEFAULT_ARRAY_STRIDE

        if stride < MIN_REGISTER_STRIDE:
            stride = MIN_REGISTER_STRIDE

        template_name = prefix + "{N}"
        array_registers[template_name] = (base_offset, count, stride)

    # ---- Step 2: Extract all register definitions ----
    # Register names start with a letter, may contain {N} for array templates
    reg_pattern = (
        r'<tr id=([^>]+)>'
        r'.*?'
        r'<td class=r>([A-Za-z][\w{}]*)</td>'  # register name
        r'.*?'
        r'<td class=r>(0x[0-9a-fA-F]+(?:\s*-\s*0x[0-9a-fA-F]+)?)</td>'  # offset (single or range)
    )

    for match in re.finditer(reg_pattern, content, re.DOTALL):
        reg_id = match.group(1)
        reg_name = match.group(2)
        reg_offset = match.group(3)

        # A register may span a range of addresses (a block/FIFO/RAM, e.g.
        # "0x1100 - 0x1104"). We store only the base (first) offset; callers
        # reach the other words with the generic "-i <instance>" option.
        if '-' in reg_offset:
            reg_offset = reg_offset.split('-', 1)[0].strip()

        fields = _extract_fields(content, match.end())

        if '{N}' in reg_name:
            template_name = reg_name
            if template_name in array_registers:
                base, count, stride = array_registers[template_name]
                prefix = template_name.replace('{N}', '')
                for i in range(count):
                    expanded_name = f"{prefix}{i}"
                    expanded_offset = base + i * stride
                    registers[expanded_name] = {
                        'offset': f"0x{expanded_offset:04X}",
                        'fields': fields,
                    }
            else:
                print(f"  Warning: array template '{reg_name}' has no range metadata, "
                      f"storing with offset {reg_offset}", file=sys.stderr)
                registers[reg_name] = {
                    'offset': reg_offset,
                    'fields': fields,
                }
        else:
            registers[reg_name] = {
                'offset': reg_offset,
                'fields': fields,
            }

    return registers, array_registers


def _extract_fields(content, start_pos):
    """Extract bit field definitions from HTML starting at start_pos."""
    fields = []
    seen_fields = set()

    next_reg_pos = content.find('<tr id=', start_pos + 1)
    if next_reg_pos == -1:
        next_reg_pos = start_pos + FIELD_SEARCH_WINDOW
    field_section = content[start_pos:min(next_reg_pos, start_pos + FIELD_SEARCH_WINDOW)]

    field_pattern = r'<td class=o>([0-9\-]+)</td>\s*<td class=s>(.+?)</td>'

    for fmatch in re.finditer(field_pattern, field_section):
        bit_range = fmatch.group(1)
        field_text = fmatch.group(2).strip()

        field_name = re.sub(r'<[^>]+>', '', field_text)
        if '--' in field_name:
            field_name = field_name.split('--')[0].strip()

        field_key = f"{field_name}:{bit_range}"
        if field_key in seen_fields:
            continue
        seen_fields.add(field_key)

        if '-' in bit_range:
            parts = bit_range.split('-')
            high, low = int(parts[0]), int(parts[1])
        else:
            high = low = int(bit_range)

        if field_name and field_name.strip() and field_name.strip().lower() != 'unused':
            fields.append({
                # Normalise field names to lowercase so the tool consistently
                # shows UPPERCASE_REGISTER + "_" + lowercase_field regardless of
                # the (inconsistent) case used in the source regmap HTML.
                'name': field_name.strip().lower(),
                'bits': [low, high],
                'desc': '',
            })

    return fields


# ============================================================================
# PLREG TOOL GENERATION
# ============================================================================


def generate_register_db(registers, array_registers, fpga_name):
    """Generate the REGISTERS and ARRAY_REGISTERS Python source."""
    lines = [
        f'# Auto-generated {fpga_name} register definitions',
        f'# Generated by generate_plreg.py from regmap HTML',
        '',
        'REGISTERS = {',
    ]

    for reg_name in sorted(registers.keys()):
        reg = registers[reg_name]
        lines.append(f'    "{reg_name}": {{')
        lines.append(f'        "offset": "{reg["offset"]}",')

        if reg['fields']:
            lines.append('        "fields": [')
            for field in reg['fields']:
                bits_str = f'[{field["bits"][0]}, {field["bits"][1]}]'
                desc_str = field['desc'].replace('"', '\\"')[:60]
                lines.append(f'            {{"name": "{field["name"]}", '
                             f'"bits": {bits_str}, "desc": "{desc_str}"}},')
            lines.append('        ],')
        else:
            lines.append('        "fields": [],')

        lines.append('    },')

    lines.append('}')
    lines.append('')
    lines.append('# Array register metadata: template_name -> (base_offset, count, stride)')
    lines.append('ARRAY_REGISTERS = {')

    for tmpl in sorted(array_registers.keys()):
        base, count, stride = array_registers[tmpl]
        prefix = tmpl.replace('{N}', '')
        lines.append(f'    "{tmpl}": (0x{base:04X}, {count}, {stride}),  '
                     f'# {prefix}0 - {prefix}{count-1}')

    lines.append('}')

    return '\n'.join(lines)


def build_plreg(registers, array_registers, fpga_name, output_file, pci_device_id):
    """Build the complete standalone plreg tool."""

    fpga_name_upper = fpga_name.upper()
    fpga_name_lower = fpga_name.lower()
    reg_db = generate_register_db(registers, array_registers, fpga_name_upper)

    fpga_pci_device_id = pci_device_id if pci_device_id else ""

    # Header section
    header = f'''#!/usr/bin/env python3
"""
plreg - {fpga_name_upper} Platform Register Read/Write Tool
Usage:
    plreg read <register|field|offset>
    plreg write <register|field|offset> <value>
    plreg list [pattern]
    plreg dump <array_register_name>
    plreg [options] <command> [args...]

Options:
    --base <hex_addr>   Override auto-detected FPGA base address (forces /dev/mem mode)
    -c, --compact       Compact output: print only the hex value (for scripting)
    -i, --instance <n>  Address the n-th 32-bit word of a register block. The
                        effective offset becomes base + n*4 (n=0 is the base).
                        Works with any register or offset; intended for block/
                        FIFO/RAM registers that span several words, e.g.
                        {fpga_name_upper}_RJ45_UART_DATA.

Field access:
    A single bit-field can be read or written using either REG.field or the
    flattened REG_field name shown by 'read'. 'list <pattern>' also matches
    field names:
        plreg read  {fpga_name_upper}_BASE_BRD_ID.fpga_load_type
        plreg read  {fpga_name_upper}_BASE_BRD_ID_fpga_load_type
        plreg write {fpga_name_upper}_BASE_BRD_ID.fpga_load_type 1
        plreg list  fpga_load

Access methods (in priority order):
    1. Kernel regmap sysfs: /sys/bus/platform/devices/regmap-{fpga_name_lower}.0/
    2. /dev/mem mmap (fallback, requires root and no kernel lockdown)

Array registers:
    Registers with {{N}} in their name can be accessed by index:
        plreg read {fpga_name_upper}_GLUE_PMBUS1_VI_MON_DATA0
        plreg dump {fpga_name_upper}_GLUE_PMBUS1_VI_MON_DATA

Register blocks:
    Some registers occupy a range of addresses (a block/FIFO/RAM) but have a
    single name. Use -i/--instance to reach words past the base:
        plreg read    {fpga_name_upper}_RJ45_UART_DATA        # base word (0x1100)
        plreg -i 1 read {fpga_name_upper}_RJ45_UART_DATA      # base + 4  (0x1104)
"""

import sys
import struct
import re
import glob
import os

# FPGA identification
FPGA_NAME = "{fpga_name_upper}"
FPGA_NAME_LOWER = "{fpga_name_lower}"
FPGA_PCI_VENDOR_ID = "0x16fc"  # Ciena
FPGA_PCI_DEVICE_ID = "{fpga_pci_device_id}"

PAGE_SIZE = 4096
REGISTER_WIDTH_BYTES = 4
REGISTER_WIDTH_BITS = 32
REGISTER_MAX_VALUE = 0xFFFFFFFF
BAR0_RESOURCE_MIN_FIELDS = 2
UNRESOLVED_OFFSET = 0xFFFFFFFF
MAX_ARRAY_PREVIEW = 16
MAX_AMBIGUOUS_MATCHES = 20

# Access mode: "regmap" (sysfs) or "devmem" (/dev/mem mmap)
ACCESS_MODE = None
REGMAP_PATH = None
FPGA_BASE = None
'''

    # The code body - detection and access methods
    code_body = r'''

def detect_regmap_sysfs():
    """Find the regmap sysfs path for this FPGA."""
    # Try standard naming: regmap-<fpga_name>.0
    candidates = [
        f"/sys/bus/platform/devices/regmap-{FPGA_NAME_LOWER}.0",
        f"/sys/bus/platform/devices/regmap-{FPGA_NAME_LOWER}.1",
    ]
    # Also glob for any regmap device matching the FPGA name
    candidates += sorted(glob.glob(f"/sys/bus/platform/devices/regmap-{FPGA_NAME_LOWER}*"))

    for path in candidates:
        if os.path.isdir(path) and os.path.exists(os.path.join(path, "read")):
            return path
    return None


def detect_fpga_base():
    """Auto-detect FPGA BAR0 base address via sysfs PCI device ID match."""
    if not FPGA_PCI_DEVICE_ID:
        return None
    pci_devices = sorted(glob.glob("/sys/bus/pci/devices/*"))
    for dev_path in pci_devices:
        try:
            with open(os.path.join(dev_path, "vendor")) as f:
                vendor = f.read().strip()
            with open(os.path.join(dev_path, "device")) as f:
                device = f.read().strip()
            if vendor == FPGA_PCI_VENDOR_ID and device == FPGA_PCI_DEVICE_ID:
                with open(os.path.join(dev_path, "resource")) as f:
                    bar0_line = f.readline().strip()
                parts = bar0_line.split()
                if len(parts) >= BAR0_RESOURCE_MIN_FIELDS:
                    base = int(parts[0], 16)
                    if base != 0:
                        return base
        except (IOError, OSError, ValueError):
            continue
    return None


def init_access():
    """Initialize register access method. Prefers regmap sysfs over /dev/mem."""
    global ACCESS_MODE, REGMAP_PATH, FPGA_BASE

    # If --base was specified, force /dev/mem mode
    if FPGA_BASE is not None:
        ACCESS_MODE = "devmem"
        return

    # Try regmap sysfs first (works under kernel lockdown)
    regmap = detect_regmap_sysfs()
    if regmap:
        ACCESS_MODE = "regmap"
        REGMAP_PATH = regmap
        return

    # Fall back to /dev/mem
    base = detect_fpga_base()
    if base:
        ACCESS_MODE = "devmem"
        FPGA_BASE = base
        return

    # Nothing worked
    print(f"Error: Cannot access {FPGA_NAME} FPGA.", file=sys.stderr)
    print(f"  Tried regmap sysfs: /sys/bus/platform/devices/regmap-{FPGA_NAME_LOWER}.*", file=sys.stderr)
    if FPGA_PCI_DEVICE_ID:
        print(f"  Tried PCI /dev/mem: vendor={FPGA_PCI_VENDOR_ID} device={FPGA_PCI_DEVICE_ID}", file=sys.stderr)
    else:
        print(f"  /dev/mem fallback not available (no PCI device ID compiled in)", file=sys.stderr)
    sys.exit(1)


init_access()


'''

    code_logic = r'''

def _resolve_array_register(name_upper):
    """Check if name_upper matches an expanded array register (e.g. FOO_BAR3).

    Returns (expanded_name, reg_data, offset, template) or None.
    """
    for template, (base_offset, count, stride) in ARRAY_REGISTERS.items():
        prefix = template.replace("{N}", "")
        if not name_upper.startswith(prefix):
            continue
        suffix = name_upper[len(prefix):]
        if not suffix.isdigit():
            continue
        idx = int(suffix)
        if idx < 0 or idx >= count:
            continue
        offset = base_offset + idx * stride
        expanded_name = prefix + suffix
        reg_data = REGISTERS.get(template)
        if reg_data:
            resolved = dict(reg_data)
            resolved['offset'] = f"0x{offset:04X}"
            return (expanded_name, resolved, offset, template)
    return None


def read_register(offset):
    """Read a 32-bit register at the given offset."""
    if ACCESS_MODE == "regmap":
        return _regmap_read(offset)
    else:
        return _devmem_read(offset)


def write_register(offset, value):
    """Write a 32-bit value to the register at the given offset."""
    if ACCESS_MODE == "regmap":
        _regmap_write(offset, value)
    else:
        _devmem_write(offset, value)


def _regmap_read(offset):
    """Read via kernel regmap sysfs binary attribute.

    The 'read' file is a binary attribute. Seek to the register offset
    and read reg_stride bytes (4 for 32-bit registers).
    """
    try:
        read_path = os.path.join(REGMAP_PATH, "read")
        with open(read_path, 'rb') as f:
            f.seek(offset)
            data = f.read(REGISTER_WIDTH_BYTES)
        if len(data) != REGISTER_WIDTH_BYTES:
            print(f"Error: short read from regmap sysfs (got {len(data)} bytes)", file=sys.stderr)
            sys.exit(1)
        return struct.unpack('<I', data)[0]
    except PermissionError:
        print("Error: Need root privileges to access regmap sysfs", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading register via regmap sysfs: {e}", file=sys.stderr)
        sys.exit(1)


def _regmap_write(offset, value):
    """Write via kernel regmap sysfs offset_and_or attribute.

    The 'offset_and_or' file accepts: "offset,mask,val" in hex.
    The kernel does: new = (old & ~mask) | (val & mask)
    To write a full register value: mask=0xFFFFFFFF, val=value.
    """
    try:
        write_path = os.path.join(REGMAP_PATH, "offset_and_or")
        with open(write_path, 'w') as f:
            f.write(f"{offset:x},{REGISTER_MAX_VALUE:x},{value:x}")
    except PermissionError:
        print("Error: Need root privileges to access regmap sysfs", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error writing register via regmap sysfs: {e}", file=sys.stderr)
        sys.exit(1)


def _devmem_read(offset):
    """Read via /dev/mem mmap (fallback)."""
    import mmap as mmap_mod
    try:
        with open('/dev/mem', 'r+b') as f:
            page_offset = (FPGA_BASE + offset) & ~(PAGE_SIZE - 1)
            reg_offset = (FPGA_BASE + offset) & (PAGE_SIZE - 1)

            mem = mmap_mod.mmap(f.fileno(), PAGE_SIZE,
                          mmap_mod.MAP_SHARED, mmap_mod.PROT_READ,
                          offset=page_offset)

            data = mem[reg_offset:reg_offset + REGISTER_WIDTH_BYTES]
            value = struct.unpack('<I', data)[0]
            mem.close()
            return value
    except PermissionError:
        print("Error: Need root privileges to access /dev/mem", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading register: {e}", file=sys.stderr)
        sys.exit(1)

def _devmem_write(offset, value):
    """Write via /dev/mem mmap (fallback)."""
    import mmap as mmap_mod
    try:
        with open('/dev/mem', 'r+b') as f:
            page_offset = (FPGA_BASE + offset) & ~(PAGE_SIZE - 1)
            reg_offset = (FPGA_BASE + offset) & (PAGE_SIZE - 1)

            mem = mmap_mod.mmap(f.fileno(), PAGE_SIZE,
                          mmap_mod.MAP_SHARED, mmap_mod.PROT_READ | mmap_mod.PROT_WRITE,
                          offset=page_offset)

            data = struct.pack('<I', value & REGISTER_MAX_VALUE)
            mem[reg_offset:reg_offset + REGISTER_WIDTH_BYTES] = data
            mem.close()
    except PermissionError:
        print("Error: Need root privileges to access /dev/mem", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error writing register: {e}", file=sys.stderr)
        sys.exit(1)

def find_register(name_or_offset):
    """Find register by name or hex offset"""
    if name_or_offset.startswith('0x'):
        try:
            offset = int(name_or_offset, 16)
            for reg_name, reg_data in REGISTERS.items():
                if reg_data['offset'] == name_or_offset.upper():
                    return (reg_name, reg_data, offset)
            return (None, None, offset)
        except ValueError:
            pass

    name_upper = name_or_offset.upper()
    if name_upper in REGISTERS:
        reg_data = REGISTERS[name_upper]
        offset = int(reg_data['offset'], 16)
        if offset == UNRESOLVED_OFFSET:
            if name_upper in ARRAY_REGISTERS:
                base, count, stride = ARRAY_REGISTERS[name_upper]
                prefix = name_upper.replace("{N}", "")
                print(f"'{name_upper}' is an array register. Use an index:", file=sys.stderr)
                for i in range(min(count, MAX_ARRAY_PREVIEW)):
                    print(f"  {prefix}{i}  (offset 0x{base + i*stride:04X})", file=sys.stderr)
                if count > MAX_ARRAY_PREVIEW:
                    print(f"  ... up to {prefix}{count-1}", file=sys.stderr)
            else:
                print(f"Register '{name_upper}' has unresolved offset", file=sys.stderr)
            sys.exit(1)
        return (name_upper, reg_data, offset)

    arr = _resolve_array_register(name_upper)
    if arr:
        expanded_name, reg_data, offset, template = arr
        return (expanded_name, reg_data, offset)

    # Partial match
    matches = [name for name in REGISTERS.keys() if name_upper in name]

    array_matches = []
    for template, (base, count, stride) in ARRAY_REGISTERS.items():
        prefix = template.replace("{N}", "")
        for i in range(count):
            expanded = f"{prefix}{i}"
            if name_upper in expanded:
                array_matches.append(expanded)

    all_matches = matches + array_matches
    if len(all_matches) == 1:
        candidate = all_matches[0]
        arr = _resolve_array_register(candidate)
        if arr:
            expanded_name, reg_data, offset, template = arr
            return (expanded_name, reg_data, offset)
        reg_data = REGISTERS[candidate]
        offset = int(reg_data['offset'], 16)
        return (candidate, reg_data, offset)
    elif len(all_matches) > 1:
        print(f"Ambiguous register name '{name_or_offset}'. Matches:", file=sys.stderr)
        for match in sorted(set(all_matches))[:MAX_AMBIGUOUS_MATCHES]:
            print(f"  {match}", file=sys.stderr)
        if len(all_matches) > MAX_AMBIGUOUS_MATCHES:
            print(f"  ... and {len(all_matches)-MAX_AMBIGUOUS_MATCHES} more", file=sys.stderr)
        sys.exit(1)

    print(f"Register not found: {name_or_offset}", file=sys.stderr)
    sys.exit(1)

def _field_low_high(field):
    """Return (low, high) bit positions for a field, normalised so low <= high."""
    b = field['bits']
    low, high = b[0], b[1]
    if high < low:
        low, high = high, low
    return low, high

def _full_field_name(reg_name, field_name):
    """Fully-qualified field name as shown by 'read', e.g. REG_field."""
    if reg_name and not field_name.upper().startswith(reg_name.upper()):
        return f"{reg_name}_{field_name}"
    return field_name

def _lookup_register_exact(name):
    """Resolve an exact register name, hex offset, or array element.

    Does no partial matching and never exits. Returns
    (reg_name_or_None, reg_data, offset) or None if unresolved.
    """
    nu = name.upper()
    if nu.startswith('0X'):
        try:
            off = int(nu, 16)
        except ValueError:
            return None
        for rn, rd in REGISTERS.items():
            if rd['offset'].upper() == nu:
                return (rn, rd, off)
        return (None, {'offset': f'0x{off:04X}', 'fields': []}, off)
    if nu in REGISTERS:
        rd = REGISTERS[nu]
        off = int(rd['offset'], 16)
        if off != UNRESOLVED_OFFSET:
            return (nu, rd, off)
    arr = _resolve_array_register(nu)
    if arr:
        en, rd, off, _tmpl = arr
        return (en, rd, off)
    return None

def _iter_all_fields():
    """Yield (full_name_upper, reg_name, reg_data, offset, field) for every
    field of every resolvable register, with array templates expanded."""
    for reg_name, reg_data in REGISTERS.items():
        if '{N}' in reg_name:
            if reg_name in ARRAY_REGISTERS:
                base, count, stride = ARRAY_REGISTERS[reg_name]
                prefix = reg_name.replace('{N}', '')
                for i in range(count):
                    ename = f"{prefix}{i}"
                    eoff = base + i * stride
                    rd = dict(reg_data)
                    rd['offset'] = f"0x{eoff:04X}"
                    for field in reg_data.get('fields', []):
                        full = _full_field_name(ename, field['name']).upper()
                        yield (full, ename, rd, eoff, field)
            continue
        off = int(reg_data['offset'], 16)
        if off == UNRESOLVED_OFFSET:
            continue
        for field in reg_data.get('fields', []):
            full = _full_field_name(reg_name, field['name']).upper()
            yield (full, reg_name, reg_data, off, field)

def find_field(spec):
    """Resolve a field spec to (reg_name, reg_data, offset, field, low, high).

    Accepts either dotted (REG.field) or flattened (REG_field) form -- the
    latter matching the fully-qualified name shown by 'plreg read <reg>'.
    Returns None if the spec does not resolve to exactly one field.
    """
    s = spec.strip()
    if '.' in s:
        reg_part, field_part = s.rsplit('.', 1)
        rr = _lookup_register_exact(reg_part)
        if not rr:
            return None
        reg_name, reg_data, offset = rr
        rn = reg_name or reg_part.upper()
        fpu = field_part.upper()
        for field in reg_data.get('fields', []):
            if field['name'].upper() == fpu or \
               _full_field_name(rn, field['name']).upper() == s.upper():
                low, high = _field_low_high(field)
                return (rn, reg_data, offset, field, low, high)
        return None
    su = s.upper()
    matches = [(rn, rd, off, f)
               for (full, rn, rd, off, f) in _iter_all_fields() if full == su]
    if len(matches) == 1:
        rn, rd, off, field = matches[0]
        low, high = _field_low_high(field)
        return (rn, rd, off, field, low, high)
    return None

def resolve_target(spec):
    """Resolve a read/write target to either:
        ('reg',   reg_name, reg_data, offset)
        ('field', reg_name, reg_data, offset, field, low, high)

    A dotted spec (REG.field) is always treated as a field. Otherwise an exact
    register/offset wins, then an exact flattened field name, then a partial
    register match (via find_register, which may exit on ambiguity).
    """
    if '.' in spec:
        ff = find_field(spec)
        if ff:
            return ('field',) + ff
        print(f"Field not found: {spec}", file=sys.stderr)
        sys.exit(1)
    reg = _lookup_register_exact(spec)
    if reg is not None:
        return ('reg', reg[0], reg[1], reg[2])
    ff = find_field(spec)
    if ff:
        return ('field',) + ff
    reg_name, reg_data, offset = find_register(spec)
    return ('reg', reg_name, reg_data, offset)

def read_field(offset, low, high):
    """Read register at offset and extract the [low:high] field value."""
    value = read_register(offset)
    width = high - low + 1
    return (value >> low) & ((1 << width) - 1)

def write_field(offset, low, high, field_value):
    """Read-modify-write a single bit-field, leaving other bits untouched."""
    width = high - low + 1
    fmask = (1 << width) - 1
    if field_value < 0 or (field_value & ~fmask):
        print(f"Error: value 0x{field_value:X} does not fit in {width}-bit field "
              f"[{low}:{high}]", file=sys.stderr)
        sys.exit(1)
    mask = fmask << low
    shifted = (field_value & fmask) << low
    if ACCESS_MODE == "regmap":
        # Kernel applies new = (old & ~mask) | (val & mask) atomically.
        try:
            write_path = os.path.join(REGMAP_PATH, "offset_and_or")
            with open(write_path, 'w') as f:
                f.write(f"{offset:x},{mask:x},{shifted:x}")
        except PermissionError:
            print("Error: Need root privileges to access regmap sysfs", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Error writing field via regmap sysfs: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        old = read_register(offset)
        write_register(offset, (old & ~mask) | shifted)

def apply_instance(reg_name, offset, instance):
    """Return (effective_offset, display_name) for the n-th 32-bit word.

    The effective offset is base + instance*REGISTER_WIDTH_BYTES. This is a
    generic word index for block/FIFO/RAM registers; no upper bound is enforced
    (the caller is trusted, matching the raw nature of the tool).
    """
    if instance == 0:
        return offset, reg_name
    eff = offset + instance * REGISTER_WIDTH_BYTES
    display = f"{reg_name}[{instance}]" if reg_name else None
    return eff, display

def cmd_read(args, compact=False, instance=0):
    """Read and display a register or a single field"""
    if len(args) < 1:
        print("Usage: plreg read <register|field|offset>", file=sys.stderr)
        sys.exit(1)

    target = resolve_target(args[0])
    if target[0] == 'field':
        if instance != 0:
            print("Error: -i/--instance cannot be used with a bit-field",
                  file=sys.stderr)
            sys.exit(1)
        _, reg_name, reg_data, offset, field, low, high = target
        fval = read_field(offset, low, high)
        if compact:
            print(f"0x{fval:X}")
            return
        full = _full_field_name(reg_name, field['name'])
        width = high - low + 1
        mask = ((1 << width) - 1) << low
        bit_str = f"[{low}]" if low == high else f"[{low}:{high}]"
        print()
        print(f"{full}  {bit_str}")
        print(f"Register: {reg_name} (offset 0x{offset:04X})")
        print(f"Mask:     0x{mask:08X}")
        print(f"Value:    0x{fval:X} ({fval})")
        print()
        return

    reg_name, reg_data, offset = target[1], target[2], target[3]
    offset, reg_name = apply_instance(reg_name, offset, instance)
    if instance != 0:
        # Field definitions belong to the base word only; don't decode them
        # against a different word of the block.
        reg_data = None
    value = read_register(offset)

    if compact:
        print(f"0x{value:02X}")
        return

    physical_addr = (FPGA_BASE + offset) if FPGA_BASE is not None else None

    if reg_name:
        print(f"\n{reg_name}")
        if physical_addr is not None:
            print(f"Address: 0x{physical_addr:08X} (offset 0x{offset:04X})")
        else:
            print(f"Offset: 0x{offset:04X}")
        print(f"Value:   0x{value:08X}")

        if reg_data and reg_data.get('fields'):
            print()
            print(f"{'Field Name':<50s} {'Bits':<8s} {'Mask':<12s} {'Value':<12s}")
            print("-" * 82)

            for field in reg_data['fields']:
                bits = field['bits']
                if len(bits) != 2:
                    continue

                low, high = bits
                if high < low or high > (REGISTER_WIDTH_BITS - 1) or low < 0:
                    continue
                width = high - low + 1
                if width <= 0 or width > REGISTER_WIDTH_BITS:
                    continue

                mask = ((1 << width) - 1) << low
                field_val = (value >> low) & ((1 << width) - 1)

                if reg_name and not field['name'].startswith(reg_name):
                    full_name = f"{reg_name}_{field['name']}"
                else:
                    full_name = field['name']

                if low == high:
                    bit_str = f"[{low}]"
                else:
                    bit_str = f"[{low}:{high}]"

                print(f"{full_name:<50s} {bit_str:<8s} 0x{mask:08X}   0x{field_val:08X}")

            print()
            print("           31*****24*23*****16*15******8*7*******0")

            binary_str = ""
            for byte_idx in range(REGISTER_WIDTH_BYTES):
                byte_bits = (value >> (24 - byte_idx * 8)) & 0xFF
                binary_byte = format(byte_bits, '08b')
                binary_str += binary_byte + "."
            binary_str = binary_str.rstrip('.')

            formatted = ""
            for i, bit in enumerate(binary_str.replace('.', '')):
                if i > 0 and i % 4 == 0:
                    formatted += "."
                formatted += bit

            print(f"           {formatted}")
            print()
    else:
        if physical_addr is not None:
            print(f"\nOffset 0x{offset:04X} (0x{physical_addr:08X}): 0x{value:08X}")
        else:
            print(f"\nOffset 0x{offset:04X}: 0x{value:08X}")

        print()
        print("           31*****24*23*****16*15******8*7*******0")
        binary_str = ""
        for byte_idx in range(REGISTER_WIDTH_BYTES):
            byte_bits = (value >> (24 - byte_idx * 8)) & 0xFF
            binary_byte = format(byte_bits, '08b')
            binary_str += binary_byte + "."
        binary_str = binary_str.rstrip('.')

        formatted = ""
        for i, bit in enumerate(binary_str.replace('.', '')):
            if i > 0 and i % 4 == 0:
                formatted += "."
            formatted += bit

        print(f"           {formatted}")
        print()

def cmd_write(args, compact=False, instance=0):
    """Write a value to a register or a single field"""
    if len(args) < 2:
        print("Usage: plreg write <register|field|offset> <value>", file=sys.stderr)
        sys.exit(1)

    target = resolve_target(args[0])

    try:
        if args[1].lower().startswith('0x'):
            value = int(args[1], 16)
        else:
            value = int(args[1])
    except ValueError:
        print(f"Invalid value: {args[1]}", file=sys.stderr)
        sys.exit(1)

    if target[0] == 'field':
        if instance != 0:
            print("Error: -i/--instance cannot be used with a bit-field",
                  file=sys.stderr)
            sys.exit(1)
        _, reg_name, reg_data, offset, field, low, high = target
        fmask = (1 << (high - low + 1)) - 1
        old_reg = read_register(offset)
        old_f = (old_reg >> low) & fmask
        write_field(offset, low, high, value)
        new_reg = read_register(offset)
        new_f = (new_reg >> low) & fmask
        if compact:
            print(f"0x{new_f:X}")
            if new_f != (value & fmask):
                sys.exit(1)
            return
        full = _full_field_name(reg_name, field['name'])
        bit_str = f"[{low}]" if low == high else f"[{low}:{high}]"
        print(f"{full} {bit_str} (in {reg_name} offset 0x{offset:04X}):")
        print(f"  Old field: 0x{old_f:X}   (reg 0x{old_reg:08X})")
        print(f"  New field: 0x{new_f:X}   (reg 0x{new_reg:08X})")
        if new_f != (value & fmask):
            print("  Warning: Read-back field differs from written value!", file=sys.stderr)
        return

    reg_name, reg_data, offset = target[1], target[2], target[3]
    offset, reg_name = apply_instance(reg_name, offset, instance)

    old_value = read_register(offset)
    write_register(offset, value)
    new_value = read_register(offset)

    if compact:
        print(f"0x{new_value:02X}")
        if new_value != value:
            sys.exit(1)
        return

    if reg_name:
        print(f"{reg_name} (offset 0x{offset:04X}):")
    else:
        print(f"Offset 0x{offset:04X}:")
    print(f"  Old value: 0x{old_value:08X}")
    print(f"  New value: 0x{new_value:08X}")

    if new_value != value:
        print(f"  Warning: Read-back value differs from written value!", file=sys.stderr)

def cmd_list(args):
    """List registers matching a pattern, expanding {N} array templates"""
    pattern = args[0].upper() if args else ''

    matches = []
    for reg_name, reg_data in sorted(REGISTERS.items()):
        if '{N}' in reg_name:
            if reg_name in ARRAY_REGISTERS:
                base, count, stride = ARRAY_REGISTERS[reg_name]
                prefix = reg_name.replace("{N}", "")
                for i in range(count):
                    expanded = f"{prefix}{i}"
                    if pattern in expanded:
                        matches.append((expanded, f"0x{base + i*stride:04X}"))
            else:
                if pattern in reg_name:
                    matches.append((reg_name, reg_data['offset']))
        else:
            if pattern in reg_name:
                matches.append((reg_name, reg_data['offset']))

    field_matches = []
    if pattern:
        seen = set()
        for full, f_reg, f_rd, f_off, field in _iter_all_fields():
            if pattern in full and full not in seen:
                seen.add(full)
                low, high = _field_low_high(field)
                bit_str = f"[{low}]" if low == high else f"[{low}:{high}]"
                display = _full_field_name(f_reg, field['name'])
                field_matches.append((display, f"0x{f_off:04X}", bit_str))

    if not matches and not field_matches:
        print(f"No registers or fields matching '{pattern}'", file=sys.stderr)
        sys.exit(1)

    if matches:
        print(f"Found {len(matches)} register(s):")
        for reg_name, offset in matches:
            print(f"  {reg_name:60s} {offset}")

    if field_matches:
        print(f"Found {len(field_matches)} field(s):")
        for full, offset, bit_str in sorted(field_matches):
            print(f"  {full:60s} {offset}  {bit_str}")

def cmd_dump(args):
    """Dump all elements of an array register, or a single register."""
    if len(args) < 1:
        print("Usage: plreg dump <register_name_or_template>", file=sys.stderr)
        print("  Dumps all elements of an array register, or a single register.", file=sys.stderr)
        sys.exit(1)

    name_upper = args[0].upper()

    template_key = None
    if name_upper in ARRAY_REGISTERS:
        template_key = name_upper
    else:
        candidate = name_upper + "{N}"
        if candidate in ARRAY_REGISTERS:
            template_key = candidate
        else:
            for tmpl in ARRAY_REGISTERS:
                prefix = tmpl.replace("{N}", "")
                if name_upper == prefix or name_upper == prefix.rstrip("_"):
                    template_key = tmpl
                    break

    if template_key:
        base, count, stride = ARRAY_REGISTERS[template_key]
        reg_data = REGISTERS.get(template_key, {})
        fields = reg_data.get('fields', [])
        prefix = template_key.replace("{N}", "")

        print(f"Array: {template_key}  ({count} elements, base=0x{base:04X}, stride={stride})")
        print(f"{'Index':<8} {'Name':<55} {'Offset':<10} {'Value':<12} {'Hex'}")
        print("-" * 100)

        for i in range(count):
            offset = base + i * stride
            value = read_register(offset)
            expanded = f"{prefix}{i}"
            print(f"{i:<8} {expanded:<55} 0x{offset:04X}     0x{value:08X}   ({value})")

            if "ADC_SPI_CONV_VALUE" in template_key:
                ch_hi = (value >> 16) & 0xFFFF
                ch_lo = value & 0xFFFF
                ch_hi_idx = i * 2
                ch_lo_idx = i * 2 + 1
                if i < count - 1 or ch_lo != 0:
                    print(f"         ch{ch_hi_idx}: {ch_hi} (0x{ch_hi:04X}),  ch{ch_lo_idx}: {ch_lo} (0x{ch_lo:04X})")
                else:
                    print(f"         ch{ch_hi_idx}: {ch_hi} (0x{ch_hi:04X}),  ch{ch_lo_idx}: unused")

            if "PMBUS" in template_key and "VI_MON" in template_key:
                hi = (value >> 16) & 0xFFFF
                lo = value & 0xFFFF
                print(f"         [31:16]=0x{hi:04X} ({hi}),  [15:0]=0x{lo:04X} ({lo})")
        return

    # Not an array — try as a single register
    try:
        reg_name, reg_data, offset = find_register(args[0])
        value = read_register(offset)
        if reg_name:
            print(f"{reg_name} (0x{offset:04X}): 0x{value:08X} ({value})")
        else:
            print(f"0x{offset:04X}: 0x{value:08X} ({value})")
    except SystemExit:
        print(f"Register not found and not an array template: {args[0]}", file=sys.stderr)
        print("Available array registers:", file=sys.stderr)
        for tmpl in sorted(ARRAY_REGISTERS.keys()):
            base, count, stride = ARRAY_REGISTERS[tmpl]
            print(f"  {tmpl:<55} {count} elements, base=0x{base:04X}", file=sys.stderr)
        sys.exit(1)


def main():
    global FPGA_BASE, ACCESS_MODE

    compact = False
    instance = 0
    argv = sys.argv[1:]

    # Pull out -i/--instance from anywhere in the argument list, since it is
    # commonly written after the register name (e.g. "read REG -i 1").
    filtered = []
    i = 0
    while i < len(argv):
        if argv[i] in ('-i', '--instance'):
            if i + 1 >= len(argv):
                print("Error: -i/--instance requires an integer", file=sys.stderr)
                sys.exit(1)
            try:
                instance = int(argv[i + 1], 0)
            except ValueError:
                print(f"Error: Invalid instance: {argv[i + 1]}", file=sys.stderr)
                sys.exit(1)
            if instance < 0:
                print("Error: instance must be >= 0", file=sys.stderr)
                sys.exit(1)
            i += 2
            continue
        filtered.append(argv[i])
        i += 1
    argv = filtered

    while argv:
        if argv[0] == '--base':
            if len(argv) < 2:
                print("Error: --base requires a hex address", file=sys.stderr)
                sys.exit(1)
            try:
                FPGA_BASE = int(argv[1], 16)
                ACCESS_MODE = "devmem"
            except ValueError:
                print(f"Error: Invalid base address: {argv[1]}", file=sys.stderr)
                sys.exit(1)
            argv = argv[2:]
        elif argv[0] in ('-c', '--compact'):
            compact = True
            argv = argv[1:]
        else:
            break

    if len(argv) < 1:
        print(__doc__, file=sys.stderr)
        if ACCESS_MODE == "regmap":
            print(f"Access: regmap sysfs ({REGMAP_PATH})", file=sys.stderr)
        elif FPGA_BASE is not None:
            print(f"Access: /dev/mem (FPGA base: 0x{FPGA_BASE:08X})", file=sys.stderr)
        sys.exit(1)

    cmd = argv[0].lower()
    args = argv[1:]

    if cmd == 'read':
        cmd_read(args, compact=compact, instance=instance)
    elif cmd == 'write':
        cmd_write(args, compact=compact, instance=instance)
    elif cmd == 'list':
        cmd_list(args)
    elif cmd == 'dump':
        cmd_dump(args)
    else:
        print(f"Unknown command: {cmd}", file=sys.stderr)
        print(__doc__, file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
'''

    # Assemble the final plreg tool
    with open(output_file, 'w') as f:
        f.write(header)
        f.write(code_body)
        f.write(reg_db)
        f.write('\n')
        f.write(code_logic)

    # Make executable
    os.chmod(output_file, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP |
             stat.S_IROTH | stat.S_IXOTH)

    return output_file


# ============================================================================
# MAIN
# ============================================================================

# Positional argument indices (after option parsing)
ARG_HTML_FILE = 0
ARG_OUTPUT_FILE = 1
MIN_ARGS_WITH_OUTPUT = 2
MIN_ARGS_REQUIRED = 1

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    # Parse arguments
    args = sys.argv[1:]
    pci_device_id = None

    while args and args[0].startswith('--'):
        if args[0] == '--device-id':
            if len(args) < 2:
                print("Error: --device-id requires a hex value (e.g. 0x032c)", file=sys.stderr)
                sys.exit(1)
            pci_device_id = args[1].lower()
            if not pci_device_id.startswith('0x'):
                pci_device_id = '0x' + pci_device_id
            args = args[2:]
        else:
            print(f"Error: Unknown option: {args[0]}", file=sys.stderr)
            sys.exit(1)

    if len(args) < MIN_ARGS_REQUIRED:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    html_file = args[ARG_HTML_FILE]

    # Derive FPGA name from filename
    basename = os.path.splitext(os.path.basename(html_file))[0]
    fpga_name = basename.replace('_regmap', '').upper()

    # Output path
    if len(args) >= MIN_ARGS_WITH_OUTPUT:
        output_file = args[ARG_OUTPUT_FILE]
    else:
        output_dir = os.path.dirname(os.path.abspath(html_file))
        output_file = os.path.join(output_dir, 'plreg')

    if not os.path.exists(html_file):
        print(f"Error: HTML file not found: {html_file}", file=sys.stderr)
        sys.exit(1)

    # --device-id is optional (only needed for /dev/mem fallback)
    if pci_device_id:
        print(f'  /dev/mem fallback: PCI device 16fc:{pci_device_id[2:]}')
    else:
        print(f'  /dev/mem fallback: disabled (no --device-id provided)')

    print(f'Parsing {html_file} (FPGA: {fpga_name})...')
    registers, array_registers = parse_regmap_html(html_file)

    # Summary
    print(f'Found {len(registers)} register entries '
          f'(from {len(array_registers)} array templates)')

    if array_registers:
        print(f'\nArray registers ({len(array_registers)}):')
        for tmpl in sorted(array_registers.keys()):
            base, count, stride = array_registers[tmpl]
            prefix = tmpl.replace('{N}', '')
            print(f'  {tmpl:<55} base=0x{base:04X}  count={count}  stride={stride}')

    # Build plreg
    build_plreg(registers, array_registers, fpga_name, output_file, pci_device_id)

    file_size = os.path.getsize(output_file)
    print(f'\nGenerated: {output_file} ({file_size:,} bytes, {len(registers)} registers)')
    print(f'FPGA: {fpga_name} (primary access: regmap-{fpga_name.lower()}.0/read)')
    if pci_device_id:
        print(f'Fallback: /dev/mem via PCI device 16fc:{pci_device_id[2:]}')
