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
