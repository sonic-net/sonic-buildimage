#!/bin/sh

#  Copyright (C) 2020      Marvell Inc
#  Copyright (C) 2014-2015 Curt Brune <curt@cumulusnetworks.com>
#  Copyright (C) 2014-2015 david_yang <david_yang@accton.com>
#
#  SPDX-License-Identifier:     GPL-2.0

# Constants
# POSIX sh way to get script directory
if [ -n "${0%/*}" ]; then
    SCRIPT_DIR="$(cd "${0%/*}" && pwd)"
else
    SCRIPT_DIR="$(pwd)"
fi
export SCRIPT_DIR

# Source utils
if [ -r "${SCRIPT_DIR}/utils.sh" ]; then
    . "${SCRIPT_DIR}/utils.sh"
else
    echo "ERROR: utils.sh not found in ${SCRIPT_DIR}" >&2
    exit 1
fi

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

detect_environment() {
    if grep -q 'root=/dev/ram0' /proc/cmdline; then
        log_info "Installing SONiC in SONIE"
        install_env="sonie"
    elif [ -d "/etc/sonic" ]; then
        log_info "Installing SONiC in SONiC"
        install_env="sonic"
    elif grep -Fxqs "DISTRIB_ID=onie" /etc/lsb-release > /dev/null; then
        log_info "Installing SONiC in ONIE"
        install_env="onie"
    else
        log_info "Installing SONiC in BUILD"
        install_env="build"
    fi
    export install_env
}

detect_machine_conf() {
    # get running machine from conf file
    if [ -r /etc/machine.conf ]; then
        machine_conf=/etc/machine.conf
        read_conf_file "$machine_conf"
    elif [ -r /host/machine.conf ]; then
        machine_conf=/host/machine.conf
        read_conf_file "$machine_conf"
    elif [ -r "${SCRIPT_DIR}/machine.conf" ]; then
        machine_conf="${SCRIPT_DIR}/machine.conf"
        read_conf_file "$machine_conf"
    elif [ -r "./machine.conf" ]; then
        machine_conf="./machine.conf"
        read_conf_file "$machine_conf"
    elif [ "$install_env" != "build" ]; then
        log_error "cannot find machine.conf"
        echo "PWD: $(pwd)" >&2
        echo "SCRIPT_DIR: $SCRIPT_DIR" >&2
        ls -la . >&2
        exit 1
    fi

    if [ -z "$onie_platform" ] && [ -n "$platform" ]; then
        onie_platform="$platform"
    fi
}

check_root() {
    if [ $(id -u) -ne 0 ]
        then log_error "Please run as root"
        exit 1
    fi
}

main() {
    detect_environment

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

    log_info "ONIE Installer: platform: $platform"

    check_root
    detect_machine_conf

    log_info "onie_platform: $onie_platform"

    # Get platform specific linux kernel command line arguments
    ONIE_PLATFORM_EXTRA_CMDLINE_LINUX=""

    # Start with build time value, set either using env variable
    # or from onie-image.conf. onie-mk-demo.sh will string replace
    # below value to env value. Platform specific installer.conf
    # will override this value if necessary by reading $onie_platform
    # after this.
    ONIE_IMAGE_PART_SIZE="${ONIE_IMAGE_PART_SIZE:-32768}"

    # Default var/log device size in MB
    VAR_LOG_SIZE=${VAR_LOG_SIZE:-4096}

    [ -r platforms/$onie_platform ] && . platforms/$onie_platform

    # Verify image platform is inside devices list
    if [ "$install_env" = "onie" ] || [ "$install_env" = "sonie" ]; then
        if ! grep -Fxq "$onie_platform" platforms_asic; then
            log_warn "The image you're trying to install is of a different ASIC type as the running platform's ASIC"
            while true; do
                read -r -p "Do you still wish to install this image? [y/n]: " input
                case $input in
                    [Yy])
                        log_info "Force installing..."
                        break
                        ;;
                    [Nn])
                        log_info "Exited installation!"
                        exit 1
                        ;;
                    *)
                        log_error "Error: Invalid input"
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
        onie_initrd_tmp=${onie_initrd_tmp:-/}
    fi

    arch="${arch:-amd64}"

    # The build system prepares this script by replacing %%DEMO-TYPE%%
    # with "OS" or "DIAG".
    demo_type="${demo_type:-OS}"

    # take final partition size after platform installer.conf override
    demo_part_size=$ONIE_IMAGE_PART_SIZE

    # Logic for A/B slot selection
    # 1. Default to A
    # 2. If running SONiC, check which slot is active and pick the other
    TARGET_SLOT="A"
    if [ "$install_env" = "sonic" ]; then
        # Detect running slot from cmdline
        # Expected format: loop=/image-A/... or loop=/image-B/...
        if grep -q "loop=/[^/]*image-A/" /proc/cmdline; then
            log_info "Detected running in Slot A"
            TARGET_SLOT="B"
        elif grep -q "loop=/[^/]*image-B/" /proc/cmdline; then
            log_info "Detected running in Slot B"
            TARGET_SLOT="A"
        else
            log_warn "Could not detect running slot from cmdline, defaulting to A->B rotation (Target: B)"
            if [ -d "/host/image-A" ]; then
                TARGET_SLOT="B"
            fi
        fi
        log_info "Targeting Slot $TARGET_SLOT"
    fi

    image_dir="image-$TARGET_SLOT"
    # Use SONIC_A / SONIC_B as label for A/B logic compatibility
    demo_volume_label="${demo_volume_label:-SONiC-OS}"

    image_version="${image_version:-sonie.0-dirty-20260206.222306}"
    # timestamp is unused currently but kept for reference
    timestamp="$(date -u +%Y%m%d)"

    bootloader_state_machine="./bootloader_state_machine.sh"
    . ./default_platform.conf

    if [ -r ./platform.conf ]; then
        . ./platform.conf
    fi

    if [ "$install_env" = "onie" ]; then
        # Create/format the flash
        create_partition
        mount_partition
    elif [ "$install_env" = "sonie" ]; then
        # Create/format the flash
        create_partition
        mount_partition
    elif [ "$install_env" = "sonic" ]; then
        demo_mnt="/host"
        # Get current SONiC image (grub/aboot/uboot)
        eval running_sonic_revision="$(cat /proc/cmdline | sed -n 's/^.*loop=\/*image-\(\S\+\)\/.*$/\1/p')"
        # Verify SONiC image exists
        if [ ! -d "$demo_mnt/image-$running_sonic_revision" ]; then
            log_error "SONiC installation is corrupted: path $demo_mnt/image-$running_sonic_revision doesn't exist"
            exit 1
        fi
        # Prevent installing existing SONiC if it is running
        if [ "$image_dir" = "image-$running_sonic_revision" ]; then
            log_info "Not installing SONiC version $running_sonic_revision, as current running SONiC has the same version"
            exit 0
        fi
        # Remove extra SONiC images if any
        for f in $demo_mnt/image-* ; do
            if [ -d $f ] && [ "$f" != "$demo_mnt/image-$running_sonic_revision" ] && [ "$f" != "$demo_mnt/$image_dir" ]; then
                log_info "Removing old SONiC installation $f"
                rm -rf $f
            fi
        done
    else
        demo_mnt="build_raw_image_mnt"
        demo_dev=$cur_wd/"target/sonic-vs-amd64.raw"

        mkfs.ext4 -L "$demo_volume_label" $demo_dev

        log_info "Mounting $demo_dev on $demo_mnt..."
        mkdir $demo_mnt
        mount -t auto -o loop $demo_dev $demo_mnt
    fi
    fi

    log_info "Installing SONiC to $demo_mnt/$image_dir"

    # Create target directory or clean it up if exists
    if [ -d $demo_mnt/$image_dir ]; then
        log_info "Directory $demo_mnt/$image_dir/ already exists. Cleaning up..."
        rm -rf $demo_mnt/$image_dir/*
    else
        mkdir $demo_mnt/$image_dir || {
            log_error "Unable to create SONiC directory"
            exit 1
        }
    fi

    # Create symlink for compatibility with sonic-installer which expects image-<version>
    if [ -n "$image_version" ]; then
        log_info "Creating symlink image-$image_version -> $image_dir for compatibility"
        rm -f "$demo_mnt/image-$image_version"
        ln -sf "$image_dir" "$demo_mnt/image-$image_version"
    fi

    # Decompress the file for the file system directly to the partition
    GRUB_EXCLUDE=""
    if [ "$install_env" = "sonie" ]; then
        GRUB_EXCLUDE="boot/grub*"
    fi

    if [ x"$docker_inram" = x"on" ]; then
        # when disk is small, keep dockerfs.tar.gz in disk, expand it into ramfs during initrd
        unzip -o $INSTALLER_PAYLOAD -x "platform.tar.gz" $GRUB_EXCLUDE -d $demo_mnt/$image_dir
    else
        unzip -o $INSTALLER_PAYLOAD -x "$FILESYSTEM_DOCKERFS" "platform.tar.gz" $GRUB_EXCLUDE -d $demo_mnt/$image_dir

        if [ "$install_env" = "onie" ]; then
            TAR_EXTRA_OPTION="--numeric-owner"
        else
            TAR_EXTRA_OPTION="--numeric-owner --warning=no-timestamp"
        fi
        mkdir -p $demo_mnt/$image_dir/$DOCKERFS_DIR
        unzip -op $INSTALLER_PAYLOAD "$FILESYSTEM_DOCKERFS" | tar xz $TAR_EXTRA_OPTION -f - -C $demo_mnt/$image_dir/$DOCKERFS_DIR
    fi

    mkdir -p $demo_mnt/$image_dir/platform
    unzip -op $INSTALLER_PAYLOAD "platform.tar.gz" | tar xz $TAR_EXTRA_OPTION -f - -C $demo_mnt/$image_dir/platform

    if [ "$install_env" = "onie" ] || [ "$install_env" = "sonie" ]; then
        # Store machine description in target file system
        if [ -f /etc/machine-build.conf ]; then
            # onie_ variable are generate at runtime.
            # they are no longer hardcoded in /etc/machine.conf
            # also remove single quotes around the value
            set | grep ^onie | sed -e "s/='/=/" -e "s/'$//" > $demo_mnt/machine.conf
        else
            if [ -n "$machine_conf" ] && [ -r "$machine_conf" ]; then
                cp "$machine_conf" $demo_mnt/machine.conf
            else
                log_warn "machine.conf not found to copy"
            fi
        fi
    fi

    log_info "ONIE_IMAGE_PART_SIZE=$demo_part_size"

    extra_cmdline_linux="${extra_cmdline_linux:-}"
    # Inherit the FIPS option, so not necessary to do another reboot after upgraded
    if grep -q '\bsonic_fips=1\b' /proc/cmdline && echo " $extra_cmdline_linux" | grep -qv '\bsonic_fips=.\b'; then
        extra_cmdline_linux="$extra_cmdline_linux sonic_fips=1"
    fi

    log_info "EXTRA_CMDLINE_LINUX=$extra_cmdline_linux"

    # Ensure XBOOTLDR is mounted at /boot for subsequent installers (sonic-installer)
    # log_info "Preparing environment for subsequent installers..."

    if mountpoint -q /boot; then
        log_info "/boot is already mounted (likely in SONIE environment)."
    else
        log_error "/boot is not mounted. Exiting."
        exit 1
    fi

    if [ ! -d /boot/grub ]; then
        log_error "/boot/grub not found. Exiting."
        exit 1
    else
        log_info "/boot/grub found."
        log_info "Listing /boot/grub contents:"
        ls -l /boot/grub
    fi

    log_info "Ensuring ${demo_mnt} is mounted to /host"
    # unmount /host/grub first
    if mountpoint -q /host/grub; then
        log_info "/host/grub is mounted, unmounting..."
        umount /host/grub || log_warn "Failed to unmount /host/grub"
    fi
    # Ensure /host is mounted so sonic-installer can find image-A/B
    if mountpoint -q /host; then
        log_info "/host is already mounted. Remounting to ensure it points to ${demo_mnt}..."
        umount /host || log_warn "Failed to unmount /host"
    fi

    if [ "$install_env" = "sonie" ]; then
        mount --make-rprivate / || log_warn "Failed to make / rprivate"
    fi
    mount --move "${demo_mnt}" /host
    demo_mnt="/host"

    # Ensure /host/grub points to /boot/grub
    if [ ! -d /host/grub ]; then
        log_info "Creating /host/grub directory..."
        mkdir -p /host/grub || log_error "Failed to create /host/grub"
    else
        log_info "/host/grub already exists, preserving it."
    fi
    # Ensure /boot/grub exists to support binding it to /host/grub
    if [ ! -d /boot/grub ]; then
        mkdir -p /boot/grub
    fi

    if [ -d /boot/grub ]; then
        log_info "/boot/grub found."
        log_info "Listing /boot/grub contents:"
        ls -l /boot/grub
    fi

    if ! mountpoint -q /host/grub; then
        if [ -d /boot/grub ]; then
            log_info "Mounting /boot/grub to /host/grub"
            mount --bind /boot/grub /host/grub || log_error "Failed to bind /boot/grub to /host/grub"
        else
            log_warn "/boot/grub not found, skipping /host/grub bind"
        fi
    else
        log_info "/host/grub is already mounted, preserving it."
    fi

    # Update Bootloader Menu with installed image
    log_info "DEBUG: /host/grub before bootloader_menu_config:"
    ls -la /host/grub || true
    bootloader_menu_config
    log_info "DEBUG: /host/grub after bootloader_menu_config:"
    ls -la /host/grub || true
    # Set NOS mode if available.  For manufacturing diag installers, you
    # probably want to skip this step so that the system remains in ONIE
    # "installer" mode for installing a true NOS later.
    if [ -x /bin/onie-nos-mode ] ; then
        /bin/onie-nos-mode -s
    fi

    # Cleanup temporary mounts manually since we are clearing the trap
    if [ -n "$demo_mnt" ] && mountpoint -q "$demo_mnt"; then
        if [ "$demo_mnt" != "/host" ]; then
            log_info "Unmounting temporary mount $demo_mnt (lazy)"
            umount -l "$demo_mnt" || log_warn "Failed to unmount $demo_mnt"
            rmdir "$demo_mnt" || true
        else
            log_info "Skipping unmount of $demo_mnt (system mount)"
        fi
    fi

    # Clear traps to prevent unmounting on exit, so sonic-installer can access files
    trap - EXIT
}

main "$@"
