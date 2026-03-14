#!/bin/bash

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x

SLAVE_DIR=$1
ARCH=$2
DISTRO=$3

# Install the latest debian package sonic-build-hooks in the slave container
sudo dpkg -i --force-overwrite $SLAVE_DIR/buildinfo/sonic-build-hooks_*.deb &> /dev/null

# Enable the build hooks
sudo symlink_build_hooks

# set the global permissions
sudo chmod -f 777 /usr/local/share/buildinfo/ -R

# Build the slave running config
cp -rf $SLAVE_DIR/buildinfo/* /usr/local/share/buildinfo/
. /usr/local/share/buildinfo/scripts/buildinfo_base.sh

# Enable reproducible mirrors
set_reproducible_mirrors
apt-get update > /dev/null 2>&1

# Build the slave version config
[ -d /usr/local/share/buildinfo/versions ] && rm -rf /usr/local/share/buildinfo/versions
scripts/versions_manager.py generate -t "/usr/local/share/buildinfo/versions" -n "build-${SLAVE_DIR}" -d "$DISTRO" -a "$ARCH"
touch ${BUILDINFO_PATH}/versions/versions-deb ${BUILDINFO_PATH}/versions/versions-web

sudo rm -f /etc/apt/preferences.d/01-versions-deb
([ "$ENABLE_VERSION_CONTROL_DEB" == "y" ] && [ -f $VERSION_DEB_PREFERENCE ]) && sudo cp -f $VERSION_DEB_PREFERENCE /etc/apt/preferences.d/
exit 0
