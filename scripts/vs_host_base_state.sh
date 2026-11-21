#!/bin/bash
# Cache the stock Trixie VS host before applying installer-specific settings.
set -euo pipefail

fail() { echo "VS host base: $*" >&2; exit 1; }

case ${1:-} in
check-inputs)
    [[ ${USERNAME:-} == admin && ${TARGET_MACHINE:-} == vs && ${IMAGE_TYPE:-} == onie &&
       ${CONFIGURED_PLATFORM:-} == vs && ${CONFIGURED_ARCH:-} == amd64 &&
       ${IMAGE_DISTRO:-} == trixie && ${TARGET_BOOTLOADER:-} == grub &&
       ${RFS_SPLIT_FIRST_STAGE:-} == y && ${RFS_SPLIT_LAST_STAGE:-} == n &&
       ${RFS_SQUASHFS_NAME:-} == sonic-vs.bin__vs__rfs.squashfs &&
       ${CROSS_BUILD_ENVIRON:-} != y && ${MULTIARCH_QEMU_ENVIRON:-} != y &&
       ${INCLUDE_KUBERNETES:-} != y && ${INCLUDE_KUBERNETES_MASTER:-} != y ]] ||
       fail "expected the native AMD64 VS installer first stage"

    for name in PASSWORD DEFAULT_PASSWORD BMC_NOS_ACCOUNT_USERNAME BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD CHANGE_DEFAULT_PASSWORD \
                http_proxy https_proxy all_proxy HTTP_PROXY HTTPS_PROXY ALL_PROXY \
                SONIC_DEBIAN_EXTRA_ENV sed_tpm_bank_a sed_tpm_bank_b; do
        [[ -z ${!name:-} ]] || fail "site or account input present: $name"
    done
    for file in rules/config.organization rules/config.user files/build_templates/build_debian.organization.sh files/image_config/sonic_release; do
        [[ ! -e $file && ! -L $file ]] || fail "site input present: $file"
    done
    # The defaults choose timestamped public Debian mirrors in the VS workflow.
    [[ ${MIRROR_SNAPSHOT:-} == y &&
       ${BUILD_SNAPSHOT_URL:-} == http://packages.trafficmanager.net/snapshot &&
       ${BUILD_PUBLIC_URL:-} == https://packages.trafficmanager.net/public &&
       ${BUILD_PACKAGES_URL:-} == https://packages.trafficmanager.net/packages &&
       ${TRUSTED_GPG_URLS:-} == https://packages.microsoft.com/keys/microsoft.asc &&
       -z ${MIRROR_URLS:-} && -z ${MIRROR_SECURITY_URLS:-} ]] ||
       fail "expected stock VS snapshot and package sources"
    # prepare_debian_image_buildinfo.sh copies this entire generated directory.
    # A site-local file or a credential-bearing buildinfo URL must never enter it.
    buildinfo=src/sonic-build-hooks/buildinfo
    [[ -f $buildinfo/config/buildinfo.config && -f $buildinfo/sonic-build-hooks_1.0_all.deb ]] ||
        fail "missing generated build hooks inputs"
    extra=$(find "$buildinfo" -mindepth 1 ! -path "$buildinfo/config" \
        ! -path "$buildinfo/config/buildinfo.config" ! -path "$buildinfo/sonic-build-hooks_1.0_all.deb" \
        -print -quit)
    [[ -z $extra && ! -L $buildinfo && ! -L $buildinfo/config &&
       ! -L $buildinfo/config/buildinfo.config && ! -L $buildinfo/sonic-build-hooks_1.0_all.deb ]] ||
        fail "unexpected site-local build hooks input"
    while IFS= read -r setting; do
        case "$setting" in
            "export BUILD_PACKAGES_URL=$BUILD_PACKAGES_URL"|"export BUILD_PUBLIC_URL=$BUILD_PUBLIC_URL"|\
            "export BUILD_SNAPSHOT_URL=$BUILD_SNAPSHOT_URL"|"export MIRROR_SNAPSHOT=y"|\
            "export SONIC_VERSION_CONTROL_COMPONENTS=py2,py3,web,git,docker"|\
            "export SONIC_VERSION_CACHE="|"export DISTRO=trixie"|"export PIP_HTTP_TIMEOUT=60"|\
            "export SONIC_VERSION_CACHE_SOURCE=/nfs/dpkg_cache/vs/vcache"|\
            "export SONIC_VERSION_CACHE_SOURCE=/vcache"|"export SONIC_VERSION_CACHE_SOURCE=/dpkg_cache/vcache") ;;
            *) fail "unexpected generated build hooks configuration" ;;
        esac
    done < "$buildinfo/config/buildinfo.config"
    exit 0
    ;;
seal|restore) ;;
*) fail "expected check-inputs, seal or restore" ;;
esac

root=$(realpath -e "${2:?expected a root filesystem}")
abi=${3:?expected the selected kernel ABI}
[[ $root == "$(pwd -P)/fsroot-vs" && $abi =~ ^[0-9A-Za-z.+-]+$ && $abi == *-sonic-amd64 ]] ||
    fail "invalid rootfs or kernel ABI"
marker=$root/etc/sonic/.vs-host-base-v1
modules=$root/usr/lib/modules/$abi
[[ -s $root/boot/vmlinuz-$abi && -s $root/boot/initrd.img-$abi && -s $modules/modules.dep ]] ||
    fail "incomplete kernel or module tree for $abi"
for module in overlay squashfs; do
    chroot "$root" modinfo -k "$abi" "$module" >/dev/null ||
        fail "missing $module for $abi"
done

if [[ $1 == restore ]]; then
    [[ -f $marker && ! -L $marker ]] || fail "missing stock host base marker"
    # debootstrap uses the builder's resolver for subsequent package installs.
    install -m 0644 /etc/resolv.conf "$root/etc/resolv.conf"
    if grep -qx machine-id "$marker"; then
        chroot "$root" systemd-id128 new > "$root/etc/machine-id"
    fi
    if grep -qx dbus-machine-id "$marker"; then
        install -m 0644 "$root/etc/machine-id" "$root/var/lib/dbus/machine-id"
    fi
    if grep -qx snakeoil "$marker"; then
        chroot "$root" make-ssl-cert generate-default-snakeoil --force-overwrite
    fi
    rm "$marker"
    exit 0
fi

# Do not walk proc, dev or any other live mount while capturing this tree.
if findmnt --list --noheadings --output TARGET | awk -v root="$root" '$0 == root || index($0, root "/") == 1 {found=1} END {exit !found}'; then
    fail "a mount remains under the rootfs"
fi
[[ ! -e $marker && ! -L $marker ]] || fail "unexpected existing host base marker"
install -o root -g root -m 0600 /dev/null "$marker"
echo vs-host-base-v1 > "$marker"
for shadow in "$root/etc/shadow" "$root/etc/shadow-"; do
    [[ -f $shadow ]] || continue
    awk -F: '$2 !~ /^[!*]+$/ {bad=1} END {exit bad}' "$shadow" ||
        fail "a password is set or empty in the account database"
done
awk -F: '$1 == "admin" && $2 ~ /^!+$/ {locked=1} END {exit !locked}' "$root/etc/shadow" ||
    fail "default account is missing or unlocked"

if [[ -e $root/etc/ssl/private/ssl-cert-snakeoil.key ]]; then
    [[ -x $root/usr/sbin/make-ssl-cert ]] || fail "cannot regenerate snakeoil key"
    echo snakeoil >> "$marker"
    rm -f "$root/etc/ssl/private/ssl-cert-snakeoil.key" "$root/etc/ssl/certs/ssl-cert-snakeoil.pem"
fi
for file in etc/sonic/{bmc_config,default_users}.json etc/apt/{auth.conf,apt.conf.d/01proxy} root/{.netrc,.git-credentials} etc/docker/key.json; do
    [[ ! -s $root/$file ]] || fail "unexpected private configuration at $file"
done
shopt -s nullglob
private_files=( "$root"/etc/apt/auth.conf.d/* "$root"/etc/ssh/ssh_host_*_key* "$root"/etc/ssl/private/*
                "$root"/root/.ssh/* "$root"/home/admin/.ssh/* "$root"/root/.gnupg/private-keys-v1.d/* )
(( ${#private_files[@]} == 0 )) || fail "private keys or credentials remain in the stock host base"
# Strip builder identity and seeds from the cached image. Restore only the
# identifiers originally generated by the package install on each worker.
if [[ -s $root/etc/machine-id ]]; then echo machine-id >> "$marker"; fi
rm -f "$root/etc/machine-id"
install -m 0444 /dev/null "$root/etc/machine-id"
if [[ -f $root/var/lib/dbus/machine-id && ! -L $root/var/lib/dbus/machine-id ]]; then
    echo dbus-machine-id >> "$marker"
    rm "$root/var/lib/dbus/machine-id"
fi
rm -f "$root"/{var/lib/systemd/random-seed,var/lib/systemd/credential.secret,var/lib/urandom/random-seed} \
      "$root"/etc/resolv.conf "$root"/etc/resolvconf/resolv.conf.d/original "$root"/run/resolvconf/resolv.conf
