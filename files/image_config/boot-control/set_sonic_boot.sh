#!/bin/bash

# Ensure we have the necessary tools
if ! command -v grub-editenv &> /dev/null; then
    echo "grub-editenv could not be found"
    exit 1
fi

EFI_MOUNT="/mnt/efi"
ENV_FILE="${EFI_MOUNT}/sonie_env"
MOUNTED_BY_SCRIPT=0

# Check if EFI partition is mounted
if ! mountpoint -q "${EFI_MOUNT}"; then
    mkdir -p "${EFI_MOUNT}"
    # Try to detect EFI partition. Fallback to /dev/sda1 common in SONiC/SONIE
    # In future, use blkid to find PARTLABEL="ESP"
    if [ -b "/dev/disk/by-partlabel/ESP" ]; then
        mount "/dev/disk/by-partlabel/ESP" "${EFI_MOUNT}"
    elif [ -b "/dev/sda1" ]; then
        mount "/dev/sda1" "${EFI_MOUNT}"
    else
        echo "Could not find EFI partition to mount."
        exit 1
    fi
    MOUNTED_BY_SCRIPT=1
fi

if [ ! -f "${ENV_FILE}" ]; then
    echo "Environment file ${ENV_FILE} not found."
    if [ "${MOUNTED_BY_SCRIPT}" -eq 1 ]; then
        umount "${EFI_MOUNT}"
    fi
    # Proceed to attempt efibootmgr even if env file missing (though unlikely in SONIE)
    echo "WARNING: ${ENV_FILE} missing, but attempting to set BootNext anyway."
else
    # Set warmboot_env to 1 to bypass SONIE check on next boot
    # This ensures we attempt to boot the NOS (SONiC)
    grub-editenv "${ENV_FILE}" set warmboot_env=1

    # Also ensure bootcount is valid (2) to give full retries
    # We do NOT touch rollback_env here, preserving user preference
    grub-editenv "${ENV_FILE}" set bootcount_env=2

    echo "Updated SONiC boot flags: warmboot_env=1, bootcount_env=2"
fi

if [ "${MOUNTED_BY_SCRIPT}" -eq 1 ]; then
    umount "${EFI_MOUNT}"
fi

exit 0
