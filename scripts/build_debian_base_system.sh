#!/bin/bash

[[ ! -z "${DBGOPT}" && $0 =~ ${DBGOPT} ]] && set -x

CONFIGURED_ARCH=$1
IMAGE_DISTRO=$2
FILESYSTEM_ROOT=$3
http_proxy=$4

TARGET=$TARGET_PATH
[ -z "$TARGET" ] && TARGET=target

. /usr/local/share/buildinfo/scripts/buildinfo_base.sh

TARGET_BASEIMAGE_PATH=$TARGET/versions/host-base-image
mkdir -p $TARGET_BASEIMAGE_PATH

alias urlencode='python3 -c "import sys, urllib.parse as ul; print (ul.quote_plus(sys.argv[1]))"'
shopt -s expand_aliases

generate_version_file()
{
    sudo LANG=C chroot $FILESYSTEM_ROOT /bin/bash -c "dpkg-query -W -f '\${Package}==\${Version}\n'" > $TARGET_BASEIMAGE_PATH/versions-deb-${IMAGE_DISTRO}-${CONFIGURED_ARCH}
}

if [ "$MIRROR_SNAPSHOT" == y ]; then
    SNAPSHOT_TIMESTAMP=$(grep "^debian==" $TARGET/versions/default/versions-mirror | tail -n 1 | sed 's/.*==//')
    MIRROR_URL=$BUILD_SNAPSHOT_URL/debian/$SNAPSHOT_TIMESTAMP
else
    MIRROR_URL=http://deb.debian.org/debian
fi

if [ "$ENABLE_VERSION_CONTROL_DEB" != "y" ] || [ ! -d files/build/versions/host-base-image ]; then
    if [[ $CONFIGURED_ARCH == armhf || $CONFIGURED_ARCH == arm64 ]]; then
        if [[ $MULTIARCH_QEMU_ENVIRON == y || $CROSS_BUILD_ENVIRON == y ]]; then
            # qemu arm bin executable for cross-building
            sudo mkdir -p $FILESYSTEM_ROOT/usr/bin
            sudo cp /usr/bin/qemu*static $FILESYSTEM_ROOT/usr/bin || true
        fi
        sudo http_proxy=$HTTP_PROXY SKIP_BUILD_HOOK=y debootstrap --foreign --variant=minbase --arch $CONFIGURED_ARCH $IMAGE_DISTRO $FILESYSTEM_ROOT "$MIRROR_URL"
        sudo rm $FILESYSTEM_ROOT/proc -rf
        sudo mkdir $FILESYSTEM_ROOT/proc
        sudo mount -t proc proc $FILESYSTEM_ROOT/proc
        sudo LANG=C chroot $FILESYSTEM_ROOT /debootstrap/debootstrap --second-stage
    else
        sudo http_proxy=$HTTP_PROXY SKIP_BUILD_HOOK=y debootstrap --variant=minbase --arch $CONFIGURED_ARCH $IMAGE_DISTRO $FILESYSTEM_ROOT "$MIRROR_URL"
    fi
    RET=$?
    if [ $RET -ne 0 ]; then
        exit $RET
    fi

    generate_version_file
    exit $RET
fi

ARCH=$(dpkg --print-architecture)
DISTRO=$(grep CODENAME /etc/os-release | cut -d= -f2)
if [ "$ARCH" != "$CONFIGURED_ARCH" ] || [ "$DISTRO" != "$IMAGE_DISTRO" ]; then
    "Not support to build different ARCH/DISTRO ${CONFIGURED_ARCH}:${IMAGE_DISTRO} in ${ARCH}:${DISTRO}."
    exit 1
fi

# Generate the version files for the host base image
TEMP_DIR=$(mktemp -d)
./scripts/versions_manager.py generate -t $TEMP_DIR -n host-base-image -d $IMAGE_DISTRO -a $CONFIGURED_ARCH
PACKAGES=$(sed -E 's/=(=[^=]*)$/\1/' $TEMP_DIR/versions-deb)
if [ -z "$PACKAGES" ]; then
    echo "Not found host-base-image packages, please check the version files in files/build/versions/host-base-image" 2>&1
    exit 1
fi
rm -rf $TEMP_DIR

BASEIMAGE_TARBALLPATH=$TARGET/baseimage
BASEIMAGE_TARBALL=$(realpath -e $TARGET)/baseimage.tgz

rm -rf $BASEIMAGE_TARBALLPATH $BASEIMAGE_TARBALL

ARCHIEVES=$BASEIMAGE_TARBALLPATH/var/cache/apt/archives
APTLIST=$BASEIMAGE_TARBALLPATH/var/lib/apt/lists
TARGET_DEBOOTSTRAP=$BASEIMAGE_TARBALLPATH/debootstrap
APTDEBIAN="$APTLIST/deb.debian.org_debian_dists_${DISTRO}_main_binary-${CONFIGURED_ARCH}_Packages"
DEBPATHS=$TARGET_DEBOOTSTRAP/debpaths
DEBOOTSTRAP_BASE=$TARGET_DEBOOTSTRAP/base
DEBOOTSTRAP_REQUIRED=$TARGET_DEBOOTSTRAP/required
[ -d $BASEIMAGE_TARBALLPATH ] && rm -rf $BASEIMAGE_TARBALLPATH
mkdir -p $ARCHIEVES
mkdir -p $APTLIST
mkdir -p $TARGET_DEBOOTSTRAP
URL_ARR=$(apt-get download --print-uris $PACKAGES | cut -d" " -f1 | tr -d "'")
PACKAGE_ARR=($PACKAGES)
LENGTH=${#PACKAGE_ARR[@]}
for ((i=0;i<LENGTH;i++))
do
    package=${PACKAGE_ARR[$i]}
    packagename=$(echo $package | sed -E 's/=[^=]*$//')
    encoded_packagename=$(urlencode $packagename)
    url=$(echo "$URL_ARR" | grep -i "/${packagename}_\|/${encoded_packagename}_")
    if [ -z "$url" ] || [[ $(echo "$url" | wc -l) -gt 1 ]]; then
        echo "No found package or found multiple package for package $packagename, url: $url" 1>&2
        exit 1
    fi
    filename=$(basename "$url")
    SKIP_BUILD_HOOK=y wget $url -O $ARCHIEVES/$filename
    echo $packagename >> $DEBOOTSTRAP_REQUIRED
    echo "$packagename /var/cache/apt/archives/$filename" >> $DEBPATHS
done
touch $APTDEBIAN
touch $DEBOOTSTRAP_BASE
(cd $BASEIMAGE_TARBALLPATH && fakeroot tar -zcf $BASEIMAGE_TARBALL .)

sudo debootstrap --verbose --variant=minbase --arch $CONFIGURED_ARCH --unpack-tarball=$BASEIMAGE_TARBALL $IMAGE_DISTRO $FILESYSTEM_ROOT
RET=$?
if [ $RET -ne 0 ]; then
    exit $RET
fi

generate_version_file
