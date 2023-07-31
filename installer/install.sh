#!/bin/sh

#  Copyright (C) 2020      Marvell Inc
#  Copyright (C) 2014-2015 Curt Brune <curt@cumulusnetworks.com>
#  Copyright (C) 2014-2015 david_yang <david_yang@accton.com>
#
#  SPDX-License-Identifier:     GPL-2.0

# Appends a command to a trap, which is needed because default trap behavior is to replace
# previous trap for the same signal
# - 1st arg:  code to add
# - ref: http://stackoverflow.com/questions/3338030/multiple-bash-traps-for-the-same-signal


_trap_push() {
    local next="$1"
    eval "trap_push() {
        local oldcmd='$(echo "$next" | sed -e s/\'/\'\\\\\'\'/g)'
        local newcmd=\"\$1; \$oldcmd\"
        trap -- \"\$newcmd\" EXIT INT TERM HUP
        _trap_push \"\$newcmd\"
    }"
}
_trap_push true

read_conf_file() {
    local conf_file=$1
    while IFS='=' read -r var value || [ -n "$var" ]
    do
        # remove newline character
        var=$(echo $var | tr -d '\r\n')
        value=$(echo $value | tr -d '\r\n')
        # remove comment string
        var=${var%#*}
        value=${value%#*}
        # skip blank line
        [ -z "$var" ] && continue
        # remove double quote in the beginning
        tmp_val=${value#\"}
        # remove double quote in the end
        value=${tmp_val%\"}
        eval "$var=\"$value\""
    done < "$conf_file"
}

set -e

if [ -d "/etc/sonic" ]; then
    echo "Installing SONiC in SONiC"
    install_env="sonic"
elif grep -Fxqs "DISTRIB_ID=onie" /etc/lsb-release > /dev/null
then
    echo "Installing SONiC in ONIE"
    install_env="onie"
else
    echo "Installing SONiC in BUILD"
    install_env="build"
fi

cd $(dirname $0)
if [ -r ./machine.conf ]; then
    read_conf_file "./machine.conf"
fi

# Load generic onie-image.conf
if [ -r ./onie-image.conf ]; then
. ./onie-image.conf
fi

# Load arch-specific onie-image-[arch].conf if exists
if [ -r ./onie-image-*.conf ]; then
. ./onie-image-*.conf
fi

echo "ONIE Installer: platform: $platform"

# Make sure run as root or under 'sudo'
if [ $(id -u) -ne 0 ]
    then echo "Please run as root"
    exit 1
fi

# get running machine from conf file
if [ -r /etc/machine.conf ]; then
    read_conf_file "/etc/machine.conf"
elif [ -r /host/machine.conf ]; then
    read_conf_file "/host/machine.conf"
elif [ "$install_env" != "build" ]; then
    echo "cannot find machine.conf"
    exit 1
fi


echo "onie_platform: $onie_platform"

# Get platform specific linux kernel command line arguments
ONIE_PLATFORM_EXTRA_CMDLINE_LINUX=""

# Default var/log device size in MB
VAR_LOG_SIZE=4096

[ -r platforms/$onie_platform ] && . platforms/$onie_platform

# Verify image platform is inside devices list
if [ "$install_env" = "onie" ]; then
    if ! grep -Fxq "$onie_platform" platforms_asic; then
        echo "The image you're trying to install is of a different ASIC type as the running platform's ASIC"
        while true; do
            read -r -p "Do you still wish to install this image? [y/n]: " input
            case $input in
                [Yy])
                    echo "Force installing..."
                    break
                    ;;
                [Nn])
                    echo "Exited installation!"
                    exit 1
                    ;;
                *)
                    echo "Error: Invalid input"
                    ;;
            esac
        done
    fi
fi

# If running in ONIE
if [ "$install_env" = "onie" ]; then
    # The onie bin tool prefix
    onie_bin=
    # The persistent ONIE directory location
    onie_root_dir=/mnt/onie-boot/onie
    # The onie file system root
    onie_initrd_tmp=/
fi

# The build system prepares this script by replacing %%DEMO-TYPE%%
# with "OS" or "DIAG".
demo_type="%%DEMO_TYPE%%"

# The build system prepares this script by replacing %%ONIE_IMAGE_PART_SIZE%%
# with the partition size
demo_part_size="%%ONIE_IMAGE_PART_SIZE%%"

# The build system prepares this script by replacing %%IMAGE_VERSION%%
# with git revision hash as a version identifier
image_version="%%IMAGE_VERSION%%"
timestamp="$(date -u +%Y%m%d)"

demo_volume_label="SONiC-${demo_type}"
demo_volume_revision_label="SONiC-${demo_type}-${image_version}"


. ./default_platform.conf

if [ -r ./platform.conf ]; then
    . ./platform.conf
fi


image_dir="image-$image_version"

if [ "$install_env" = "onie" ]; then
    # Create/format the flash
    create_partition
    mount_partition
elif [ "$install_env" = "sonic" ]; then
    demo_mnt="/host"
    # Get current SONiC image (grub/aboot/uboot)
    eval running_sonic_revision="$(cat /proc/cmdline | sed -n 's/^.*loop=\/*image-\(\S\+\)\/.*$/\1/p')"
    # Verify SONiC image exists
    if [ ! -d "$demo_mnt/image-$running_sonic_revision" ]; then
        echo "ERROR: SONiC installation is corrupted: path $demo_mnt/image-$running_sonic_revision doesn't exist"
        exit 1
    fi
    # Prevent installing existing SONiC if it is running
    if [ "$image_dir" = "image-$running_sonic_revision" ]; then
        echo "Not installing SONiC version $running_sonic_revision, as current running SONiC has the same version"
        exit 0
    fi
    # Remove extra SONiC images if any
    for f in $demo_mnt/image-* ; do
        if [ -d $f ] && [ "$f" != "$demo_mnt/image-$running_sonic_revision" ] && [ "$f" != "$demo_mnt/$image_dir" ]; then
            echo "Removing old SONiC installation $f"
            rm -rf $f
        fi
    done
else
    demo_mnt="build_raw_image_mnt"
    demo_dev=$cur_wd/"%%OUTPUT_RAW_IMAGE%%"

    mkfs.ext4 -L $demo_volume_label $demo_dev

    echo "Mounting $demo_dev on $demo_mnt..."
    mkdir $demo_mnt
    mount -t auto -o loop $demo_dev $demo_mnt
fi

echo "Installing SONiC to $demo_mnt/$image_dir"

# Create target directory or clean it up if exists
if [ -d $demo_mnt/$image_dir ]; then
    echo "Directory $demo_mnt/$image_dir/ already exists. Cleaning up..."
    rm -rf $demo_mnt/$image_dir/*
else
    mkdir $demo_mnt/$image_dir || {
        echo "Error: Unable to create SONiC directory"
        exit 1
    }
fi

# Decompress the file for the file system directly to the partition
if [ x"$docker_inram" = x"on" ]; then
    # when disk is small, keep dockerfs.tar.gz in disk, expand it into ramfs during initrd
    unzip -o $ONIE_INSTALLER_PAYLOAD -x "platform.tar.gz" -d $demo_mnt/$image_dir
else
    unzip -o $ONIE_INSTALLER_PAYLOAD -x "$FILESYSTEM_DOCKERFS" "platform.tar.gz" -d $demo_mnt/$image_dir

    if [ "$install_env" = "onie" ]; then
        TAR_EXTRA_OPTION="--numeric-owner"
    else
        TAR_EXTRA_OPTION="--numeric-owner --warning=no-timestamp"
    fi
    mkdir -p $demo_mnt/$image_dir/$DOCKERFS_DIR
    unzip -op $ONIE_INSTALLER_PAYLOAD "$FILESYSTEM_DOCKERFS" | tar xz $TAR_EXTRA_OPTION -f - -C $demo_mnt/$image_dir/$DOCKERFS_DIR
fi

mkdir -p $demo_mnt/$image_dir/platform
unzip -op $ONIE_INSTALLER_PAYLOAD "platform.tar.gz" | tar xz $TAR_EXTRA_OPTION -f - -C $demo_mnt/$image_dir/platform

if [ "$install_env" = "onie" ]; then
    # Store machine description in target file system
    if [ -f /etc/machine-build.conf ]; then
        # onie_ variable are generate at runtime.
        # they are no longer hardcoded in /etc/machine.conf
        # also remove single quotes around the value
        set | grep ^onie | sed -e "s/='/=/" -e "s/'$//" > $demo_mnt/machine.conf
    else
        cp /etc/machine.conf $demo_mnt
    fi
fi

echo "ONIE_IMAGE_PART_SIZE=$demo_part_size"

extra_cmdline_linux=%%EXTRA_CMDLINE_LINUX%%
# Inherit the FIPS option, so not necessary to do another reboot after upgraded
if grep -q '\bsonic_fips=1\b' /proc/cmdline && echo " $extra_cmdline_linux" | grep -qv '\bsonic_fips=.\b'; then
    extra_cmdline_linux="$extra_cmdline_linux sonic_fips=1"
fi

echo "EXTRA_CMDLINE_LINUX=$extra_cmdline_linux"

# Update Bootloader Menu with installed image
bootloader_menu_config

# Set NOS mode if available.  For manufacturing diag installers, you
# probably want to skip this step so that the system remains in ONIE
# "installer" mode for installing a true NOS later.
if [ -x /bin/onie-nos-mode ] ; then
    /bin/onie-nos-mode -s
fi
