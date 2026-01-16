#!/bin/bash

# Generate the sources.list.<arch> in the config path
CONFIG_PATH=$1
export ARCHITECTURE=$2
export DISTRIBUTION=$3

DEFAULT_MIRROR_URL_PREFIX=http://packages.trafficmanager.net
MIRROR_VERSION_FILE=
[[ "$SONIC_VERSION_CONTROL_COMPONENTS" == *deb* || $SONIC_VERSION_CONTROL_COMPONENTS == *all* ]] && MIRROR_VERSION_FILE=files/build/versions/default/versions-mirror && MIRROR_SNAPSHOT=y
[ -f target/versions/default/versions-mirror ] && MIRROR_VERSION_FILE=target/versions/default/versions-mirror

# The default mirror urls
# - bullseye: use deb.debian.org (security仍在线)
# - 其他/EOL发行版：使用 archive
if [ "$DISTRIBUTION" = "bullseye" ]; then
    DEFAULT_MIRROR_URLS=http://deb.debian.org/debian/
    DEFAULT_MIRROR_SECURITY_URLS=http://deb.debian.org/debian-security/
    DEFAULT_MIRROR_BACKPORTS_URLS=http://deb.debian.org/debian/
else
    DEFAULT_MIRROR_URLS=http://archive.debian.org/debian/
    DEFAULT_MIRROR_SECURITY_URLS=http://archive.debian.org/debian-security/
    DEFAULT_MIRROR_BACKPORTS_URLS=http://archive.debian.org/debian/
fi


# Use archive for all arches to keep HTTP only

if [ "$MIRROR_SNAPSHOT" == y ]; then
    if [ -f "$MIRROR_VERSION_FILE" ]; then
        DEBIAN_TIMESTAMP=$(grep "^debian==" $MIRROR_VERSION_FILE | tail -n 1 | sed 's/.*==//')
        DEBIAN_SECURITY_TIMESTAMP=$(grep "^debian-security==" $MIRROR_VERSION_FILE | tail -n 1 | sed 's/.*==//')
    elif [ -z "$DEBIAN_TIMESTAMP" ] || [ -z "$DEBIAN_SECURITY_TIMESTAMP" ]; then
        DEBIAN_TIMESTAMP=$(curl $DEFAULT_MIRROR_URL_PREFIX/snapshot/debian/latest/timestamp)
        DEBIAN_SECURITY_TIMESTAMP=$(curl $DEFAULT_MIRROR_URL_PREFIX/snapshot/debian-security/latest/timestamp)
    fi

    if [ "$DISTRIBUTION" = "bullseye" ]; then
        DEFAULT_MIRROR_URLS=http://deb.debian.org/debian/,http://packages.trafficmanager.net/snapshot/debian/$DEBIAN_TIMESTAMP/
        DEFAULT_MIRROR_SECURITY_URLS=http://deb.debian.org/debian-security/,http://packages.trafficmanager.net/snapshot/debian-security/$DEBIAN_SECURITY_TIMESTAMP/
    else
        DEFAULT_MIRROR_URLS=http://archive.debian.org/debian/,http://packages.trafficmanager.net/snapshot/debian/$DEBIAN_TIMESTAMP/
        DEFAULT_MIRROR_SECURITY_URLS=http://archive.debian.org/debian-security/,http://packages.trafficmanager.net/snapshot/debian-security/$DEBIAN_SECURITY_TIMESTAMP/
    fi

    mkdir -p target/versions/default
    if [ ! -f target/versions/default/versions-mirror ]; then
        echo "debian==$DEBIAN_TIMESTAMP" > target/versions/default/versions-mirror
        echo "debian-security==$DEBIAN_SECURITY_TIMESTAMP" >> target/versions/default/versions-mirror
    fi
fi

[ -z "$MIRROR_URLS" ] && MIRROR_URLS=$DEFAULT_MIRROR_URLS
[ -z "$MIRROR_SECURITY_URLS" ] && MIRROR_SECURITY_URLS=$DEFAULT_MIRROR_SECURITY_URLS
[ -z "$MIRROR_BACKPORTS_URLS" ] && MIRROR_BACKPORTS_URLS=$DEFAULT_MIRROR_BACKPORTS_URLS

TEMPLATE=files/apt/sources.list.j2
[ -f files/apt/sources.list.$ARCHITECTURE.j2 ] && TEMPLATE=files/apt/sources.list.$ARCHITECTURE.j2
[ -f $CONFIG_PATH/sources.list.j2 ] && TEMPLATE=$CONFIG_PATH/sources.list.j2
[ -f $CONFIG_PATH/sources.list.$ARCHITECTURE.j2 ] && TEMPLATE=$CONFIG_PATH/sources.list.$ARCHITECTURE.j2

MIRROR_URLS=$MIRROR_URLS MIRROR_SECURITY_URLS=$MIRROR_SECURITY_URLS MIRROR_BACKPORTS_URLS=$MIRROR_BACKPORTS_URLS j2 $TEMPLATE | sed '/^$/N;/^\n$/D' > $CONFIG_PATH/sources.list.$ARCHITECTURE
if [ "$MIRROR_SNAPSHOT" == y ]; then
    # Set the snapshot mirror, and add the SET_REPR_MIRRORS flag
    sed -i -e "/^#*deb.*packages.trafficmanager.net/! s/^#*deb/#&/" -e "\$a#SET_REPR_MIRRORS" $CONFIG_PATH/sources.list.$ARCHITECTURE
fi
