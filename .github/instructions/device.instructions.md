---
applyTo: "device/**"
---

# Instructions for `device/`

`device/<vendor>/<platform>/` holds the runtime configuration of one switch model, and `device/common/` holds files shared by many of them. Build rules belong in `platform/`, not here.

## Layout

- The platform directory name is the ONIE platform string: `<arch>-<name>-r<revision>`, for example `x86_64-cel_seastone_2-r0`. It must match what the switch reports.
- Typical platform files: `default_sku` (`<HWSKU> t1`), `platform_asic`, `platform.json`, `platform_components.json`, `installer.conf`, `pcie.yaml`, `pmon_daemon_control.json`, `system_health_monitoring_config.json` and the reboot hooks (`platform_reboot`, `pre_reboot_hook`, ...).
- Each HWSKU is a subdirectory named exactly like the value in `default_sku`. It holds `port_config.ini`, `hwsku.json`, `buffers.json.j2`, `qos.json.j2`, `buffers_defaults_{def,t0,t1}.j2`, `pg_profile_lookup.ini`, `sai.profile.j2` and the ASIC config (`*.config.bcm`, ...).
- New platforms implement the Platform API (`sonic_platform` package, built from `platform/<name>/sonic-platform-modules-<vendor>`). Do not add new legacy `plugins/` utilities.

## Keep changesets small

- Do not copy templates that already exist. Share them with relative symlinks: to a profile in `device/common/profiles/<asic>/...` (`BALANCED -> ../../../common/profiles/td2/gen/BALANCED`, `qos.json.j2 -> BALANCED/qos.json.j2`) or to a sibling HWSKU (`../Other-SKU/phy2_config.json`).
- Use relative symlinks only, and keep them inside `device/`.
- When only a few values differ between SKUs, parameterize the shared Jinja2 template instead of forking it.

## Files and binaries

- Keep JSON, Jinja2 and INI files valid: JSON without trailing commas, templates that render with `sonic-cfggen`, `port_config.ini` columns aligned with the header.
- Use LF line endings and no trailing whitespace.
- Do not commit binaries. The only accepted binary files are the ones in `.github/binary-artifacts-allowlist.txt` (Broadcom LED microcode, Cancun packages, vendor firmware headers). A pull request that adds any other binary fails the `binary-artifacts-check` workflow, so add the path to the allowlist with a comment explaining why, or build the file at build time.

## Pull requests

- Commit format: `[device/<vendor>]: Short description`. Every commit needs a `Signed-off-by:` line (`git commit -s`).
- Keep a pull request to one vendor or platform and one logical change. Describe how it was verified (image version, platform tested) in the PR template.
