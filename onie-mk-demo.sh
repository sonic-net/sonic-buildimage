#!/bin/sh

#  Copyright (C) 2013-2014 Curt Brune <curt@cumulusnetworks.com>
#
#  SPDX-License-Identifier:     GPL-2.0
set -x

arch=$1
machine=$2
platform=$3
installer_dir=$4
platform_conf=$5
output_file=$6
demo_type=$7
image_version=$8
onie_image_part_size=$9

shift 9

if  [ ! -d $installer_dir ] || \
    [ ! -r $installer_dir/sharch_body.sh ] ; then
    echo "Error: Invalid installer script directory: $installer_dir"
    exit 1
fi

if  [ ! -d $installer_dir ] || \
    [ ! -r $installer_dir/install.sh ] ; then
    echo "Error: Invalid arch installer directory: $installer_dir"
    exit 1
fi

[ -n "$image_version" ] || {
    echo "Error: Invalid git revisions"
    exit 1
}

[ -n "$onie_image_part_size" ] || {
    echo "Error: Invalid onie_image_part_size"
    exit 1
}

[ -r "$platform_conf" ] || {
    echo "Warning: Unable to read installer platform configuration file: $platform_conf"
}

[ $# -gt 0 ] || {
    echo "Error: No OS image files found"
    exit 1
}

case $demo_type in
    OS|DIAG)
        # These are supported
        ;;
    *)
        echo "Error: Unsupported demo type: $demo_type"
        exit 1
esac

tmp_dir=
clean_up()
{
    rm -rf $tmp_dir
    exit $1
}

# make the data archive
# contents:
#   - kernel and initramfs
#   - install.sh
#   - $platform_conf

echo -n "Building self-extracting install image ."
tmp_dir=$(mktemp --directory)
tmp_installdir="$tmp_dir/installer"
mkdir $tmp_installdir || clean_up 1

cp -r $installer_dir/* $tmp_installdir || clean_up 1
cp onie-image.conf $tmp_installdir
cp onie-image-$arch.conf $tmp_installdir

# Set sonic fips config for the installer script
if [ "$ENABLE_FIPS" = "y" ]; then
    EXTRA_CMDLINE_LINUX="$EXTRA_CMDLINE_LINUX sonic_fips=1"
fi

# Escape special chars in the user provide kernel cmdline string for use in
# sed. Special chars are: \ / &
EXTRA_CMDLINE_LINUX=`echo $EXTRA_CMDLINE_LINUX | sed -e 's/[\/&]/\\\&/g'`

output_raw_image=$(cat onie-image.conf | grep OUTPUT_RAW_IMAGE | cut -f2 -d"=")
[ -z "$TARGET_MACHINE" ] && output_raw_image=$(echo $output_raw_image | sed -e 's/$TARGET_MACHINE/$machine/g')
output_raw_image=$(eval echo $output_raw_image)

# Tailor the demo installer for OS mode or DIAG mode
sed -i -e "s/%%DEMO_TYPE%%/$demo_type/g" \
       -e "s/%%IMAGE_VERSION%%/$image_version/g" \
       -e "s/%%ONIE_IMAGE_PART_SIZE%%/$onie_image_part_size/" \
       -e "s/%%EXTRA_CMDLINE_LINUX%%/$EXTRA_CMDLINE_LINUX/" \
       -e "s@%%OUTPUT_RAW_IMAGE%%@$output_raw_image@" \
    $tmp_installdir/install.sh || clean_up 1
echo -n "."
cp -r $* $tmp_installdir || clean_up 1
echo -n "."
[ -r "$platform_conf" ] && {
    cp $platform_conf $tmp_installdir || clean_up 1
}
echo "machine=$machine" > $tmp_installdir/machine.conf
echo "platform=$platform" >> $tmp_installdir/machine.conf
echo -n "."

sharch="$tmp_dir/sharch.tar"
tar -C $tmp_dir -cf $sharch installer || {
    echo "Error: Problems creating $sharch archive"
    clean_up 1
}
echo -n "."

[ -f "$sharch" ] || {
    echo "Error: $sharch not found"
    clean_up 1
}
sha1=$(cat $sharch | sha1sum | awk '{print $1}')
echo -n "."
cp $installer_dir/sharch_body.sh $output_file || {
    echo "Error: Problems copying sharch_body.sh"
    clean_up 1
}

# Replace variables in the sharch template
sed -i -e "s/%%IMAGE_SHA1%%/$sha1/" $output_file
echo -n "."
cat $sharch >> $output_file
rm -rf $tmp_dir
echo " Done."

echo "Success:  Demo install image is ready in ${output_file}:"
ls -l ${output_file}

clean_up 0
