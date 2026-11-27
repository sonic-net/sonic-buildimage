# U-Boot env vendor hook

`sonic-program-uboot-env.sh` (aspeed-platform-services) writes the
vendor-neutral U-Boot environment on every Aspeed BMC. A platform that needs to
change it, for example to gate autoboot on a board-specific U-Boot command
through `boot_guard`, ships a vendor hook. The TFTP installer ramdisk is shared
by every Aspeed board, so the script runs only the hook of the platform it
detected.

## Adding a hook

- Put an executable script named `sonic-uboot-env-vendor-hook` in the
  platform's device directory, next to its `installer.conf`:
  `device/<vendor>/<platform>/sonic-uboot-env-vendor-hook`. Commit it with mode
  100755; a file without the executable bit is skipped with a warning.
- The platform directory must be named `arm64-*` and contain `installer.conf`.
  `create_tftp_image.sh` copies hooks into the installer ramdisk only from such
  directories.
- Boards of one family can share a script through a relative symlink to
  another platform directory of the same vendor.

Example: `device/nexthop/arm64-nexthop_b27-r0/sonic-uboot-env-vendor-hook`.

## When it runs

The hook runs at the end of `sonic-program-uboot-env.sh`, after the defaults
are written, each time the U-Boot environment is programmed:

- at the end of a TFTP install, from the installer ramdisk
  (`/device/<vendor>/<platform>/`);
- on the installed system's first boot, from
  `/usr/share/sonic/device/<platform>/`, while `/host/.uboot-env-initialized`
  does not exist.

So it runs twice after a TFTP install and must be idempotent. A SONiC upgrade
does not run it again, because the marker stays on `/host`. On the installed
system nothing is programmed when the eMMC has an ONIE partition.

The hook is found only when the platform is detected:

- In the installer ramdisk, `sonic-machine-conf-init.sh` maps the device tree
  to the platform name. Add your board there first.
- On the installed system, the platform comes from `/host/machine.conf`.

## What the hook can rely on

- `fw_setenv` and `fw_printenv` on `PATH`.
- `SONIC_UBOOT_ENV_LOG_FILE`, to append log lines in the same style as the
  script.
- Nothing from the vendor's platform package: it is not installed in the
  installer ramdisk, which has busybox and a few tools only. Write the hook for
  POSIX `sh`.
- A non-zero exit is logged as a warning and the install continues.

Keep `boot_guard` defined and set it only to a command your U-Boot has:
`bootcmd` runs `if run boot_guard`, so anything else stops every boot at the
U-Boot prompt.
