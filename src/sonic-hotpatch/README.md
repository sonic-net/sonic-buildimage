# SONiC Hotpatch

Hotpatch delivers targeted fixes to a running SONiC device without a full image
upgrade. See the HLD for the full design: [SONiC Hotpatch HLD][hld].

[hld]: https://github.com/sonic-net/SONiC/pull/2323

## Layout

| Path          | Built into the image | Purpose                                                                 |
| :------------ | :------------------- | :---------------------------------------------------------------------- |
| `libcareplus/`| yes (Debian package) | Userspace live-patching engine used by the `func_hotpatch` patch type   |
| `tools/`      | no                   | Offline toolkit used on a build host to assemble hotpatch packages       |

`tools/` is source-only: it runs on a build host when a hotpatch is produced and
is never installed onto a device.

## Building a hotpatch

Place the payload (docker image tarballs, `.deb` files, scripts, `.kpatch`
files, `Dockerfile`) into a `packages/` directory, then build a sub-patch:

```
./build.sh --patch-name sub-Hotfix1
```

`build.sh` calls `generate.py` to derive a `patch_info.yml` manifest from the
payload, opens it in `$EDITOR` so the remaining fields can be filled in (set
`CI=true` to skip the editor), validates it with `check.py`, and packs the
result into a sub-patch archive.

Sub-patches are then assembled into a distributable Hotfix package:

```
# first Hotfix
./package.py -a <sonic_version> -n Hotfix1-<sonic_version> -s sub-Hotfix1.tar.gz

# subsequent Hotfix, carrying forward the sub-patches of the previous one
./package.py -o Hotfix1-<sonic_version>.tar.gz -n Hotfix2-<sonic_version> -s sub-Hotfix2.tar.gz
```

`package.py` maintains `summary.yml` (the OS version plus every sub-patch and
its md5sum) and emits the Hotfix archive together with its `.md5` file.

Use `-r/--replace-sub-patches` to swap the contents of a sub-patch that already
exists in the previous Hotfix; the entry keeps its position in `summary.yml` and
its md5sum is recomputed.

## Installation plugin

`install.py` is the runtime plugin. It is copied into every sub-patch archive
rather than installed on the device: `sonic-installer` extracts a Hotfix package
and dynamically imports the `install.py` carried inside it. The plugin must
define:

```
SUCCESS = 0
FAIL = 1

get_patch_name()       # patch name
get_patch_desc()       # patch description
do_patch_install()     # apply the patch,   returns (status, output)
do_patch_uninstall()   # roll the patch back, returns (status, output)
```

Because the plugin travels with the patch, an older device image can install a
Hotfix built by a newer toolkit as long as this interface is honored.

## Reapplying function hotpatches after reboot

Function hotpatches live in process memory and disappear when the system
reboots. Sub-patch archives persisted by `sonic-installer` under
`/usr/share/sonic/hotpatches/` are reapplied by the
`hotpatches-auto-install.service` oneshot unit.

The service waits for `PORT_TABLE:PortInitDone` in APPL_DB, orders archives by
their Hotfix number, and invokes:

```
sonic-installer hotpatch-install-single <archive> -y
```

The `hotpatch-install-single` command is provided by the corresponding
sonic-utilities change. If no persisted archives exist, the service exits
successfully without accessing Redis or invoking the CLI.
