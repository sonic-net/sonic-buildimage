#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# The firmware tools component.py runs, faked, and the files they and the
# components read -- for the component tests (`component_tests.rs`), and
# for the run of Python's component.py that gave their expected answers.
#
#   R     the root everything lives under: empty for that Python run, the
#         test's io root for the Rust one
#   BIN   where the tools run by a bare name go: on the Python run's PATH,
#         or the root's /usr/bin, where the Rust test looks for them
#   PLAT  the platform
#
# A tool is written with $R spelled out in it, as the code that runs it
# passes it no environment.  Each logs its argv to $C/calls and exits with
# what $C/rc_<name> holds (0 if nothing; "kill" is SIGKILL).  tar and rm
# (and, for the Rust run, /bin/bash) pass through to the real ones with every
# absolute path moved under $R; grep and awk pass through as they are.
set -e
C=$R/tmp/c
rm -rf "$C" "$R"/tmp/00-* "$R"/tmp/99-* "$R"/tmp/mpfa-* "$R/lib/onie" "$R/mnt/onie-fs" "$R/var/run/hw-management" \
    "$R/etc/sonic/bmc.json" "$R/usr/share/sonic/platform"
mkdir -p "$C/fw" "$C/orig" "$BIN" "$R/usr/bin/asic_detect" "$R/usr/local/bin" "$R/var/run/hw-management/config" \
    "$R/var/run/hw-management/system" "$R/tmp" "$R/mnt" "$R/lib" "$R/usr/share/sonic/platform" "$R/host" "$R/etc/sonic"
: > "$C/calls"

cat > "$C/lib.sh" <<'EOF'
rc() { v=$(cat "$C/rc_$1" 2>/dev/null || echo 0); [ "$v" = kill ] && kill -9 $$; exit "$v"; }
EOF
# Every absolute argument moved under $R -- inline, as a function's `set --`
# would only reach its own arguments.
cat > "$C/under_root.sh" <<'EOF'
n=$#; for a; do case $a in /*) a=$R$a ;; esac; set -- "$@" "$a"; done; shift "$n"
EOF
# tool PATH: the body from stdin, after the prelude; a pristine copy kept.
tool() {
  { printf '#!/bin/sh\nR=%s\nC=$R/tmp/c\n' "'$R'"; cat "$C/lib.sh"; cat; } > "$1"
  chmod 755 "$1"
  cp "$1" "$C/orig/$(basename "$1")"
}

{ cat "$C/under_root.sh"; echo 'exec /usr/bin/tar "$@"'; } | tool "$BIN/tar"
{ cat "$C/under_root.sh"; echo 'exec /usr/bin/rm "$@"'; } | tool "$BIN/rm"
tool "$BIN/grep" <<'EOF'
exec /usr/bin/grep "$@"
EOF
tool "$BIN/awk" <<'EOF'
exec /usr/bin/awk "$@"
EOF
if [ -n "$R" ]; then
  mkdir -p "$R/bin"
  { cat "$C/under_root.sh"; echo 'exec /bin/bash "$@"'; } | tool "$R/bin/bash"
fi
tool "$BIN/fdisk" <<'EOF'
echo "fdisk $*" >> "$C/calls"
cat "$C/fdisk" 2>/dev/null
rc fdisk
EOF
# ONIE's file system is not mounted: the mount point stays an empty
# directory, which the unmount removes.  With $C/mount_copy, ONIE's files are
# copied there instead -- and, the mount point being no mount point, Python
# and the Rust alike then fail to remove it, after reading them.
tool "$BIN/mount" <<'EOF'
echo "mount $*" >> "$C/calls"
v=$(cat "$C/rc_mount" 2>/dev/null || echo 0); [ "$v" != 0 ] && rc mount
for last; do :; done
[ -e "$C/mount_copy" ] && cp -a "$C/onie-fs/." "$R$last/"
exit 0
EOF
tool "$BIN/umount" <<'EOF'
echo "umount $*" >> "$C/calls"
EOF
tool "$BIN/dmidecode" <<'EOF'
echo "dmidecode $*" >> "$C/calls"
if [ "$1" = -t0 ]; then cat "$C/dmi_t0" 2>/dev/null; else cat "$C/dmi_oem" 2>/dev/null; fi
echo "dmi stderr" >&2
rc dmi
EOF
tool "$BIN/cpldupdate" <<'EOF'
echo "cpldupdate $*" >> "$C/calls"
echo "cpldupdate: 100%"
[ -e "$C/cpld_hook" ] && . "$C/cpld_hook"
rc cpld
EOF
tool "$R/usr/bin/mlnx-onie-fw-update.sh" <<'EOF'
echo "onie-fw-update $*" >> "$C/calls"
case "$1" in
  show-pending) cat "$C/pending" 2>/dev/null; echo "(from stderr)" >&2; rc show ;;
  add) echo "Staged $2"; [ -e "$C/stage_adds" ] && echo "$(basename "$2") | 1 | x" >> "$C/pending"; rc add ;;
  remove) echo "Removed $2"; rc remove ;;
  update) echo "Updating $*"; rc update ;;
  *) rc other ;;
esac
EOF
tool "$R/usr/bin/mlnx-ssd-fw-update.sh" <<'EOF'
echo "ssd $*" >> "$C/calls"
case "$*" in
  "-q") cat "$C/ssd_q" 2>/dev/null; rc ssd_q ;;
  "-q -i "*) cat "$C/ssd_qi" 2>/dev/null; rc ssd_qi ;;
  *) echo "ssd: installing"; rc ssd_install ;;
esac
EOF
tool "$R/usr/bin/mst" <<'EOF'
echo "mst $*" >> "$C/calls"
echo "mst $1: ok"
rc "mst_$1"
EOF
tool "$R/usr/bin/asic_detect/asic_detect.sh" <<'EOF'
echo "asic_detect $*" >> "$C/calls"
if [ "$1" = -m ]; then cat "$C/mstdev" 2>/dev/null; echo "asic -m stderr" >&2; rc asic_m; fi
cat "$C/asic" 2>/dev/null
rc asic
EOF
tool "$R/usr/local/bin/reboot" <<'EOF'
sid=$(cut -d' ' -f6 /proc/$$/stat)
echo "reboot $* session=$([ "$sid" = $$ ] && echo new || echo old)" >> "$C/calls"
echo "reboot (fake)"
rc reboot
EOF

# The platform: machine.conf for the Rust run (the Python one has $PLATFORM),
# the DPUs platform.json lists, a BMC platform's environment.
printf 'onie_platform=%s\n' "$PLAT" > "$R/host/machine.conf"
case $PLAT in
  *sn4280*) echo '{"DPUS": {"dpu0": {}, "dpu1": {}, "dpu2": {}, "dpu3": {}}}' ;;
  *) echo '{}' ;;
esac > "$R/usr/share/sonic/platform/platform.json"
case $PLAT in
  *sn6600_ld*)
    echo switch_host=1 > "$R/usr/share/sonic/platform/platform_env.conf"
    printf '{\n    "bmc_if_name": "usb0",\n    "bmc_if_addr": "169.254.0.2",\n    "bmc_addr": "169.254.0.1",\n    "bmc_net_mask": "255.255.255.252"\n}\n' \
      > "$R/usr/share/sonic/platform/bmc.json" ;;
esac

# ONIE
printf 'Disk /dev/sda: 32 GiB\nDevice     Start    End Sectors  Size Type\n/dev/sda1   2048   6143    4096    2M ONIE boot\n/dev/sda2   6144 268287  262144  128M EFI System\n' > "$C/fdisk"
mkdir -p "$C/onie-fs/onie/grub" "$C/onie-fs/onie/tools/lib/onie"
printf 'onie_machine=mlnx_x86\nonie_version=2025.05-5.3.0021-115200-dev\nonie_platform=x86_64-mlnx_x86-r0\n' > "$C/onie-fs/onie/grub/grub-machine.cfg"
echo 'No pending firmware updates present' > "$C/pending"
printf '#!/bin/sh\necho "image_version=2025.08-5.3.0022-115200"\necho "image_build_date=2025-08-01"\nexit ${ONIE_IMG_RC:-0}\n' > "$C/fw/onie-updater-x86_64-mlnx_x86-r0"
cp "$C/fw/onie-updater-x86_64-mlnx_x86-r0" "$C/fw/bios.rom"
printf 'payload' > "$C/fw/bios.cab"; printf 'payload' > "$C/fw/bios.bin"
# SSD and BIOS
printf 'Device Model: SSD123\nFirmware Version: 0115\nCapacity: 32GB\n' > "$C/ssd_q"
printf 'Firmware Version: 0115\nAvailable Firmware Version: 0202\nUpgrade Required: yes\nPower Cycle Required: yes\n' > "$C/ssd_qi"
printf 'payload' > "$C/fw/ssd.pkg"; printf 'payload' > "$C/fw/ssd.bin"
printf 'NVIDIA-BIOS 0ACQF.03.07\n' > "$C/dmi_oem"
printf '# dmidecode 3.5\nBIOS Information\n\tVendor: AMI\n\tVersion: 0ACLH003_02.02.007\n\tRelease Date: 01/02/2026\n' > "$C/dmi_t0"
# CPLDs, their images and MPFAs
H=$R/var/run/hw-management
echo 4 > "$H/config/cpld_num"
for i in 1 2 3 4; do
  echo "00$i" > "$H/system/cpld${i}_pn"; echo $((i + 4)) > "$H/system/cpld${i}_version"; echo 1 > "$H/system/cpld${i}_version_min"
done
rm "$H/system/cpld2_version" "$H/system/cpld3_pn" "$H/system/cpld3_version_min"
echo 123 > "$H/system/cpld4_version"; echo 1234567 > "$H/system/cpld4_pn"
printf 'payload' > "$C/fw/cpld.vme"; printf 'payload' > "$C/fw/cpld.bin"
mk() {  # mk NAME METADATA [FILES...]: an MPFA of a metadata text and files
  d=$C/mpfa-src/$1; mkdir -p "$d"; printf "$2" > "$d/metadata.ini"; shift 2
  for f in "$@"; do printf 'vme' > "$d/$f"; done
  /usr/bin/tar czf "$C/fw/$(basename "$d").mpfa" -C "$d" .
}
mk good '[version]\nCPLD1 = CPLD000085_REV0700\ncpld2: CPLD000086_REV0800\nDPU1_FPGA = FPGA000001_REV0100\n\n[firmware]\nburn = burn.vme\nrefresh = refresh.vme\n' burn.vme refresh.vme
mk noburn '[version]\nCPLD1=X\n[firmware]\nrefresh=refresh.vme\n' refresh.vme
mk norefresh '[version]\nCPLD1=X\n[firmware]\nburn=burn.vme\n' burn.vme
mk nohdr 'CPLD1 = X\n[version]\n'
mk dup '[version]\nCPLD1=X\ncpld1=Y\n'
mk dupsec '[version]\nCPLD1=X\n[version]\n'
mk parse '[version]\nCPLD1=X\njunk line\n[firmware]\nmore junk\nburn=b.vme\n' b.vme
mk interp '[DEFAULT]\nroot=/opt\n[version]\nCPLD1=%%(root)s/v1 %%%% done\nCPLD2=%%(nope)s\nCPLD3=50%%\nCPLD4=%%(cpld1)s+\n[firmware]\nburn=%%(root)s.vme\n'
mk multi '[version]\nCPLD1 = first\n   second line\n\n   third\n# comment\n; other\nCPLD2 =\n[firmware]\nburn = ../burn.vme\n' burn.vme
mkdir -p "$C/mpfa-src/nometa"; printf 'x' > "$C/mpfa-src/nometa/x"; /usr/bin/tar czf "$C/fw/nometa.mpfa" -C "$C/mpfa-src/nometa" .
printf 'not a tarball' > "$C/fw/broken.mpfa"
cp "$C/fw/good.mpfa" "$C/fw/good.tgz"
# DPUs (SN4280)
echo 4 > "$H/config/dpu_num"
for i in 1 2 3 4; do
  mkdir -p "$H/dpu$i/system"
  echo "00$i" > "$H/dpu$i/system/fpga1_pn"; echo 2 > "$H/dpu$i/system/fpga1_version"; echo 3 > "$H/dpu$i/system/fpga1_version_min"
done
echo 5 > "$H/dpu1/system/boot_progress"; echo 3 > "$H/dpu2/system/boot_progress"; echo 5 > "$H/dpu4/system/boot_progress"
echo spc4 > "$C/asic"; echo /dev/mst/mt53100_pci_cr0 > "$C/mstdev"
