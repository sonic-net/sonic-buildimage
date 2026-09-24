---
applyTo: "device/**"
---

# SONiC Device Directory — AI Contribution Guidelines

## Scope

- Applies to files under `device/**`
- Use this as contribution guidance, not a hard schema; prefer matching nearby
    existing platform/HWSKU patterns in this repo

## Overview

The `device/` directory contains hardware-specific configuration files for each
network switch supported by SONiC. Files are organized by vendor, platform, and
hardware SKU (HWSKU).

## Directory Hierarchy (Representative)

```
device/
├── common/                                # Shared profiles across vendors
│   └── profiles/<asic>/gen/<profile>/     # Common buffer default templates
└── <vendor_name>/                         # e.g., arista, dell, mellanox
    └── <onie_platform_string>/            # e.g., x86_64-arista_7060x6_64pe_b
        ├── default_sku                    # "<HWSKU_NAME> <topology>" (e.g., "Arista-7060X6-64PE-B-C512S2 t1")
        ├── installer.conf                 # Platform installer configuration
        ├── media_settings.json            # Optics/transceiver media settings
        ├── optics_si_settings.json        # Signal integrity settings (optional)
        ├── platform.json                  # Platform metadata (optional)
        ├── platform_asic                  # ASIC type identifier (e.g., "broadcom")
        ├── platform_components.json       # FW component versions (optional)
        ├── platform_env.conf              # Environment config (optional)
        ├── platform_reboot               # Custom reboot script (optional)
        ├── pmon_daemon_control.json       # Platform monitor daemon config (optional)
        ├── sensors.conf                   # lm-sensors hardware thresholds (optional)
        ├── system_health_monitoring_config.json  # Health monitoring (optional)
        ├── thermal_policy.json            # Thermal management policy (optional)
        ├── plugins/                       # Legacy platform API plugins (optional)
        │   ├── eeprom.py
        │   ├── sfputil.py
        │   └── psuutil.py
        └── <HWSKU_Name>/                  # e.g., Arista-7060X6-64PE-B-P64
            ├── port_config.ini            # Port-to-lane mapping (required)
            ├── buffers.json.j2            # Buffer allocation Jinja2 template
            ├── buffer_ports.j2            # Port list generation macro
            ├── qos.json.j2               # QoS policy Jinja2 template
            ├── sai.profile               # SAI (Switch Abstraction Interface) config
            ├── hwsku.json                 # Port breakout mode definitions
            ├── pg_profile_lookup.ini      # Buffer profiles per speed/cable length
            ├── buffers_defaults_t0.j2     # T0 (ToR) buffer defaults (often a symlink to common/)
            ├── buffers_defaults_t1.j2     # T1 (Leaf) buffer defaults (often a symlink to common/)
            └── <asic_config>.config.bcm   # ASIC-specific config referenced by sai.profile
```

## Naming Conventions

### Vendor Name
- All lowercase, no spaces or hyphens
- Examples: `arista`, `dell`, `mellanox`, `celestica`, `quanta`, `nokia`

### ONIE Platform String
- Format: `<arch>-<vendor>_<model_info>-r<revision>`
- Always lowercase
- Examples: `x86_64-arista_7060x6_64pe_b`, `x86_64-dell_z9264f-r0`, `x86_64-mlnx_msn4600c-r0`
- Keep naming consistent with existing folders for the same vendor; do not rename
    established platform directories

### HWSKU Name
- Vendor's official model name with proper capitalization
- Use hyphens to separate components
- Examples: `Arista-7060X6-64PE-B-P64`, `DellEMC-S5232f-C32`, `Mellanox-SN4600C-C64`
- Follow existing vendor conventions when adding new HWSKUs under the same platform

## File Format Requirements

### port_config.ini (Required for HWSKU)
```ini
# name        lanes                            alias        index  speed    fec
Ethernet0     17,18,19,20,21,22,23,24          etp1         1      800000   rs
Ethernet8     1,2,3,4,5,6,7,8                  etp2         2      800000   rs
```
- Lanes must match physical ASIC layout
- Speed is specified in Mbps (e.g., 800000 = 800Gbps, 100000 = 100Gbps)
- FEC column (rs/fc/none) is optional but recommended for high-speed ports

### buffers.json.j2 (Jinja2 Template)
Typical content — references the platform-level default:
```jinja2
{%- set default_topo = 'ft2' %}
{%- include 'buffers_config.j2' %}
```
Only create a custom template if the HWSKU has genuinely different buffer requirements.

### qos.json.j2 (Jinja2 Template)
Typical content — almost always a single line:
```jinja2
{%- include 'qos_config.j2' %}
```

### buffer_ports.j2 (Jinja2 Macro)
Generates the list of Ethernet ports for the buffer configuration:
```jinja2
{%- macro generate_port_lists(PORT_ALL) %}
    {%- for port_idx in range(0, 513, 8) %}
        {%- if PORT_ALL.append("Ethernet%d" % (port_idx)) %}{%- endif %}
    {%- endfor %}
{%- endmacro %}
```
The range and step must match the actual port layout of the HWSKU.

### buffers_defaults_t0.j2 / buffers_defaults_t1.j2
Often a **symlink** to shared profiles in `device/common/profiles/`:
```
../../../common/profiles/th5/gen/BALANCED/buffers_defaults_t0.j2
```
Prefer symlinks to shared profiles over duplicating content.

### sai.profile
```ini
SAI_INIT_CONFIG_FILE=/usr/share/sonic/hwsku/th5-a7060x6-64pe.config.bcm
SAI_NUM_ECMP_MEMBERS=64
SAI_NHG_HIERARCHICAL_NEXTHOP=false
```
- File paths must reference `/usr/share/sonic/hwsku/` or `/usr/share/sonic/platform/`
- Keys are uppercase with underscores

### pg_profile_lookup.ini
```ini
# PG lossless profiles.
# speed  cable  size     xon    xoff    threshold  xon_offset
800000   5m     8636     0      724662  0          3556
```

### hwsku.json
```json
{
    "interfaces": {
        "Ethernet0": {
            "default_brkout_mode": "1x800G[400G]"
        }
    }
}
```
Breakout notation: `<count>x<speed>[<alt_speed>]` (e.g., `4x25G[10G]`, `1x100G[40G]`)

### default_sku
```
Arista-7060X6-64PE-B-C512S2 t1
```
Format: `<default_HWSKU_name> <default_topology>`. Topologies: t0, t1, t2, ft2, etc.

Some legacy or special-case platforms may differ; keep changes aligned with sibling
entries for that vendor/platform.

## CI and Review Expectations

1. **No compiled binaries**: Never commit `.whl`, `.so`, `.pyc`, `.o`, or compiled archives.
   LED firmware blobs (`.bin`) and ASIC config files (`.config.bcm`, `.soc`) are exceptions
2. **Valid Jinja2**: All `.j2` files must parse without errors by `sonic-cfggen`
3. **Minimal diff**: Reuse templates via `{% include %}` or symlinks to `device/common/`
4. **Signed-off commits**: All commits must include `Signed-off-by:` (use `git commit -s`)
5. **Unix line endings**: Always use LF, never CRLF

## Common Mistakes

- Copying an entire HWSKU folder when only `port_config.ini` and `hwsku.json` differ
- Creating a custom `buffers.json.j2` identical to other HWSKUs on the same platform
- Including vendor Python `.whl` files as binary blobs
- Hardcoding buffer sizes instead of using `pg_profile_lookup.ini`
- Using Windows line endings (CRLF)
