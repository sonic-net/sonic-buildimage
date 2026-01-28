#!/bin/bash
## This script is to generate an installer image based on a file system overload

## Enable debug output for script
set -x -e

## Read ONIE image related config file

CONFIGURED_ARCH="$([ -f .arch ] && cat .arch || echo amd64)"

if [[ $CONFIGURED_ARCH == armhf || $CONFIGURED_ARCH == arm64 ]]; then
    if [ -r ./platform/"${CONFIGURED_PLATFORM}"/onie-image-"${CONFIGURED_ARCH}".conf ]; then
        . ./platform/"${CONFIGURED_PLATFORM}"/onie-image-"${CONFIGURED_ARCH}".conf
    else
        . ./onie-image-"${CONFIGURED_ARCH}".conf
    fi
else
    . ./onie-image.conf
fi

[ -n "$ONIE_IMAGE_PART_SIZE" ] || {
    echo "Error: Invalid ONIE_IMAGE_PART_SIZE in onie image config file"
    exit 1
}
[ -n "$XBOOTLDR_PART_SIZE" ] || {
    echo "Error: Invalid XBOOTLDR_PART_SIZE in onie image config file"
    exit 1
}
[ -n "$INSTALLER_PAYLOAD" ] || {
    echo "Error: Invalid INSTALLER_PAYLOAD in onie image config file"
    exit 1
}

IMAGE_VERSION="${SONIC_IMAGE_VERSION}"

#######################################
# Generates a KVM image for a specific ASIC count.
# Globals:
#   OUTPUT_KVM_4ASIC_IMAGE
#   onie_recovery_kvm_4asic_image
#   OUTPUT_KVM_6ASIC_IMAGE
#   onie_recovery_kvm_6asic_image
#   KVM_IMAGE
#   RECOVERY_ISO
#   OUTPUT_ONIE_IMAGE
#   KVM_IMAGE_DISK_SIZE
#   USERNAME
#   PASSWORD
# Arguments:
#   NUM_ASIC: Number of ASICs (4 or 6)
# Returns:
#   0 on success, non-zero on failure.
#######################################
generate_kvm_image()
{
    NUM_ASIC=$1
    if [ "$NUM_ASIC" == 4 ]; then
         KVM_IMAGE=$OUTPUT_KVM_4ASIC_IMAGE
         RECOVERY_ISO=$onie_recovery_kvm_4asic_image
    elif [ "$NUM_ASIC" == 6 ]; then
         KVM_IMAGE=$OUTPUT_KVM_6ASIC_IMAGE
         RECOVERY_ISO=$onie_recovery_kvm_6asic_image
    else
         KVM_IMAGE=$OUTPUT_KVM_IMAGE
         RECOVERY_ISO=$onie_recovery_image
         NUM_ASIC=1
    fi

    echo "Build $NUM_ASIC-asic KVM image"
    KVM_IMAGE_DISK=${KVM_IMAGE%.gz}
    sudo rm -f "$KVM_IMAGE_DISK" "$KVM_IMAGE_DISK.gz"

    SONIC_USERNAME=$USERNAME PASSWD=$PASSWORD sudo -E ./scripts/build_kvm_image.sh "$KVM_IMAGE_DISK" "$RECOVERY_ISO" "$OUTPUT_ONIE_IMAGE" "$KVM_IMAGE_DISK_SIZE" || {
        echo "Error : build kvm image failed"
        exit 1
    }

    [ -r "$KVM_IMAGE_DISK" ] || {
        echo "Error : $KVM_IMAGE_DISK not generated!"
        exit 1
    }

    pigz "$KVM_IMAGE_DISK"

    [ -r "$KVM_IMAGE_DISK.gz" ] || {
        echo "Error : pigz $KVM_IMAGE_DISK failed!"
        exit 1
    }

    echo "The compressed kvm image is in $KVM_IMAGE_DISK.gz"
}

#######################################
# Generates an ONIE installer image.
# Globals:
#   TARGET_PLATFORM
#   VENDOR
#   TARGET_MACHINE
#   CONFIGURED_ARCH
#   ONIE_IMAGE_PART_SIZE
#   IMAGE_TYPE
#   INSTALLER_PAYLOAD
#   FILESYSTEM_ROOT
#   FILESYSTEM_SQUASHFS
#   FILESYSTEM_DOCKERFS
#   DOCKERFS_DIR
#   DOCKER_RAMFS_SIZE
#   OUTPUT_RAW_IMAGE
#   RAW_IMAGE_DISK_SIZE
#   OUTPUT_KVM_IMAGE
#   KVM_IMAGE_DISK_SIZE
#   OUTPUT_ABOOT_IMAGE
#   ABOOT_BOOT_IMAGE
#   FILESYSTEM_ROOT
# Arguments:
#   output_file: The path to the output ONIE installer image.
# Returns:
#   0 on success, non-zero on failure.
#######################################
generate_onie_installer_image()
{
    output_file=$OUTPUT_ONIE_IMAGE
    [ -n "$1" ] && output_file=$1
    echo "DEBUG: generate_onie_installer_image: output_file=${output_file}"

    local part_size="$ONIE_IMAGE_PART_SIZE"
    local installer_dir="installer"
    local platform_arg="$TARGET_PLATFORM-$TARGET_MACHINE-$ONIEIMAGE_VERSION"

    if [[ "$IMAGE_TYPE" = "recovery" || "$IMAGE_TYPE" = "sonie" ]]; then
        part_size="$XBOOTLDR_PART_SIZE"
        installer_dir="sonie-installer"
        platform_arg="$CONFIGURED_PLATFORM"
    fi

    # Copy platform-specific ONIE installer config files where onie-mk-demo.sh expects them
    rm -rf "./$installer_dir/platforms/"
    mkdir -p "./$installer_dir/platforms/"
    for VENDOR in ./device/*; do
        [ -d "${VENDOR}" ] || continue
        VENDOR="${VENDOR##*/}"
        # Iterate over platforms that start with TARGET_PLATFORM
        for PLATFORM in ./device/"${VENDOR}"/"${TARGET_PLATFORM}"*; do
            [ -d "${PLATFORM}" ] || continue
            PLATFORM="${PLATFORM##*/}"
            if [ -f ./device/"$VENDOR"/"$PLATFORM"/installer.conf ]; then
                cp ./device/"$VENDOR"/"$PLATFORM"/installer.conf "./$installer_dir/platforms/$PLATFORM"
            fi

        done
    done

    platform_conf_file="platform/$TARGET_MACHINE/platform_${CONFIGURED_ARCH}.conf"
    if [ ! -f "$platform_conf_file" ]; then
        platform_conf_file="platform/$TARGET_MACHINE/platform.conf"
    fi

    ## Generate an ONIE installer image
    declare -a onie_mk_demo_args=(
        "$CONFIGURED_ARCH"
        "$TARGET_MACHINE"
        "$platform_arg"
        "$installer_dir"
	"$platform_conf_file"
        "$output_file"
        "OS"
        "$IMAGE_VERSION"
        "$part_size"
        "$INSTALLER_PAYLOAD"
        "$SECURE_UPGRADE_SIGNING_CERT"
        "$SECURE_UPGRADE_DEV_SIGNING_KEY"
        "$ONIE_INSTALLER_PAYLOAD"
    )
    # only add acpi.conf if it exists for the platform.
    if [ -f "platform/$TARGET_MACHINE/acpi.conf" ]; then
        onie_mk_demo_args+=("platform/$TARGET_MACHINE/acpi.conf")
    fi
    SECURE_UPGRADE_MODE="${SECURE_UPGRADE_MODE}" \
    SECURE_UPGRADE_SIGNING_CERT="${SECURE_UPGRADE_SIGNING_CERT}" \
    SECURE_UPGRADE_DEV_SIGNING_KEY="${SECURE_UPGRADE_DEV_SIGNING_KEY}" \
    SECURE_UPGRADE_PROD_TOOL_ARGS="${SECURE_UPGRADE_PROD_TOOL_ARGS}" \
    SECURE_UPGRADE_PROD_TOOL_CONFIG="${SECURE_UPGRADE_PROD_TOOL_CONFIG}" \
        echo "DEBUG: onie-mk-demo.sh args: ${onie_mk_demo_args[@]}"
        ./onie-mk-demo.sh "${onie_mk_demo_args[@]}"
}

# Generate asic-specific device list
#######################################
# Generates a list of supported devices/platforms.
# Globals:
#   None
# Arguments:
#   platforms_asic: Output file to write the list to.
# Returns:
#   None
#######################################
generate_device_list()
{
    local platforms_asic=$1

    # Create an empty function, and later append to it
    echo -n > "$platforms_asic"

    while IFS= read -r d; do
        if [ -f "$d"/platform_asic ]; then
            if [ "$TARGET_MACHINE" = "generic" ] || grep -Fxq "$TARGET_MACHINE" "$d"/platform_asic; then
                echo "${d##*/}" >> "$platforms_asic";
            fi;
        fi;
    done < <(find -L ./device -maxdepth 2 -mindepth 2 -type d)

    # Add kvm to the list
    if [ "$TARGET_MACHINE" = "broadcom" ] ; then
      echo "x86_64-kvm_x86_64-r0" >> "$platforms_asic";
    fi

    if [ "$TARGET_MACHINE" = "alpinevs" ] ; then
      echo "x86_64-kvm_x86_64-r0" >> "$platforms_asic";
    fi
}

#######################################
# Generates a SONIE installer image.
# Globals:
#   None
# Arguments:
#   output_file: The path to the output SONIE installer image.
# Returns:
#   0 on success, non-zero on failure.
#######################################
generate_sonie_installer_image()
{
    local output_file=$1
    if [ -z "$output_file" ]; then
        echo "Error: Output file not specified for sonie installer"
        exit 1
    fi
    mkdir -p "$(dirname "$output_file")"
    sudo rm -f "$output_file"
    generate_device_list "./sonie-installer/platforms_asic"
    generate_onie_installer_image "$output_file"
}
#######################################
# Builds the ONIE target (Installer, Raw, KVM).
# Globals:
#   OUTPUT_ONIE_IMAGE
#   OUTPUT_RAW_IMAGE
#   RAW_IMAGE_DISK_SIZE
#   OUTPUT_KVM_IMAGE
#   KVM_IMAGE_DISK_SIZE
#   USERNAME
#   PASSWORD
#   OUTPUT_ONIE_IMAGE
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_onie_image() {
    echo "Build ONIE installer"
    mkdir -p "$(dirname "$OUTPUT_ONIE_IMAGE")"
    sudo rm -f "$OUTPUT_ONIE_IMAGE"

    generate_device_list "./installer/platforms_asic"
    generate_onie_installer_image

    echo "Creating SONiC raw partition : $OUTPUT_RAW_IMAGE of size $RAW_IMAGE_DISK_SIZE MB"
    fallocate -l "${RAW_IMAGE_DISK_SIZE}M" "$OUTPUT_RAW_IMAGE"

    # ensure proc is mounted
    sudo mount proc /proc -t proc || true

    ## Generate a partition dump that can be used to 'dd' in-lieu of using the onie-nos-installer
    ## Run the installer
    ## The 'build' install mode of the installer is used to generate this dump.
    local tmp_output_onie_image
    tmp_output_onie_image=$(mktemp)
    generate_onie_installer_image "$tmp_output_onie_image"
    sudo chmod a+x "$tmp_output_onie_image"
    sudo ./"$tmp_output_onie_image" || {
        ## Failure during 'build' install mode of the installer results in an incomplete raw image.
        ## Delete the incomplete raw image.
        sudo rm -f "$OUTPUT_RAW_IMAGE"
    }
    rm "$tmp_output_onie_image"

    [ -r "$OUTPUT_RAW_IMAGE" ] || {
        echo "Error : $OUTPUT_RAW_IMAGE not generated!"
        exit 1
    }
    echo "The raw partition image is in $OUTPUT_RAW_IMAGE"
    echo "Compressing the raw partition image ..."
    pigz -c "$OUTPUT_RAW_IMAGE" > "$OUTPUT_RAW_IMAGE.gz"
    echo "The compressed raw partition image is in $OUTPUT_RAW_IMAGE.gz"

    echo "Creating SONiC KVM image"
    KVM_IMAGE_DISK=${OUTPUT_KVM_IMAGE%.gz}
    sudo rm -f "$KVM_IMAGE_DISK" "$KVM_IMAGE_DISK.gz"

    SONIC_USERNAME=$USERNAME PASSWD=$PASSWORD sudo -E ./scripts/build_kvm_image.sh "$KVM_IMAGE_DISK" . "$OUTPUT_ONIE_IMAGE" "$KVM_IMAGE_DISK_SIZE" || {
        echo "Error : build kvm image failed"
        exit 1
    }
    [ -r "$KVM_IMAGE_DISK" ] || {
        echo "Error : $KVM_IMAGE_DISK not generated!"
        exit 1
    }
    echo "The kvm image is in $KVM_IMAGE_DISK"

    pigz "$KVM_IMAGE_DISK"
    [ -r "$KVM_IMAGE_DISK.gz" ] || {
        echo "Error : pigz $KVM_IMAGE_DISK failed!"
        exit 1
    }
    echo "The compressed kvm image is in $KVM_IMAGE_DISK.gz"

    generate_kvm_image 4
    generate_kvm_image 6
}

#######################################
# Builds the RAW image target.
# Globals:
#   OUTPUT_ONIE_IMAGE
#   OUTPUT_RAW_IMAGE
#   RAW_IMAGE_DISK_SIZE
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_raw_image() {
    echo "Build RAW image"
    local tmp_output_onie_image=${OUTPUT_ONIE_IMAGE}.tmp
    mkdir -p "$(dirname "$OUTPUT_RAW_IMAGE")"
    sudo rm -f "$OUTPUT_RAW_IMAGE"

    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image "$tmp_output_onie_image"

    echo "Creating SONiC raw partition : $OUTPUT_RAW_IMAGE of size $RAW_IMAGE_DISK_SIZE MB"
    fallocate -l "${RAW_IMAGE_DISK_SIZE}M" "$OUTPUT_RAW_IMAGE"

    # ensure proc is mounted
    sudo mount proc /proc -t proc || true

    ## Generate a partition dump that can be used to 'dd' in-lieu of using the onie-nos-installer
    ## Run the installer
    ## The 'build' install mode of the installer is used to generate this dump.
    sudo chmod a+x "$tmp_output_onie_image"
    sudo ./"$tmp_output_onie_image" || {
        ## Failure during 'build' install mode of the installer results in an incomplete raw image.
        ## Delete the incomplete raw image.
        sudo rm -f "$OUTPUT_RAW_IMAGE"
    }
    rm "$tmp_output_onie_image"

    [ -r "$OUTPUT_RAW_IMAGE" ] || {
        echo "Error : $OUTPUT_RAW_IMAGE not generated!"
        exit 1
    }

    echo "The raw image is in $OUTPUT_RAW_IMAGE"
}

#######################################
# Builds the KVM image target.
# Globals:
#   BUILD_MULTIASIC_KVM
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_kvm_image_logic() {
    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image
    # Generate single asic KVM image
    generate_kvm_image
    if [ "$BUILD_MULTIASIC_KVM" == "y" ]; then
        # Genrate 4-asic KVM image
        generate_kvm_image 4
        # Generate 6-asic KVM image
        generate_kvm_image 6
    fi
}

#######################################
# Builds the Aboot image target.
# Globals:
#   OUTPUT_ABOOT_IMAGE
#   ABOOT_BOOT_IMAGE
#   INSTALLER_PAYLOAD
#   IMAGE_VERSION
#   ENABLE_FIPS
#   SONIC_ENABLE_IMAGE_SIGNATURE
#   CA_CERT
#   SIGNING_KEY
#   SIGNING_CERT
#   TARGET_PATH
#   build_date
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_aboot_image() {
    echo "Build Aboot installer"
    mkdir -p "$(dirname "$OUTPUT_ABOOT_IMAGE")"
    sudo rm -f "$OUTPUT_ABOOT_IMAGE"
    sudo rm -f "$ABOOT_BOOT_IMAGE"
    ## Add main payload
    cp "$INSTALLER_PAYLOAD" "$OUTPUT_ABOOT_IMAGE"
    ## Add Aboot boot0 file
    j2 -f env files/Aboot/boot0.j2 ./onie-image.conf > files/Aboot/boot0
    sed -i -e "s/%%IMAGE_VERSION%%/$IMAGE_VERSION/g" files/Aboot/boot0
    pushd files/Aboot && zip -g "$OLDPWD"/"$OUTPUT_ABOOT_IMAGE" boot0; popd
    pushd files/Aboot && zip -g "$OLDPWD"/"$ABOOT_BOOT_IMAGE" boot0; popd
    pushd files/image_config/secureboot && zip -g "$OLDPWD"/"$OUTPUT_ABOOT_IMAGE" allowlist_paths.conf; popd
    echo "$IMAGE_VERSION" >> .imagehash
    zip -g "$OUTPUT_ABOOT_IMAGE" .imagehash
    zip -g "$ABOOT_BOOT_IMAGE" .imagehash
    rm .imagehash
    echo "SWI_VERSION=42.0.0" > version
    {
        echo "BUILD_DATE=$(date -d "${build_date}" -u +%Y%m%dT%H%M%SZ)"
        echo "SWI_MAX_HWEPOCH=2"
        echo "SWI_VARIANT=US"
    } >> version
    zip -g "$OUTPUT_ABOOT_IMAGE" version
    zip -g "$ABOOT_BOOT_IMAGE" version
    rm version

    generate_device_list ".platforms_asic"
    zip -g "$OUTPUT_ABOOT_IMAGE" .platforms_asic

    if [ "$ENABLE_FIPS" = "y" ]; then
        echo "sonic_fips=1" >> kernel-cmdline-append
    else
        echo "sonic_fips=0" >> kernel-cmdline-append
    fi
    zip -g "$OUTPUT_ABOOT_IMAGE" kernel-cmdline-append
    rm kernel-cmdline-append

    zip -g "$OUTPUT_ABOOT_IMAGE" "$ABOOT_BOOT_IMAGE"
    rm "$ABOOT_BOOT_IMAGE"
    if [ "$SONIC_ENABLE_IMAGE_SIGNATURE" = "y" ]; then
        TARGET_CA_CERT="$TARGET_PATH/ca.cert"
        rm -f "$TARGET_CA_CERT"
        [ -f "$CA_CERT" ] && cp "$CA_CERT" "$TARGET_CA_CERT"
        ./scripts/sign_image.sh -i "$OUTPUT_ABOOT_IMAGE" -k "$SIGNING_KEY" -c "$SIGNING_CERT" -a "$TARGET_CA_CERT"
    fi
}

#######################################
# Builds the DSC image target.
# Globals:
#   OUTPUT_DSC_IMAGE
#   files/dsc
#   INSTALLER_PAYLOAD
#   OUTPUT_ONIE_IMAGE
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_dsc_image() {
    echo "Build DSC installer"

    local dsc_installer_dir="files/dsc"
    local dsc_installer="$dsc_installer_dir/install_debian"
    local dsc_installer_manifest="$dsc_installer_dir/MANIFEST"

    mkdir -p "$(dirname "$OUTPUT_DSC_IMAGE")"
    sudo rm -f "$OUTPUT_DSC_IMAGE"

    . ./onie-image.conf

    j2 "$dsc_installer.j2" > "$dsc_installer"
    installer_sha=$(sha512sum "$dsc_installer" | awk '{print $1}')
    export installer_sha

    build_date=$(date -u)
    export build_date
    build_user=$(id -un)
    export build_user
    installer_payload_sha=$(sha512sum "$INSTALLER_PAYLOAD" | awk '{print $1}')
    export installer_payload_sha
    j2 "$dsc_installer_manifest.j2" > "$dsc_installer_manifest"

    cp "$INSTALLER_PAYLOAD" "$dsc_installer_dir"
    tar cf "$OUTPUT_DSC_IMAGE" -C files/dsc "$(basename "$dsc_installer_manifest")" "$INSTALLER_PAYLOAD" "$(basename "$dsc_installer")"

    echo "Build ONIE installer"
    mkdir -p "$(dirname "$OUTPUT_ONIE_IMAGE")"
    sudo rm -f "$OUTPUT_ONIE_IMAGE"

    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image
}

#######################################
# Builds the Recovery image target.
# Globals:
#   FILESYSTEM_ROOT
#   CONFIGURED_ARCH
#   OUTPUT_RECOVERY_IMAGE
#   LINUX_KERNEL_VERSION
#   SIGNING_KEY
#   SIGNING_CERT
#   GZ_COMPRESS_PROGRAM
#   BOOT_IMAGE_TYPE
#   IMAGE_TYPE
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_recovery_image() {
    echo "Constructing SONIE recovery image"

    # Use separate filesystem root for recovery to allow parallel builds
    FILESYSTEM_ROOT="${FILESYSTEM_ROOT}-recovery"

    # Check if filesystem exists
    if [ ! -d "${FILESYSTEM_ROOT}" ]; then
        echo "Error: Filesystem root '${FILESYSTEM_ROOT}' does not exist!"
        exit 1
    fi

    # Derive LINUX_KERNEL_VERSION if not already set
    if [ -z "${LINUX_KERNEL_VERSION}" ]; then
        # Look for vmlinuz file in boot directory
        # Expected format: vmlinuz-<version>-<arch>
        kernel_file=$(find "${FILESYSTEM_ROOT}/boot" -name "vmlinuz-*-${CONFIGURED_ARCH}" -print -quit)

        if [ -n "${kernel_file}" ]; then
            kernel_fname=$(basename "${kernel_file}")
            # Remove 'vmlinuz-' prefix
            ver_arch="${kernel_fname#vmlinuz-}"
            # Remove '-<arch>' suffix
            LINUX_KERNEL_VERSION="${ver_arch%-"${CONFIGURED_ARCH}"}"
            echo "Auto-detected LINUX_KERNEL_VERSION: ${LINUX_KERNEL_VERSION}"
        else
            echo "Error: Could not detect LINUX_KERNEL_VERSION from ${FILESYSTEM_ROOT}/boot"
            exit 1
        fi
    fi

    # Disable networking.service to prevent ifupdown from starting standard networking on boot
    # This avoids conflicts with ONIE discovery in recovery mode.
    sudo LANG=C chroot "${FILESYSTEM_ROOT}" systemctl disable networking.service || true

    # Export variables required by recovery_image_build.sh
    export FILESYSTEM_ROOT
    export OUTPUT_RECOVERY_IMAGE
    export LINUX_KERNEL_VERSION
    export CONFIGURED_ARCH
    export SIGNING_KEY
    export SIGNING_CERT
    export GZ_COMPRESS_PROGRAM
    export BOOT_IMAGE_TYPE
    export IMAGE_TYPE

    # Invoke the build script
    ./recovery_image_build.sh
}

#######################################
# Builds the SONIE installer target.
# Globals:
#   SONIE_INSTALLER_PAYLOAD
#   OUTPUT_ONIE_IMAGE
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
build_sonie_image() {
     local output_file="$1"
     echo "Build SONIE installer: ${output_file}"

     if [ -z "$SONIE_INSTALLER_PAYLOAD" ]; then
         echo "Error: SONIE_INSTALLER_PAYLOAD not set for 'sonie' image type"
         exit 1
     fi
     if [ ! -f "$SONIE_INSTALLER_PAYLOAD" ]; then
         echo "Error: Payload $SONIE_INSTALLER_PAYLOAD not found"
         exit 1
     fi

     echo "DEBUG: Calling generate_sonie_installer_image with ${output_file}"
     generate_sonie_installer_image "$output_file"
     echo "DEBUG: generate_sonie_installer_image finished"
}


#######################################
# Main execution entry point.
# Globals:
#   IMAGE_TYPE
#   OUTPUT_ONIE_IMAGE
#   SONIC_IMAGE_VERSION
#   OUTPUT_RAW_IMAGE
#   RAW_IMAGE_DISK_SIZE
#   OUTPUT_KVM_IMAGE
#   KVM_IMAGE_DISK_SIZE
#   OUTPUT_ABOOT_IMAGE
#   FILESYSTEM_ROOT
#   FILESYSTEM_SQUASHFS
#   INSTALLER_PAYLOAD
#   OUTPUT_DSC_IMAGE
#   SONIE_INSTALLER_PAYLOAD
#   LINUX_KERNEL_VERSION
#   CONFIGURED_ARCH
# Arguments:
#   None
# Returns:
#   0 on success, non-zero on failure.
#######################################
main() {
    case "$IMAGE_TYPE" in
        onie)
            build_onie_image
            ;;
        raw)
            build_raw_image
            ;;
        kvm)
            build_kvm_image_logic
            ;;
        aboot)
            build_aboot_image
            ;;
        dsc)
            build_dsc_image
            ;;
        recovery)
            build_recovery_image
            ;;
        sonie)
            build_sonie_image "$@"
            ;;
        *)
            echo "Error: Non supported image type $IMAGE_TYPE"
            exit 1
            ;;
    esac
}

main "$@"
