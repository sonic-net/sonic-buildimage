#!/bin/bash
# Install already-built DEBs in a disposable native Trixie system; never builds the kernel.
set -euo pipefail
packages=${1:?kernel DEB directory required}
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install --no-install-recommends -y python3 git gcc-14 make binutils gdb pahole \
    libelf-dev libssl-dev perl initramfs-tools kmod linux-base busybox cpio xz-utils zstd
python3 -B tools/bazel/kernel/prepare.py verify --debs-directory "$packages" >/tmp/kernel-package-verification.json
# APT resolves package dependencies and dpkg executes real kernel maintainer hooks.
python3 - "$packages" <<'PYTHON'
import json, pathlib, subprocess, sys
directory = pathlib.Path(sys.argv[1])
manifest = json.loads((directory / 'kernel-packages.json').read_text())
subprocess.run(['apt-get', 'install', '--no-install-recommends', '-y',
                *(str(directory / item['name']) for item in manifest['packages'])], check=True)
PYTHON
python3 - "$packages/kernel-packages.json" <<'PYTHON'
import json, subprocess, sys
manifest = json.load(open(sys.argv[1]))
assert subprocess.check_output(['dpkg', '--print-architecture'], text=True).strip() == manifest['architecture']
for item in manifest['packages']:
    actual = subprocess.check_output(['dpkg-query', '-W', '-f=${Status}\t${Version}\t${Architecture}', item['package']], text=True)
    assert actual == 'install ok installed\t' + item['version'] + '\t' + item['architecture'], item['name']
assert not subprocess.check_output(['dpkg', '--audit'], text=True).strip()
PYTHON
architecture=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["architecture"])' "$packages/kernel-packages.json")
gdb --version
python3 -B src/sonic-linux-kernel/tools/bazel/verify_debug.py --architecture "$architecture" --root / --output /tmp/kernel-debugger.json
python3 -B - "$packages" <<'PYTHON'
from pathlib import Path
import sys
sys.path.insert(0, str(Path.cwd() / "scripts"))
import sbom_kernel_deb
component = sbom_kernel_deb.component(sys.argv[1], Path.cwd(), "/")
assert component["purl"].startswith("pkg:deb/sonic/linux-image-")
PYTHON
abi=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["kernel_abi"])' "$packages/kernel-packages.json")
module_dir=$(mktemp -d /tmp/kernel-module-smoke.XXXXXX)
cat > "$module_dir/sonic_kernel_smoke.c" <<'SOURCE'
#include <linux/init.h>
#include <linux/module.h>
static int __init sonic_kernel_smoke_init(void) { return 0; }
static void __exit sonic_kernel_smoke_exit(void) { }
module_init(sonic_kernel_smoke_init);
module_exit(sonic_kernel_smoke_exit);
MODULE_LICENSE("GPL");
SOURCE
printf 'obj-m := sonic_kernel_smoke.o\n' > "$module_dir/Makefile"
make -C "/lib/modules/$abi/build" M="$module_dir" CC=gcc-14 modules
[[ $(modinfo -F vermagic "$module_dir/sonic_kernel_smoke.ko") == "$abi "* ]]
# Debian headers retain MODULE_SIG_ALL; an external build cannot access the
# original private key. Keep Kbuild's standard tolerated external-signing path.
install_root=$(mktemp -d /tmp/kernel-module-installed.XXXXXX)
make -C "/lib/modules/$abi/build" M="$module_dir" CC=gcc-14 \
    INSTALL_MOD_PATH="$install_root" modules_install
installed_module=$(find "$install_root/lib/modules/$abi" -type f -name 'sonic_kernel_smoke.ko*')
[[ -n $installed_module && $(modinfo -F vermagic "$installed_module") == "$abi "* ]]
cat files/initramfs-tools/modules >> /etc/initramfs-tools/modules
# The image postinst must create the initial initramfs and boot links.
test -s "/boot/initrd.img-$abi"
# Exercise the update path used after SONiC image hooks.
depmod -a "$abi"
update-initramfs -u -k "$abi"
lsinitramfs "/boot/initrd.img-$abi" > "$module_dir/initramfs-files.txt"
grep -F "lib/modules/$abi/" "$module_dir/initramfs-files.txt" > /dev/null
[[ $(readlink -f /vmlinuz) == "/boot/vmlinuz-$abi" ]]
[[ $(readlink -f /initrd.img) == "/boot/initrd.img-$abi" ]]
test -s "/usr/lib/debug/boot/vmlinux-$abi"
printf 'Kernel DEB installation, package hooks, external module build and initramfs update passed for %s.\n' "$abi"
