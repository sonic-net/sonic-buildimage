#!/bin/bash

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x

SCRIPT_SRC_PATH=src/sonic-build-hooks
if [ -e ${SCRIPT_SRC_PATH} ]; then
	. ${SCRIPT_SRC_PATH}/scripts/utils.sh
fi

ARCH=$1
DISTRO=$2
TARGET=$3
FILESYSTEM_ROOT=$4
VERSIONS_PATH=$TARGET/versions/host-image
IMAGENAME="host-image"

[ -d $VERSIONS_PATH ] && sudo rm -rf $VERSIONS_PATH
mkdir -p $VERSIONS_PATH

mkdir -p target/vcache/${IMAGENAME}
sudo LANG=C chroot $FILESYSTEM_ROOT post_run_buildinfo ${IMAGENAME}

cp -r $FILESYSTEM_ROOT/usr/local/share/buildinfo/pre-versions $VERSIONS_PATH/
cp -r $FILESYSTEM_ROOT/usr/local/share/buildinfo/post-versions $VERSIONS_PATH/

sudo LANG=C chroot $FILESYSTEM_ROOT post_run_cleanup ${IMAGENAME}

# Re-capture host-base-image package versions from the finished rootfs.
# host-base-image was originally captured right after the initial debootstrap,
# which only pulls from the plain Debian archive (no "-security" suite). By
# now the rootfs has been apt-upgraded against the full mirrors (including
# "-security"). Refresh the tracked version file to match what's actually
# installed, unless deb versions are pinned for a reproducible build
# (SONIC_VERSION_CONTROL_COMPONENTS includes "deb"/"all"), where the file is
# authoritative input and must stay untouched.
#
# host-base-image only tracks the minbase package set, not the full
# host-image package set that ends up installed in this same rootfs, so we
# must not blindly dump the whole "dpkg-query -W" output here -- that would
# pollute host-base-image with host-image-only packages. Instead, only
# refresh the versions of packages that were already present in the
# base-image's own version file (written earlier in this build by
# build_debian_base_system.sh), keeping its package set unchanged.
BASEIMAGE_VERSIONS_FILE=$TARGET/versions/host-base-image/versions-deb-${DISTRO}-${ARCH}
if [[ ",$SONIC_VERSION_CONTROL_COMPONENTS," != *,all,* ]] && [[ ",$SONIC_VERSION_CONTROL_COMPONENTS," != *,deb,* ]] && [ -f "$BASEIMAGE_VERSIONS_FILE" ]; then
    TMP_INSTALLED_VERSIONS=$(mktemp)
    trap 'rm -f "$TMP_INSTALLED_VERSIONS"' EXIT
    if ! sudo LANG=C chroot "$FILESYSTEM_ROOT" /bin/bash -c "dpkg-query -W -f '\${Package}==\${Version}\n'" > "$TMP_INSTALLED_VERSIONS"; then
         echo "Failed to capture installed host-base-image package versions" >&2
         exit 1
     fi
     awk -F'==' '
         FILENAME == ARGV[1] { installed[$1]=$0; next }
         { print ($1 in installed) ? installed[$1] : $0 }
     ' "$TMP_INSTALLED_VERSIONS" "$BASEIMAGE_VERSIONS_FILE" > "${BASEIMAGE_VERSIONS_FILE}.new" && \
         mv "${BASEIMAGE_VERSIONS_FILE}.new" "$BASEIMAGE_VERSIONS_FILE" || \
         rm -f "${BASEIMAGE_VERSIONS_FILE}.new"
    rm -f $TMP_INSTALLED_VERSIONS
    trap - EXIT
fi
