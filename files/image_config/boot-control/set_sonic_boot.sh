#!/bin/bash
# set_sonic_boot.sh
#
# This script configures the GRUB environment file on the EFI partition
# to trigger a warm boot into SONiC on the next reboot.
#
# Usage: ./set_sonic_boot.sh

set -e  # Exit immediately if a command exits with a non-zero status
set -u  # Treat unset variables as an error

# Constants
readonly EFI_MOUNT="/efi"
readonly ENV_FILE_NAME="sonie_env"
readonly ENV_FILE_PATH="${EFI_MOUNT}/${ENV_FILE_NAME}"
readonly ESP_PARTLABEL="/dev/disk/by-partlabel/ESP"

# Flag to track if we mounted the EFI partition
MOUNTED_BY_SCRIPT=0

#######################################
# Logging function
# Arguments:
#   $@ - Message to log
# Outputs:
#   Writes the timestamped message to stdout
#######################################
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

#######################################
# Cleanup function to unmount EFI partition if we mounted it
# Globals:
#   MOUNTED_BY_SCRIPT
#   EFI_MOUNT
# Arguments:
#   None
# Outputs:
#   Unmounts the EFI partition.
#######################################
cleanup() {
    if [[ "${MOUNTED_BY_SCRIPT}" -eq 1 ]]; then
        log "Unmounting ${EFI_MOUNT}..."
        umount "${EFI_MOUNT}" || log "Warning: Failed to unmount ${EFI_MOUNT}"
    fi
}

# Trap cleanup on exit
trap cleanup EXIT

#######################################
# Main execution function
# Configures GRUB environment
# Globals:
#   ENV_FILE_PATH
#   ESP_PARTLABEL
#   FALLBACK_ESP_DEV
#   EFI_MOUNT
# Arguments:
#   None
#######################################
main() {
    # Check for required tools
    if ! command -v grub-editenv &> /dev/null; then
        log "Error: grub-editenv could not be found."
        exit 1
    fi

    # Mount EFI partition if not already mounted
    if ! mountpoint -q "${EFI_MOUNT}"; then
        mkdir -p "${EFI_MOUNT}"
        local esp_dev=""

        # 1. Try udev symlink
        if [[ -b "${ESP_PARTLABEL}" ]]; then
            esp_dev="${ESP_PARTLABEL}"
        fi

        # 2. Try blkid by PARTLABEL
        if [[ -z "${esp_dev}" ]] && command -v blkid >/dev/null; then
            esp_dev=$(blkid -t PARTLABEL="ESP" -o device | head -n 1)
        fi

        # 3. Try blkid by filesystem LABEL
        if [[ -z "${esp_dev}" ]] && command -v blkid >/dev/null; then
            esp_dev=$(blkid -t LABEL="ESP" -o device | head -n 1)
        fi

        # 4. Fallback to common devices
        if [[ -z "${esp_dev}" ]]; then
            for dev in /dev/vda1 /dev/sda1 /dev/nvme0n1p1; do
                if [[ -b "${dev}" ]]; then
                    esp_dev="${dev}"
                    break
                fi
            done
        fi

        if [[ -n "${esp_dev}" && -b "${esp_dev}" ]]; then
            log "Mounting partition ${esp_dev} to ${EFI_MOUNT}..."
            mount "${esp_dev}" "${EFI_MOUNT}"
        else
            log "Error: Could not find EFI partition to mount."
            exit 1
        fi
        MOUNTED_BY_SCRIPT=1
    else
        log "EFI partition already mounted at ${EFI_MOUNT}."
    fi

    # Update GRUB environment file
    if [[ ! -f "${ENV_FILE_PATH}" ]]; then
        log "Warning: Environment file ${ENV_FILE_PATH} not found."
        log "Attempting to create/update ${ENV_FILE_PATH} anyway..."
    fi

    # Set warmboot_env to 1 to bypass SONIE check on next boot
    log "Setting warmboot_env=1..."
    grub-editenv "${ENV_FILE_PATH}" set warmboot_env=1

    # Ensure bootcount is valid (2) to give full retries
    # We do NOT touch rollback_env here, preserving user preference
    log "Setting bootcount_env=2..."
    grub-editenv "${ENV_FILE_PATH}" set bootcount_env=2

    log "Successfully updated SONiC boot flags."
}

main "$@"
