# Docker Routing Config Mode Guide

## Overview

This document describes the configuration modes for FRR routing in SONiC, including recent changes to default behavior and deprecation notices.

## Configuration Mode Changes

### Default Behavior Change

**Previously:** The `docker_routing_config_mode` field in `DEVICE_METADATA` was required to specify the configuration mode.

**Now:** If `docker_routing_config_mode` is missing from `DEVICE_METADATA`, the system defaults to `"unified"` mode.

This change simplifies configuration for new deployments, as unified mode is the recommended and supported configuration approach.

### Deprecation of "separated" Mode

**Status:** Deprecated

The `"separated"` configuration mode is now deprecated. When explicitly set to `"separated"`:

- A deprecation warning is logged: `Config Type 'separated' is deprecated. The system will use unified mode instead.`
- The system automatically falls back to unified mode behavior
- Per-daemon configuration files (e.g., `bgpd.conf`, `zebra.conf`, `staticd.conf`) are no longer generated
- All routing configuration is rendered into the unified `frr.conf` file

**Migration Path:** Operators should remove any explicit `"separated"` configuration and rely on the default unified mode.

## Impact on Tooling

### Per-Daemon Config File Dependencies

If your tooling or automation depends on per-daemon configuration files:

1. **Update tooling to read from unified `frr.conf`:** The unified configuration file contains all routing daemon configurations in a single file, following FRR's integrated configuration format. Parse this file instead of individual daemon configs.

2. **Use FRR vtysh for programmatic access:** Instead of parsing individual daemon config files, use FRR's vtysh for querying configuration:
   ```bash
   # Get full running configuration
   vtysh -c "show running-config"

   # Get specific daemon configuration
   vtysh -c "show running-config bgp"
   vtysh -c "show running-config zebra"
   vtysh -c "show running-config static"

   # Get specific configuration sections with JSON output
   vtysh -c "show running-config json"
   ```

3. **Monitor Config DB changes:** Subscribe to Config DB updates via redis or the FRR management framework (frrcfgd) for configuration change notifications.

4. **Template-based configuration:** In unified mode, configurations are generated from Jinja2 templates in `/usr/share/sonic/templates/`. Review these templates to understand the configuration structure.

### Configuration Restore Behavior

The `use_template_render_for_restore` field in `DEVICE_METADATA` controls how FRR configurations are restored during startup in unified mode:

- **Default (`true`):** Configurations are regenerated from templates using sonic-config-engine. This ensures consistency with the Config DB state.
- **When set to `false`:** Configurations are restored from cached DB tables in the FRR management framework, replaying configuration changes incrementally.

## Configuration Examples

### Minimal Configuration (Recommended)

No `docker_routing_config_mode` needed - defaults to unified:

```json
{
  "DEVICE_METADATA": {
    "localhost": {
      "hostname": "switch01"
    }
  }
}
```

### Explicit Unified Configuration

```json
{
  "DEVICE_METADATA": {
    "localhost": {
      "hostname": "switch01",
      "docker_routing_config_mode": "unified"
    }
  }
}
```

### With Custom Restore Behavior

```json
{
  "DEVICE_METADATA": {
    "localhost": {
      "hostname": "switch01",
      "docker_routing_config_mode": "unified",
      "use_template_render_for_restore": false
    }
  }
}
```

## Verification

To verify your current configuration mode:

```bash
# Check DEVICE_METADATA
redis-cli -n 4 HGET "DEVICE_METADATA|localhost" docker_routing_config_mode

# Check generated FRR configuration
cat /etc/frr/frr.conf

# Check for deprecation warnings in logs
grep "separated is deprecated" /var/log/syslog
```

## Support

For questions or issues related to this configuration:
- Review the SONiC documentation for FRR configuration
- Check the FRR management framework logs in `/var/log/syslog`
- Refer to the template files in `/usr/share/sonic/templates/` for configuration structure
