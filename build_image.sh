#!/bin/bash
## This script is to generate an ONIE installer image based on a file system overload

## Enable debug output for script
set -x -e

## Read ONIE image related config file

CONFIGURED_ARCH=$([ -f .arch ] && cat .arch || echo amd64)

if [[ $CONFIGURED_ARCH == armhf || $CONFIGURED_ARCH == arm64 ]]; then
    if [ -r ./platform/${CONFIGURED_PLATFORM}/onie-image-${CONFIGURED_ARCH}.conf ]; then
        . ./platform/${CONFIGURED_PLATFORM}/onie-image-${CONFIGURED_ARCH}.conf
    else
        . ./onie-image-${CONFIGURED_ARCH}.conf
    fi
else
    . ./onie-image.conf
fi

[ -n "$ONIE_IMAGE_PART_SIZE" ] || {
    echo "Error: Invalid ONIE_IMAGE_PART_SIZE in onie image config file"
    exit 1
}
[ -n "$INSTALLER_PAYLOAD" ] || {
    echo "Error: Invalid INSTALLER_PAYLOAD in onie image config file"
    exit 1
}

IMAGE_VERSION="${SONIC_IMAGE_VERSION}"

generate_kvm_image()
{
    NUM_ASIC=$1
    if [ $NUM_ASIC == 4 ]; then 
         KVM_IMAGE=$OUTPUT_KVM_4ASIC_IMAGE
         RECOVERY_ISO=$onie_recovery_kvm_4asic_image
    elif [ $NUM_ASIC == 6 ]; then
         KVM_IMAGE=$OUTPUT_KVM_6ASIC_IMAGE
         RECOVERY_ISO=$onie_recovery_kvm_6asic_image
    else 
         KVM_IMAGE=$OUTPUT_KVM_IMAGE
         RECOVERY_ISO=$onie_recovery_image
         NUM_ASIC=1
    fi

    echo "Build $NUM_ASIC-asic KVM image"
    KVM_IMAGE_DISK=${KVM_IMAGE%.gz}
    sudo rm -f $KVM_IMAGE_DISK $KVM_IMAGE_DISK.gz

    SONIC_USERNAME=$USERNAME PASSWD=$PASSWORD sudo -E ./scripts/build_kvm_image.sh $KVM_IMAGE_DISK $RECOVERY_ISO $OUTPUT_ONIE_IMAGE $KVM_IMAGE_DISK_SIZE

    if [ $? -ne 0 ]; then
        echo "Error : build kvm image failed"
        exit 1
    fi

    [ -r $KVM_IMAGE_DISK ] || {
        echo "Error : $KVM_IMAGE_DISK not generated!"
        exit 1
    }

    $GZ_COMPRESS_PROGRAM $KVM_IMAGE_DISK

    [ -r $KVM_IMAGE_DISK.gz ] || {
        echo "Error : $GZ_COMPRESS_PROGRAM $KVM_IMAGE_DISK failed!"
        exit 1
    }

    echo "The compressed kvm image is in $KVM_IMAGE_DISK.gz"
}

generate_onie_installer_image()
{
    output_file=$OUTPUT_ONIE_IMAGE
    [ -n "$1" ] && output_file=$1
    # Copy platform-specific ONIE installer config files where onie-mk-demo.sh expects them
    rm -rf ./installer/platforms/
    mkdir -p ./installer/platforms/
    for VENDOR in `ls ./device`; do
        for PLATFORM in `ls ./device/$VENDOR | grep ^${TARGET_PLATFORM}`; do
            if [ -f ./device/$VENDOR/$PLATFORM/installer.conf ]; then
                cp ./device/$VENDOR/$PLATFORM/installer.conf ./installer/platforms/$PLATFORM
            fi

        done
    done

    platform_conf_file="platform/$TARGET_MACHINE/platform_${CONFIGURED_ARCH}.conf"
    if [ ! -f $platform_conf_file ]; then
        platform_conf_file="platform/$TARGET_MACHINE/platform.conf"
    fi
    ## Generate an ONIE installer image
    ## Note: Don't leave blank between lines. It is single line command.
    ./onie-mk-demo.sh $CONFIGURED_ARCH $TARGET_MACHINE $TARGET_PLATFORM-$TARGET_MACHINE-$ONIEIMAGE_VERSION \
          installer $platform_conf_file $output_file OS $IMAGE_VERSION $ONIE_IMAGE_PART_SIZE \
          $INSTALLER_PAYLOAD $SECURE_UPGRADE_SIGNING_CERT $SECURE_UPGRADE_DEV_SIGNING_KEY
}

# Generate asic-specific device list
generate_device_list()
{
    local platforms_asic=$1

    # Create an empty function, and later append to it
    echo -n > $platforms_asic

    for d in `find -L ./device  -maxdepth 2 -mindepth 2 -type d`; do
        if [ -f $d/platform_asic ]; then
            if [ "$TARGET_MACHINE" = "generic" ] || grep -Fxq "$TARGET_MACHINE" $d/platform_asic; then
                echo "${d##*/}" >> "$platforms_asic";
            fi;
        fi;
    done
}

if [ "$IMAGE_TYPE" = "onie" ]; then
    echo "Build ONIE installer"
    mkdir -p `dirname $OUTPUT_ONIE_IMAGE`
    sudo rm -f $OUTPUT_ONIE_IMAGE

    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image

## Build a raw partition dump image using the ONIE installer that can be
## used to dd' in-lieu of using the onie-nos-installer. Used while migrating
## into SONiC from other NOS.
elif [ "$IMAGE_TYPE" = "raw" ]; then

    echo "Build RAW image"
    tmp_output_onie_image=${OUTPUT_ONIE_IMAGE}.tmp
    mkdir -p `dirname $OUTPUT_RAW_IMAGE`
    sudo rm -f $OUTPUT_RAW_IMAGE

    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image "$tmp_output_onie_image"

    echo "Creating SONiC raw partition : $OUTPUT_RAW_IMAGE of size $RAW_IMAGE_DISK_SIZE MB"
    fallocate -l "$RAW_IMAGE_DISK_SIZE"M $OUTPUT_RAW_IMAGE

    # ensure proc is mounted
    sudo mount proc /proc -t proc || true

    ## Generate a partition dump that can be used to 'dd' in-lieu of using the onie-nos-installer
    ## Run the installer
    ## The 'build' install mode of the installer is used to generate this dump.
    sudo chmod a+x $tmp_output_onie_image
    sudo ./$tmp_output_onie_image || {
        ## Failure during 'build' install mode of the installer results in an incomplete raw image.
        ## Delete the incomplete raw image.
        sudo rm -f $OUTPUT_RAW_IMAGE
    }
    rm $tmp_output_onie_image

    [ -r $OUTPUT_RAW_IMAGE ] || {
        echo "Error : $OUTPUT_RAW_IMAGE not generated!"
        exit 1
    }

    echo "The raw image is in $OUTPUT_RAW_IMAGE"

elif [ "$IMAGE_TYPE" = "kvm" ]; then

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


## Use 'aboot' as target machine category which includes Aboot as bootloader
elif [ "$IMAGE_TYPE" = "aboot" ]; then
    echo "Build Aboot installer"
    mkdir -p `dirname $OUTPUT_ABOOT_IMAGE`
    sudo rm -f $OUTPUT_ABOOT_IMAGE
    sudo rm -f $ABOOT_BOOT_IMAGE
    ## Add main payload
    cp $INSTALLER_PAYLOAD $OUTPUT_ABOOT_IMAGE
    ## Add Aboot boot0 file
    j2 -f env files/Aboot/boot0.j2 ./onie-image.conf > files/Aboot/boot0
    sed -i -e "s/%%IMAGE_VERSION%%/$IMAGE_VERSION/g" files/Aboot/boot0
    pushd files/Aboot && zip -g $OLDPWD/$OUTPUT_ABOOT_IMAGE boot0; popd
    pushd files/Aboot && zip -g $OLDPWD/$ABOOT_BOOT_IMAGE boot0; popd
    pushd files/image_config/secureboot && zip -g $OLDPWD/$OUTPUT_ABOOT_IMAGE allowlist_paths.conf; popd
    echo "$IMAGE_VERSION" >> .imagehash
    zip -g $OUTPUT_ABOOT_IMAGE .imagehash
    zip -g $ABOOT_BOOT_IMAGE .imagehash
    rm .imagehash
    echo "SWI_VERSION=42.0.0" > version
    echo "BUILD_DATE=$(date -d "${build_date}" -u +%Y%m%dT%H%M%SZ)" >> version
    echo "SWI_MAX_HWEPOCH=2" >> version
    echo "SWI_VARIANT=US" >> version
    zip -g $OUTPUT_ABOOT_IMAGE version
    zip -g $ABOOT_BOOT_IMAGE version
    rm version

    generate_device_list ".platforms_asic"
    zip -g $OUTPUT_ABOOT_IMAGE .platforms_asic

    if [ "$ENABLE_FIPS" = "y" ]; then
        echo "sonic_fips=1" >> kernel-cmdline-append
    else
        echo "sonic_fips=0" >> kernel-cmdline-append
    fi
    zip -g $OUTPUT_ABOOT_IMAGE kernel-cmdline-append
    rm kernel-cmdline-append

    zip -g $OUTPUT_ABOOT_IMAGE $ABOOT_BOOT_IMAGE
    rm $ABOOT_BOOT_IMAGE
    if [ "$SONIC_ENABLE_IMAGE_SIGNATURE" = "y" ]; then
        TARGET_CA_CERT="$TARGET_PATH/ca.cert"
        rm -f "$TARGET_CA_CERT"
        [ -f "$CA_CERT" ] && cp "$CA_CERT" "$TARGET_CA_CERT"
        ./scripts/sign_image.sh -i "$OUTPUT_ABOOT_IMAGE" -k "$SIGNING_KEY" -c "$SIGNING_CERT" -a "$TARGET_CA_CERT"
    fi

elif [ "$IMAGE_TYPE" = "dsc" ]; then
    echo "Build DSC installer"

    dsc_installer_dir=files/dsc
    dsc_installer=$dsc_installer_dir/install_debian
    dsc_installer_manifest=$dsc_installer_dir/MANIFEST

    mkdir -p `dirname $OUTPUT_DSC_IMAGE`
    sudo rm -f $OUTPUT_DSC_IMAGE

    source ./onie-image.conf

    j2 $dsc_installer.j2 > $dsc_installer
    export installer_sha=$(sha512sum "$dsc_installer" | awk '{print $1}')

    export build_date=$(date -u)
    export build_user=$(id -un)
    export installer_payload_sha=$(sha512sum "$INSTALLER_PAYLOAD" | awk '{print $1}')
    j2 $dsc_installer_manifest.j2 > $dsc_installer_manifest

    cp $INSTALLER_PAYLOAD $dsc_installer_dir
    tar cf $OUTPUT_DSC_IMAGE -C files/dsc $(basename $dsc_installer_manifest) $INSTALLER_PAYLOAD $(basename $dsc_installer)

    echo "Build ONIE installer"
    mkdir -p `dirname $OUTPUT_ONIE_IMAGE`
    sudo rm -f $OUTPUT_ONIE_IMAGE

    generate_device_list "./installer/platforms_asic"

    generate_onie_installer_image

else
    echo "Error: Non supported image type $IMAGE_TYPE"
    exit 1
fi
