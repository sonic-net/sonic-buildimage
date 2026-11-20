---
applyTo: "platform/**"
---

# SONiC Platform Directory — AI Contribution Guidelines

## Scope

- Applies to files under `platform/**`
- Use this as contribution guidance, not a hard schema; prefer matching nearby
    vendor and ASIC-family patterns already present in this repo

## Overview

The `platform/` directory contains ASIC vendor-specific drivers, Board Support
Packages (BSPs), kernel modules, Docker container definitions, and build recipes.
Unlike `device/` (which holds per-switch configs), `platform/` holds the low-level
software that enables SONiC on a specific chipset family.

## Directory Hierarchy (Representative, Not Exhaustive)

```
platform/
├── broadcom/                     # Memory: TD2, TH, TH2, TH3, TH4, TH5
├── mellanox/                     # Spectrum 1/2/3/4 ASICs (NVIDIA)
├── marvell-prestera/             # Enterprise switches
├── marvell-teralynx/             # Data center switches
├── aspeed/                       # AST-based platform modules
├── nephos/                       # Nephos-based platform modules
├── barefoot/                     # Tofino programmable ASICs (Intel)
├── centec/                       # Centec ASICs (x86_64)
├── centec-arm64/                 # Centec ASICs (ARM)
├── nvidia-bluefield/             # DPU (Data Processing Unit)
├── vs/                           # Virtual Switch (software simulation for testing)
├── pddf/                         # Platform Driver Development Framework
│   ├── i2c/                      # I2C device drivers
│   ├── platform-api-pddf-base/   # PDDF base platform API
│   └── pddf-platform-modules/    # PDDF kernel modules
├── template/                     # Common templates for platform builds
├── components/                   # Shared platform components
└── generic/                      # Generic platform support
```

### Typical ASIC Vendor Directory
```
platform/<vendor>/
├── docker-syncd-<vendor>/        # Docker container for syncd daemon
│   ├── Dockerfile.j2             # Jinja2-templated Dockerfile
│   ├── base_image_version/
│   └── supervisord.conf
├── docker-syncd-<vendor>-rpc/    # RPC variant (optional)
├── one-image.mk                  # Build recipe for platform SONiC image
├── platform-modules-<vendor>.mk  # Kernel module build recipe
├── rules.mk                      # Platform-specific build rules
└── <component_name>/             # Individual platform components
    ├── Makefile
    └── src/
```

## What Belongs in platform/ vs device/

| `platform/` (this directory) | `device/` (NOT this directory) |
|---|---|
| Kernel modules (.ko) and drivers | Port config (port_config.ini) |
| Docker container definitions | Buffer/QoS templates (*.json.j2) |
| Build recipes (.mk files) | SAI profile (sai.profile) |
| ASIC SDK integration | Sensor thresholds (sensors.conf) |
| Syncd daemon configuration | Per-HWSKU breakout definitions |

## Build System Conventions

### Makefile Recipes (.mk files)
```makefile
# one-image.mk — defines output image filename
SONIC_IMAGE_FILENAME = sonic-<vendor>.bin

# platform-modules-<vendor>.mk — kernel module package
SONIC_PLATFORM_MODULE = platform-modules-<vendor>_$(PLATFORM_MODULE_VERSION)_amd64.deb
$(SONIC_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/<vendor>
$(SONIC_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS)
```
- Use **tabs** for Makefile indentation (GNU Make requirement)
- Use `UPPER_SNAKE_CASE` for Make variables
- Reference `$(PLATFORM_PATH)` for platform-relative paths
- Reuse existing variable naming and package patterns from sibling `.mk` files

### Docker Containers
- Extend the base `docker-syncd` image
- Use `Dockerfile.j2` (Jinja2 template) when build-time variables are needed
- Manage processes with `supervisord` — define all daemons in `supervisord.conf`
- Init scripts (`docker-init.sh` or `docker-init.j2`) use `sonic-cfggen` for configuration

### PDDF (Platform Driver Development Framework)
- Uses **JSON descriptor files** instead of custom kernel modules
- JSON descriptors go in `device/<vendor>/<platform>/pddf/`
- Base Python API classes are in `platform/pddf/platform-api-pddf-base/`
- Prefer PDDF over custom drivers when hardware is standard I2C-based

## CI Expectations

1. CI coverage is partial and evolves over time; changes outside commonly gated
    platforms may not be exercised automatically
2. Changes to other platforms may not be automatically tested
3. Kernel modules must build cleanly against SONiC kernel headers
4. Never commit pre-compiled binaries — build all `.deb` and `.ko` from source
5. All commits require `Signed-off-by:` (use `git commit -s`)
6. Always use Unix line endings (LF), never CRLF

## Common Mistakes

- Adding a platform without updating `rules/config` for the build matrix
- Committing vendor SDK binaries instead of building from source
- Using absolute paths instead of `$(PLATFORM_PATH)` variables in Makefiles
- Creating custom drivers when PDDF JSON descriptors would work
- Forgetting to add Docker image build dependencies
- Making broad refactors across multiple vendor folders in a single PR instead of
    focused, vendor-scoped changes
