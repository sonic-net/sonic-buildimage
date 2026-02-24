#!/bin/bash

# Generate the sources.list.<arch> in the config path
CONFIG_PATH=$1
export ARCHITECTURE=$2
export DISTRIBUTION=$3

# Handling default
[[ -z $APT_RETRIES_COUNT ]] && APT_RETRIES_COUNT=20
export APT_RETRIES_COUNT

DEFAULT_MIRROR_URL_PREFIX=http://packages.trafficmanager.net
MIRROR_VERSION_FILE=
[[ "$MIRROR_SNAPSHOT" == "y" ]] && MIRROR_VERSION_FILE=files/build/versions/default/versions-mirror
[ -f target/versions/default/versions-mirror ] && MIRROR_VERSION_FILE=target/versions/default/versions-mirror

# The default mirror urls
DEFAULT_MIRROR_URLS=http://debian-archive.trafficmanager.net/debian/
DEFAULT_MIRROR_SECURITY_URLS=http://debian-archive.trafficmanager.net/debian-security/


# The debian-archive.trafficmanager.net does not support armhf, use debian.org instead
if [ "$ARCHITECTURE" == "armhf" ]; then
    DEFAULT_MIRROR_URLS=http://deb.debian.org/debian/
    DEFAULT_MIRROR_SECURITY_URLS=http://deb.debian.org/debian-security/
fi

if [ "$DISTRIBUTION" == "buster" ]; then
    DEFAULT_MIRROR_URLS=http://archive.debian.org/debian/
    DEFAULT_MIRROR_SECURITY_URLS=http://archive.debian.org/debian-security/
fi

if [ "$DISTRIBUTION" == "bullseye" ]; then
    DEFAULT_MIRROR_URLS=http://archive.debian.org/debian/
fi

if [ "$MIRROR_SNAPSHOT" == y ]; then
    if [ -f "$MIRROR_VERSION_FILE" ]; then
        DEBIAN_TIMESTAMP=$(grep "^debian==" $MIRROR_VERSION_FILE | tail -n 1 | sed 's/.*==//')
        DEBIAN_SECURITY_TIMESTAMP=$(grep "^debian-security==" $MIRROR_VERSION_FILE | tail -n 1 | sed 's/.*==//')
    elif [ -z "$DEBIAN_TIMESTAMP" ] || [ -z "$DEBIAN_SECURITY_TIMESTAMP" ]; then
        DEBIAN_TIMESTAMP=$(curl $DEFAULT_MIRROR_URL_PREFIX/debian-snapshot/debian/latest)
        DEBIAN_SECURITY_TIMESTAMP=$(curl $DEFAULT_MIRROR_URL_PREFIX/debian-snapshot/debian-security/latest)
    fi

    DEFAULT_MIRROR_URLS=http://deb.debian.org/debian/,$BUILD_SNAPSHOT_URL/debian/$DEBIAN_TIMESTAMP/
    DEFAULT_MIRROR_SECURITY_URLS=http://deb.debian.org/debian-security/,$BUILD_SNAPSHOT_URL/debian-security/$DEBIAN_SECURITY_TIMESTAMP/

	if [ "$DISTRIBUTION" == "buster" ] || [ "$DISTRIBUTION" == "bullseye" ]; then
		DEFAULT_MIRROR_URLS=http://archive.debian.org/debian/,$BUILD_SNAPSHOT_URL/debian/$DEBIAN_TIMESTAMP/
	fi

    mkdir -p target/versions/default
    if [ ! -f target/versions/default/versions-mirror ]; then
        echo "debian==$DEBIAN_TIMESTAMP" > target/versions/default/versions-mirror
        echo "debian-security==$DEBIAN_SECURITY_TIMESTAMP" >> target/versions/default/versions-mirror
    fi
fi

# Handle sources list
[ -z "$MIRROR_URLS" ] && MIRROR_URLS=$DEFAULT_MIRROR_URLS
[ -z "$MIRROR_SECURITY_URLS" ] && MIRROR_SECURITY_URLS=$DEFAULT_MIRROR_SECURITY_URLS

TEMPLATE=files/apt/sources.list.j2
[ -f files/apt/sources.list.$ARCHITECTURE.j2 ] && TEMPLATE=files/apt/sources.list.$ARCHITECTURE.j2
[ -f $CONFIG_PATH/sources.list.j2 ] && TEMPLATE=$CONFIG_PATH/sources.list.j2
[ -f $CONFIG_PATH/sources.list.$ARCHITECTURE.j2 ] && TEMPLATE=$CONFIG_PATH/sources.list.$ARCHITECTURE.j2

MIRROR_URLS=$MIRROR_URLS MIRROR_SECURITY_URLS=$MIRROR_SECURITY_URLS j2 $TEMPLATE | sed '/^$/N;/^\n$/D' > $CONFIG_PATH/sources.list.$ARCHITECTURE
if [ "$MIRROR_SNAPSHOT" == y ]; then
    # Escape special characters in BUILD_SNAPSHOT_URL for use in sed regex
    ESCAPED_MIRROR_URL=$(echo "$BUILD_SNAPSHOT_URL" | sed 's/[\/&.]/\\&/g')
    # Set the snapshot mirror, and add the SET_REPR_MIRRORS flag
    sed -i -e "/^#*deb.*$ESCAPED_MIRROR_URL/! s/^#*deb/#&/" -e "\$a#SET_REPR_MIRRORS" $CONFIG_PATH/sources.list.$ARCHITECTURE
fi

# Handle apt retry count config
APT_RETRIES_COUNT_FILENAME=apt-retries-count
TEMPLATE=files/apt/$APT_RETRIES_COUNT_FILENAME.j2
j2 $TEMPLATE > $CONFIG_PATH/$APT_RETRIES_COUNT_FILENAME
