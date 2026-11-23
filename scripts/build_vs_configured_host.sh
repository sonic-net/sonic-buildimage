#!/bin/bash
# Prepare a stock VS host from a sealed host-base SquashFS. The caller keys
# this native package installation on the base, packages, tools and settings.
set -euo pipefail

fail() { echo "VS configured host: $*" >&2; exit 1; }

if [[ ${1:-} == --restore-nvme ]]; then
    # Reproduce nvme-cli.postinst after the per-build machine ID is restored.
    chroot "${2:?expected configured rootfs}" sh -ec '
        for file in /etc/nvme/hostnqn /etc/nvme/hostid; do
            [ -f "$file" ] || continue
            [ ! -L "$file" ] && [ ! -s "$file" ] || exit 1
            if [ "$file" = /etc/nvme/hostnqn ]; then
                nvme gen-hostnqn > "$file"
            else
                uuidgen > "$file"
            fi
        done'
    exit 0
fi

if [[ ${1:-} == --configure ]]; then
    work=${2:?expected the private work directory}
    root=$work/fsroot-vs
    [[ $EUID == 0 && $$ == 1 ]] || fail 'expected a private mount and PID namespace'
    mount -t proc proc "$root/proc"
    install -m 0644 /etc/resolv.conf "$root/etc/resolv.conf"

    # The immutable Docker packages were installed in the base. Restrict this
    # second transaction to the signed Debian snapshots selected by that base.
    mirrors=target/versions/default/versions-mirror
    debian=$(sed -n 's/^debian==//p' "$mirrors")
    security=$(sed -n 's/^debian-security==//p' "$mirrors")
    [[ $debian =~ ^[0-9]{8}T[0-9]{6}Z$ && $security =~ ^[0-9]{8}T[0-9]{6}Z$ ]] ||
        fail 'missing dated Debian or security snapshot'
    awk -v deb="http://packages.trafficmanager.net/snapshot/debian/$debian/" \
        -v sec="http://packages.trafficmanager.net/snapshot/debian-security/$security/" '
        /^[[:space:]]*($|#)/ { next }
        ($1 == "deb" || $1 == "deb-src") && $2 == "[arch=amd64]" {
            if ($3 == deb) { seen_debian=1; next }
            if ($3 == sec) { seen_security=1; next }
        }
        { unknown=1 }
        END { exit (unknown || !seen_debian || !seen_security) }
    ' "$root/etc/apt/sources.list" || fail 'unexpected APT source in the host base'
    # The option applies to every apt-get invocation including dpkg -i recovery.
    mkdir "$root/etc/apt/vs-configured-host-empty"
    printf '%s\n' 'Dir::Etc::sourceparts "/etc/apt/vs-configured-host-empty";' \
        'APT::Update::Error-Mode "any";' > "$root/etc/apt/apt.conf.d/99vs-configured-host"
    python3 tools/bazel/ci/vs_host_pypi.py "$root"
    ./sonic_debian_extension.sh "$root" platform trixie configure-host 2>&1 | tee "$work/configure.log"
    if grep -Fq 'try to install with the original command' "$work/configure.log"; then
        fail "pip retried without its constraints"
    fi
    python3 tools/bazel/ci/vs_host_pypi.py --remove "$root"
    rm -- "$root/etc/apt/apt.conf.d/99vs-configured-host"
    rmdir "$root/etc/apt/vs-configured-host-empty"
    exit 0
fi

base=$(realpath -e "${1:?expected the host-base SquashFS}")
output=$(realpath -m "${2:?expected the configured SquashFS output}")
repo=$(pwd -P)
[[ -x ./sonic_debian_extension.sh ]] || fail 'missing rendered host extension'
work=$(mktemp -d "$repo/target/.vs-configured-host.XXXXXXXX")
trap 'sudo rm -rf --one-file-system -- "$work"' EXIT
root=$work/fsroot-vs
sudo unsquashfs -no-progress -d "$root" "$base"
marker=etc/sonic/.vs-host-base-v1
sudo grep -qx 'vs-host-base-v1' "$root/$marker" || fail 'expected unpersonalized base'
sudo mv -- "$root/$marker" "$work/base-state"

# Package maintainer scripts sometimes start daemons. They must be gone before
# the filesystem is captured, including on failure; the builder's mounts stay
# untouched. The action driver supplies a sanitized, fingerprinted environment.
sudo -E unshare --mount --pid --fork --kill-child --mount-proc --propagation private \
    "$repo/scripts/build_vs_configured_host.sh" --configure "$work"

# Reuse the first-stage sealing and validation checks (including locked shadow
# entries, generated keys, resolver state, mount checks and the kernel ABI).
(cd "$work" && sudo bash "$repo/scripts/vs_host_base_state.sh" seal "$root" "$kversion")
# Some base files such as the machine ID or TLS key were already removed.
# Preserve their first-boot regeneration flags across both sealing passes.
for regeneration in machine-id dbus-machine-id snakeoil; do
    if sudo grep -qx "$regeneration" "$work/base-state" && ! sudo grep -qx "$regeneration" "$root/$marker"; then
        printf '%s\n' "$regeneration" | sudo tee -a "$root/$marker" >/dev/null
    fi
done
for path in etc/sonic/sonic_version.yml usr/share/sonic/docker-image-inventory.json; do
    if sudo test -e "$root/$path" || sudo test -L "$root/$path"; then
        fail "release-specific file present in configured host: $path"
    fi
done
# nvme-cli.postinst generates identities on installation. Leave an empty file
# only where the package created one, and regenerate it for each consumer.
for identity in "$root/etc/nvme/hostnqn" "$root/etc/nvme/hostid"; do
    if sudo test -L "$identity"; then fail "unexpected NVMe identity symlink"; fi
    if sudo test -f "$identity"; then sudo truncate -s 0 "$identity"; fi
done
# New host packages can introduce credential stores beyond the Debian base.
# Stock chrony and LDAP install templates without keys or bind passwords.
for file in etc/chrony/chrony.keys var/lib/ucf/cache/:etc:chrony:chrony.keys; do
    if sudo test -f "$root/$file" && sudo grep -Eq '^[[:space:]]*[^#[:space:]]' "$root/$file"; then
        fail "configured keys present at $file"
    fi
done
for file in var/cache/debconf/passwords.dat var/cache/debconf/passwords.dat-old; do
    if sudo test -f "$root/$file" && sudo grep -Eq '^Value:[[:space:]]*[^[:space:]]' "$root/$file"; then
        fail "debconf contains a password value"
    fi
done
if sudo test -f "$root/etc/nslcd.conf" &&
   sudo grep -Eiq '^[[:space:]]*(bindpw|rootpwmodpw|tls_key)[[:space:]]' "$root/etc/nslcd.conf"; then
    fail "private LDAP configuration present"
fi
for file in etc/iscsi/initiatorname.iscsi etc/krb5.keytab var/lib/snmp/snmpd.conf; do
    if sudo test -s "$root/$file"; then fail "unexpected host identity at $file"; fi
done
for dir in var/lib/tpm var/lib/tpm2-tss/system/keystore var/lib/docker; do
    if sudo test -d "$root/$dir" && [[ -n $(sudo find "$root/$dir" ! -type d -print -quit) ]]; then
        fail "unexpected state in $dir"
    fi
done
printf '%s\n' vs-configured-host-v1 | sudo tee "$root/etc/sonic/.vs-configured-host-v1" >/dev/null
sudo mksquashfs "$root" "$output" -noappend -no-progress -comp zstd -Xcompression-level 3
# Bazel normalizes output permissions after the native action returns.
sudo chown --reference="$work" -- "$output"
