#!/bin/bash

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x


ARCH=$1
DISTRO=$2
FILESYSTEM_ROOT=$3
HOST_IMAGE_NAME=host-image
IMAGENAME=${HOST_IMAGE_NAME}

. /usr/local/share/buildinfo/scripts/buildinfo_base.sh
VERSION_DEB_PREFERENCE="01-versions-deb"
BUILDINFO_PATH=${FILESYSTEM_ROOT}/usr/local/share/buildinfo
BUILDINFO_VERSION_PATH=${FILESYSTEM_ROOT}/usr/local/share/buildinfo/versions
BUILDINFO_VERSION_DEB=${BUILDINFO_VERSION_PATH}/${VERSION_DEB_PREFERENCE}
OVERRIDE_VERSION_PATH=files/build/versions/host-image
DIFF_VERSIONS_PATH=$BUILDINFO_PATH/diff-versions

mkdir -p $BUILDINFO_PATH

# Copy the build info config
cp -rf src/sonic-build-hooks/buildinfo/* $BUILDINFO_PATH/

# Generate version lock files
scripts/versions_manager.py generate -t "$BUILDINFO_VERSION_PATH" -m "$OVERRIDE_VERSION_PATH" -d "$DISTRO" -a "$ARCH"

if [ "$ENABLE_VERSION_CONTROL_DEB" == "y" ]; then
    cp -f $BUILDINFO_VERSION_DEB ${FILESYSTEM_ROOT}/etc/apt/preferences.d/
fi

sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb"
#sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "pre_run_buildinfo"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo export DISTRO=${DISTRO} >> /usr/local/share/buildinfo/config/buildinfo.config"
sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "echo export IMAGENAME=${IMAGENAME} >> /usr/local/share/buildinfo/config/buildinfo.config"

sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "pre_run_buildinfo ${HOST_IMAGE_NAME}"
