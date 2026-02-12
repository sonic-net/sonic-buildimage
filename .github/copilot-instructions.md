# Copilot Instructions for sonic-buildimage

## Project Overview

sonic-buildimage is the master build system for SONiC (Software for Open Networking in the Cloud). It produces ONIE-compatible network operating system installer images for network switches across multiple ASIC platforms (Broadcom, Mellanox/NVIDIA, Marvell, etc.). This is the central repo that pulls in all other SONiC components as submodules and builds them into a complete NOS image.

## Architecture

```
sonic-buildimage/
├── device/           # Platform-specific device configurations and plugins
├── dockers/          # Dockerfile definitions for all SONiC containers
├── files/            # Configuration files, scripts, and templates
├── installer/        # ONIE installer scripts
├── platform/         # Platform-specific build rules and configurations
├── rules/            # Makefile rules for building individual components
├── scripts/          # Build helper scripts
├── sonic-slave-*/    # Build environment container definitions (per Debian version)
├── src/              # Source code and submodules for SONiC components
├── .azure-pipelines/ # CI/CD pipeline definitions
├── Makefile          # Top-level build entry point
└── .github/          # GitHub Actions and PR templates
```

### Key Concepts
- **Rules system**: Each component has a `.mk` file in `rules/` defining how to build it
- **Docker containers**: SONiC services run in Docker containers defined in `dockers/`
- **Platform abstraction**: `device/` and `platform/` directories abstract hardware differences
- **Build slaves**: Builds run inside Debian-versioned containers (bookworm, bullseye, etc.)
- **Submodules**: Most SONiC components are git submodules under `src/`

## Language & Style

- **Primary languages**: Makefile, Shell (bash), Python, Jinja2 templates
- **Makefile style**: Use tabs for indentation in Makefiles (GNU Make requirement)
- **Shell scripts**: Use `#!/bin/bash`, 4-space indentation
- **Python**: Follow PEP 8, 4-space indentation
- **Naming**: Use snake_case for variables and functions in shell/Python; UPPER_CASE for Make variables

## Build Instructions

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/sonic-net/sonic-buildimage.git
cd sonic-buildimage

# Initialize build environment
make init

# Configure for a specific platform
make configure PLATFORM=vs  # Virtual Switch for testing
# Other platforms: broadcom, mellanox, marvell-teralynx, etc.

# Build the image
make SONIC_BUILD_JOBS=4 target/sonic-vs.img.gz

# Build specific component
make target/debs/bookworm/swss_1.0.0_amd64.deb
```

### Build Environment Requirements
- Multiple CPU cores, 8+ GiB RAM, 300+ GiB disk
- Docker installed and running
- KVM virtualization support (for some builds)

## Testing

- **VS (Virtual Switch)** platform is the primary testing platform
- CI runs on Azure Pipelines (`.azure-pipelines/`)
- Test images are built with `PLATFORM=vs`
- Integration tests run against VS images in sonic-mgmt repo
- Use `pytest.ini` at the root for Python test configuration

## PR Guidelines

- **Commit format**: `[component/folder]: Description of changes`
- **Signed-off-by**: All commits MUST include `Signed-off-by: Your Name <email>` (DCO requirement)
- **CLA**: Sign the Linux Foundation EasyCLA before contributing
- **Single logical change per PR**: Isolate each commit to one component/bugfix/feature
- **Submodule updates**: When updating a submodule, reference the PR in the submodule repo
- **PR description**: Include what changed, why, and how to test

## Common Patterns

- **Adding a new package**: Create a `.mk` file in `rules/`, add source in `src/`
- **Adding a Docker container**: Create Dockerfile in `dockers/`, add build rule in `rules/`
- **Platform support**: Add platform config in `device/<vendor>/`, build rules in `platform/`
- **Version pinning**: Dependencies are version-pinned in rules files
- **Build flags**: Use `ENABLE_*` and `INCLUDE_*` variables to toggle features

## Dependencies

- All SONiC repos are submodules (sonic-swss, sonic-sairedis, sonic-utilities, etc.)
- Debian base system (bookworm/bullseye)
- Docker for containerized builds
- Azure Pipelines for CI/CD

## Gotchas

- **Build times**: Full builds take 2-6 hours; use `SONIC_BUILD_JOBS` to parallelize
- **Disk space**: Builds require 100+ GiB; clean with `make clean` or `make reset`
- **Submodule versions**: Always check that submodule pins are correct before building
- **Docker cache**: Build uses Docker layer caching; `make clean` to force rebuild
- **Branch compatibility**: Component branches must match buildimage branch (e.g., master ↔ master)
- **Make variables**: Many build options are controlled by variables in `rules/config`
- **Platform differences**: Some features are platform-specific; check `rules/config` for `ENABLE_*` flags
- **Do NOT modify files in `src/` directly**: Changes should go to the respective submodule repos
