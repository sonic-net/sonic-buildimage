#!/usr/bin/env python3
"""
Filter systemd services for Aspeed BMC platform.

This script removes services that are not needed on BMC platform
and ensures BMC-specific services are included.
"""

import sys
import os
import glob

# Services to REMOVE for BMC platform (not needed)
BMC_EXCLUDED_SERVICES = {
    # AAA/TACACS - not needed on BMC
    'tacacs-config.service',
    'tacacs-config.timer',

    # Backend ACL - not needed on BMC
    'backend-acl.service',

    # PCIe - not needed on BMC
    'pcie-check.service',

    # Smart switch / midplane - not needed on BMC
    'midplane-network-npu.service',
    'midplane-network-dpu.service',

    # Virtual switch specific
    'topology.service',

    # NOS-specific services not needed for BMC
    'copp-config.service',           # Control Plane Policing - no ASIC on BMC
    'dhcp_dos_logger.service',       # DHCP DoS detection - no switch ports on BMC
    'warmboot-finalizer.service',    # Warm boot reconciliation - no SWSS/BGP on BMC

    # SONiC NOS services - not needed for BMC
    'sonic-hostservice.service',     # SONiC host services (VLAN, LAG, etc.) - NOS-specific
    'featured.service',              # SONiC feature management - NOS-specific
    'featured.timer',                # Timer for featured service
    'hostcfgd.service',              # SONiC host config daemon - NOS-specific
    'hostcfgd.timer',                # Timer for hostcfgd service

    # UUID daemon - not needed
    'uuidd.service',                 # UUID daemon - kernel provides UUID generation
    'uuidd.socket',                  # UUID daemon socket activation

    # Console - not needed if no local display
    'getty@tty1.service',            # Local console - only if VGA/HDMI available

    # Optional services - can be excluded for minimal BMC
    'auditd.service',                # Security audit - only needed for compliance
}

# Services to ENSURE are included for BMC
BMC_REQUIRED_SERVICES = {
    # BMC-specific services (core functionality)
    'bmc-init.service',

    # Security and monitoring (important for BMC)
    'caclmgrd.service',               # Firewall for management interfaces
    'haveged.service',                # Entropy for SSH/SSL/TLS
    'monit.service',                  # Auto-restart failed services
    'procdockerstatsd.service',       # Container resource monitoring

    # Time synchronization
    'ntp.service',                    # NTP time synchronization

    # Zero Touch Provisioning
    'ztp.service',                    # ZTP for automated device provisioning

    # System health monitoring
    'system-health.service',          # SONiC system health monitor

    # Telemetry and monitoring
    'gnmi.service',                   # gNMI telemetry service
}

def create_service_override(filesystem_root, service_name, override_content, enable=True):
    """Create systemd drop-in override for a service on BMC."""

    systemd_system = os.path.join(filesystem_root, 'etc/systemd/system')
    override_dir = os.path.join(systemd_system, f'{service_name}.d')
    override_file = os.path.join(override_dir, 'bmc-override.conf')

    # Create override directory
    os.makedirs(override_dir, exist_ok=True)

    try:
        with open(override_file, 'w') as f:
            f.write(override_content)
        print(f"  Created: {override_file}")

        # Create symlink in multi-user.target.wants to enable auto-start if requested
        if enable:
            multi_user_wants = os.path.join(systemd_system, 'multi-user.target.wants')
            os.makedirs(multi_user_wants, exist_ok=True)

            service_symlink = os.path.join(multi_user_wants, service_name)
            service_path = f'/usr/lib/systemd/system/{service_name}'

            # Remove existing symlink if present
            if os.path.islink(service_symlink) or os.path.exists(service_symlink):
                os.remove(service_symlink)

            # Create symlink to enable service
            os.symlink(service_path, service_symlink)
            print(f"  Enabled: {service_name} -> multi-user.target.wants")

        return True
    except Exception as e:
        print(f"  Warning: Could not create {service_name} override: {e}")
        return False

def create_gnmi_bmc_override(filesystem_root):
    """Create systemd drop-in override for GNMI to remove swss/syncd dependencies on BMC."""

    print("")
    print("Creating GNMI service override for BMC (removing swss/syncd dependencies)...")

    override_content = """[Unit]
# BMC override: Remove ASIC/switch dependencies (swss, syncd)
# GNMI on BMC only needs database service
After=database.service

[Install]
# Enable GNMI to start automatically on BMC
WantedBy=multi-user.target
"""

    return create_service_override(filesystem_root, 'gnmi.service', override_content, enable=True)

def create_watchdog_control_bmc_override(filesystem_root):
    """Create systemd drop-in override for watchdog-control to remove swss dependency on BMC."""

    print("")
    print("Creating watchdog-control service override for BMC (removing swss dependency)...")

    override_content = """[Unit]
# BMC override: Remove ASIC/switch dependency (swss)
# Watchdog control on BMC doesn't need swss
After=

[Install]
WantedBy=multi-user.target
"""

    return create_service_override(filesystem_root, 'watchdog-control.service', override_content, enable=True)

def create_determine_reboot_cause_bmc_override(filesystem_root):
    """Create systemd drop-in override for determine-reboot-cause to remove rc-local dependency on BMC."""

    print("")
    print("Creating determine-reboot-cause service override for BMC (removing rc-local dependency)...")

    override_content = """[Unit]
# BMC override: Remove rc-local dependency
# Reboot cause determination on BMC doesn't need rc-local
Requires=
After=

[Install]
WantedBy=multi-user.target
"""

    return create_service_override(filesystem_root, 'determine-reboot-cause.service', override_content, enable=True)

def mask_services_in_filesystem(filesystem_root):
    """Mask excluded services by removing symlinks and creating mask files."""

    print("")
    print("Masking excluded services in filesystem...")

    systemd_system = os.path.join(filesystem_root, 'etc/systemd/system')

    masked_count = 0
    for service in BMC_EXCLUDED_SERVICES:
        # Remove symlinks from target.wants directories
        for wants_dir in glob.glob(os.path.join(systemd_system, '*.wants')):
            symlink = os.path.join(wants_dir, service)
            if os.path.islink(symlink) or os.path.exists(symlink):
                try:
                    os.remove(symlink)
                    print(f"  Removed: {symlink}")
                    masked_count += 1
                except Exception as e:
                    print(f"  Warning: Could not remove {symlink}: {e}")

        # Create mask file (symlink to /dev/null)
        mask_file = os.path.join(systemd_system, service)
        if not os.path.exists(mask_file):
            try:
                os.symlink('/dev/null', mask_file)
                print(f"  Masked: {service}")
                masked_count += 1
            except Exception as e:
                print(f"  Warning: Could not mask {service}: {e}")

    print(f"Total masked/disabled: {masked_count} service links")
    return masked_count

def filter_services(input_file, output_file, filesystem_root=None):
    """Filter services for BMC platform."""

    # Read existing services
    try:
        with open(input_file, 'r') as f:
            services = set(line.strip() for line in f if line.strip() and not line.startswith('#'))
    except FileNotFoundError:
        services = set()

    # Remove excluded services
    filtered_services = services - BMC_EXCLUDED_SERVICES

    # Add required BMC services
    filtered_services.update(BMC_REQUIRED_SERVICES)

    # Sort for consistent output
    sorted_services = sorted(filtered_services)

    # Write filtered services (NO COMMENTS - systemd-sonic-generator can't handle them)
    with open(output_file, 'w') as f:
        for service in sorted_services:
            f.write(service + '\n')

    # Mask services in filesystem if root provided
    if filesystem_root:
        mask_services_in_filesystem(filesystem_root)
        # Create service overrides to remove dependencies that don't exist on BMC
        create_gnmi_bmc_override(filesystem_root)
        create_watchdog_control_bmc_override(filesystem_root)
        create_determine_reboot_cause_bmc_override(filesystem_root)

    # Print summary to build log (not to file)
    print("=" * 80)
    print("Aspeed BMC Service Filtering Summary")
    print("=" * 80)
    print(f"Original services: {len(services)}")
    print(f"Filtered services: {len(filtered_services)}")
    print(f"")
    print(f"Services REMOVED for BMC:")
    removed = BMC_EXCLUDED_SERVICES & services
    if removed:
        for svc in sorted(removed):
            print(f"  - {svc}")
    else:
        print(f"  (none)")
    print(f"")
    print(f"Services ADDED for BMC:")
    added = BMC_REQUIRED_SERVICES - services
    if added:
        for svc in sorted(added):
            print(f"  + {svc}")
    else:
        print(f"  (none)")
    print(f"")
    print(f"Excluded services list: {', '.join(sorted(BMC_EXCLUDED_SERVICES))}")
    print("=" * 80)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: aspeed_bmc_filter_services.py <input_file> <output_file> [filesystem_root]")
        sys.exit(1)

    filesystem_root = sys.argv[3] if len(sys.argv) > 3 else None
    filter_services(sys.argv[1], sys.argv[2], filesystem_root)

