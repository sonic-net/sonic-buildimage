#!/bin/sh

#  Copyright (C) 2013 Curt Brune <curt@cumulusnetworks.com>
#
#  SPDX-License-Identifier:     GPL-2.0

##
## Shell archive template
##
## Strings of the form %%VAR%% are replaced during construction.
##

echo -n "Verifying image checksum ..."
payload_image_size=%%PAYLOAD_IMAGE_SIZE%%

sha1=$(sed -e '1,/^exit_marker$/d' "$0" | head -c $payload_image_size | sha1sum | awk '{ print $1 }')

payload_sha1=%%IMAGE_SHA1%%

if [ "$sha1" != "$payload_sha1" ] ; then
    echo
    echo "ERROR: Unable to verify archive checksum"
    echo "Expected: $payload_sha1"
    echo "Found   : $sha1"
    exit 1
fi

echo " OK."

image_size_in_kb=$((($(sed -e '1,/^exit_marker$/d' "$0"  | tar --to-stdout -xf - | wc -c) + 1023 ) / 1024))
# Untar and launch install script in a tmpfs
cur_wd=$(pwd)
export cur_wd
archive_path=$(realpath "$0")
tmp_dir=$(mktemp -d)
if [ "$(id -u)" = "0" ] ; then
    mount -t tmpfs tmpfs-installer $tmp_dir || exit 1
    mount_size_in_kb=$(df $tmp_dir | tail -1 | tr -s ' ' | cut -d' ' -f4)
    #checking extra 100KB space in tmp_dir, after image extraction
    padding=102400
    if [ "$mount_size_in_kb" -le "$((image_size_in_kb + padding))" ]; then
        image_size_in_mb=$(((image_size_in_kb + 1023) / 1024))
        #Adding extra 32MB free space for image extraction.
        mount_size_in_mb=$((((image_size_in_mb + 31) / 32) * 32))
        mount -o remount,size="${mount_size_in_mb}M" -t tmpfs tmpfs-installer $tmp_dir || exit 1
    fi
fi
cd $tmp_dir
echo -n "Preparing image archive ..."

sed -e '1,/^exit_marker$/d' $archive_path | head -c $payload_image_size | tar xf - || exit 1

echo " OK."
cd $cur_wd
if [ -n "$extract" ] ; then
    # stop here
    echo "Image extracted to: $tmp_dir"
    if [ "$(id -u)" = "0" ] && [ ! -d "$extract" ] ; then
        echo "To un-mount the tmpfs when finished type:  umount $tmp_dir"
    fi
    exit 0
fi

$tmp_dir/installer/install.sh
rc="$?"

# clean up
if [ "$(id -u)" = "0" ] ; then
    umount $tmp_dir
fi
rm -rf $tmp_dir

exit $rc
exit_marker
