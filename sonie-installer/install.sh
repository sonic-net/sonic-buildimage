#!/bin/sh
# shellcheck shell=ash
#
# Copyright (C) 2020      Marvell Inc
# Copyright (C) 2014-2015 Curt Brune <curt@cumulusnetworks.com>
# Copyright (C) 2014-2015 david_yang <david_yang@accton.com>
#
# SPDX-License-Identifier: GPL-2.0

set -e

# Constants
# POSIX sh way to get script directory (assuming called by path or in path)
if [ -n "${0%/*}" ]; then
    SCRIPT_DIR="${0%/*}"
else
    SCRIPT_DIR="."
fi
export SCRIPT_DIR
export VAR_LOG_SIZE=4096

# Global state
export install_env=""
export is_ram_root=0
export is_onie=0
export platform=""
export onie_platform=""

export demo_type="%%DEMO_TYPE%%"
export demo_part_size="%%ONIE_IMAGE_PART_SIZE%%"
image_version="%%IMAGE_VERSION%%"
export image_version
timestamp="$(date -u +%Y%m%d)"
export timestamp
export demo_volume_label="SONIE-${demo_type}"
export demo_volume_revision_label="SONIE-${demo_type}-${image_version}"

# Alias for default_platform.conf compatibility
export sonie_part_size="${demo_part_size}"

#######################################
# Logs an info message to stdout.
# Globals:
#   None
# Arguments:
#   Message to log
# Returns:
#   None
#######################################
log_info() { printf "INFO: %s\n" "$*"; }

#######################################
# Logs a warning message to stderr.
# Globals:
#   None
# Arguments:
#   Message to log
# Returns:
#   None
#######################################
log_warn() { printf "WARN: %s\n" "$*" >&2; }

#######################################
# Logs an error message to stderr.
# Globals:
#   None
# Arguments:
#   Message to log
# Returns:
#   None
#######################################
log_error() { printf "ERROR: %s\n" "$*" >&2; }

#######################################
# Appends a command to a trap, preserving existing traps.
# Globals:
#   None
# Arguments:
#   next: The command to append to the trap.
# Returns:
#   None
#######################################
_trap_push() {
    local next="${1}"
    eval "trap_push() {
        local oldcmd='$(echo "${next}" | sed -e s/\'/\'\\\\\'\'/g)'
        local newcmd=\"\${1}; \${oldcmd}\"
        trap -- \"\${newcmd}\" EXIT INT TERM HUP
        _trap_push \"\${newcmd}\"
    }"
}

#######################################
# Reads a configuration file and exports variables.
# Globals:
#   None
# Arguments:
#   conf_file: Path to the configuration file.
# Returns:
#   None
#######################################
read_conf_file() {
    local conf_file="${1}"
    local var value
    while IFS='=' read -r var value || [ -n "${var}" ]; do
        # Cleanup: remove newline, comments, quotes
        var="$(printf "%s" "${var}" | tr -d '\r\n')"
        value="$(printf "%s" "${value}" | tr -d '\r\n')"
        var="${var%#*}"
        value="${value%#*}"

        [ -z "${var}" ] && continue

        value="${value%\"}"
        value="${value#\"}"

        # Eval implementation to set variable
        eval "${var}=\"${value}\""
    done < "${conf_file}"
}

#######################################
# Detects the execution environment (ONIE, SONiC, or Build).
# Globals:
#   is_ram_root
#   is_onie
#   install_env
# Arguments:
#   None
# Returns:
#   None
#######################################
detect_environment() {
    if [ -r /proc/mounts ]; then
        while read -r _ mountpoint fstype _; do
            if [ "${mountpoint}" = "/" ]; then
                case "${fstype}" in
                    rootfs|tmpfs|ramfs) is_ram_root=1 ;;
                esac
                break
            fi
        done < /proc/mounts
    fi

    if grep -Fxqs "DISTRIB_ID=onie" /etc/lsb-release >/dev/null; then
        is_onie=1
    fi

    if grep -q 'root=/dev/ram0' /proc/cmdline || [ "${is_ram_root}" -eq 1 ]; then
        log_info "Installing SONIE in SONIE (Running from RAM-based rootfs)"
        install_env="sonie"
    elif [ -d "/etc/sonic" ]; then
        log_info "Installing SONIE in SONiC"
        install_env="sonic"
    elif [ "${is_onie}" -eq 1 ]; then
        log_info "Installing SONIE in ONIE"
        install_env="onie"
    else
        log_info "Installing SONIE in BUILD"
        install_env="build"
    fi
}

#######################################
# Loads configuration files.
# Globals:
#   SCRIPT_DIR
#   platform
#   install_env
# Arguments:
#   None
# Returns:
#   None
#######################################
load_configs() {
    [ -r "${SCRIPT_DIR}/machine.conf" ] && read_conf_file "${SCRIPT_DIR}/machine.conf"

    # shellcheck disable=SC1091
    [ -r "${SCRIPT_DIR}/onie-image.conf" ] && . "${SCRIPT_DIR}/onie-image.conf"

    # shellcheck disable=SC1090
    for f in "${SCRIPT_DIR}"/onie-image-*.conf; do
        [ -r "${f}" ] && . "${f}"
    done

    log_info "SONIE Installer: platform: ${platform:-unknown}"
    log_info "                 install_env: ${install_env}"
}

#######################################
# Checks if the script is running as root.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   Exits with status 1 if not root.
#######################################
check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        log_error "Please run as root"
        exit 1
    fi
}

#######################################
# Detects machine configuration.
# Globals:
#   install_env
#   onie_platform
# Arguments:
#   None
# Returns:
#   None
#######################################
detect_machine_conf() {
    if [ -r /etc/machine.conf ]; then
        read_conf_file "/etc/machine.conf"
    elif [ -r /host/machine.conf ]; then
        read_conf_file "/host/machine.conf"
    elif [ "${install_env}" != "build" ]; then
        log_error "cannot find machine.conf"
        exit 1
    fi
    log_info "onie_platform: ${onie_platform:-unknown}"
}

#######################################
# Checks if the ASIC platform matches.
# Globals:
#   onie_platform
#   install_env
# Arguments:
#   None
# Returns:
#   None
#######################################
check_asic_platform() {
    if [ -r "platforms/${onie_platform}" ]; then
        # shellcheck disable=SC1090
        . "platforms/${onie_platform}"
    fi

    if [ "${install_env}" = "onie" ]; then
        if ! grep -Fxq "${onie_platform}" platforms_asic; then
            log_warn "The image you're trying to install is of a different ASIC type as the running platform's ASIC"
            while true; do
                printf "Do you still wish to install this image? [y/n]: "
                read -r input
                case "${input}" in
                    [Yy]*)
                        log_info "Force installing..."
                        break
                        ;;
                    [Nn]*)
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
}

#######################################
# Installs GRUB to the EFI System Partition (ESP).
# Globals:
#   None
# Arguments:
#   blk_dev: Block device.
#   demo_mnt: Mount point of the demo installation.
# Returns:
#   None
#######################################
install_esp_bootloader() {
    local blk_dev="$1"
    local demo_mnt="$2"

    # Mount ESP
    # Use make_partition_dev instead of sed hack
    local esp_dev
    esp_dev="$(make_partition_dev "${blk_dev}" 1)"

    local esp_mnt
    esp_mnt="$(mktemp -d)" || { log_error "Failed to create temp dir for ESP"; exit 1; }
    
    # Ensure cleanup on exit
    _trap_push "umount \"${esp_mnt}\" >/dev/null 2>&1; rmdir \"${esp_mnt}\" >/dev/null 2>&1"
    
    mount "${esp_dev}" "${esp_mnt}" || { log_error "Failed to mount ESP ${esp_dev}"; exit 1; }

    install_grub_to_esp "${blk_dev}" "${esp_mnt}" "${demo_mnt}"

    umount "${esp_mnt}"
    rmdir "${esp_mnt}"
}

# Export these for default_platform.conf usage
export bootloader_state_machine="${SCRIPT_DIR}/bootloader_state_machine.grub"

main() {
    _trap_push true
    detect_environment
    load_configs
    check_root
    detect_machine_conf

    # Vars for ONIE
    if [ "${install_env}" = "onie" ]; then
        export onie_bin=""
        export onie_root_dir="/mnt/onie-boot/onie"
        export onie_initrd_tmp="/"
    fi

    check_asic_platform

    # Source platform defaults
    # shellcheck disable=SC1091
    . "${SCRIPT_DIR}/default_platform.conf"

    # shellcheck disable=SC1091
    if [ -r "${SCRIPT_DIR}/platform.conf" ]; then
        . "${SCRIPT_DIR}/platform.conf"
    fi

    export sonie_mnt=""

    local demo_mnt
    if [ "${install_env}" = "sonie" ] || [ "${install_env}" = "onie" ] || [ "${install_env}" = "sonic" ]; then
        create_partition
        mount_partition
        # sonie_mnt is set by mount_partition
        demo_mnt="${sonie_mnt}"
    else
        demo_mnt="build_raw_image_mnt"
        local demo_dev="${PWD}/%%OUTPUT_RAW_IMAGE%%"

        mkfs.ext4 -L "${demo_volume_label}" "${demo_dev}"
        log_info "Mounting ${demo_dev} on ${demo_mnt}..."
        mkdir -p "${demo_mnt}"
        mount -t auto -o loop "${demo_dev}" "${demo_mnt}"
    fi

    log_info "Installing SONIE to ${demo_mnt}"

    if [ -z "${ONIE_INSTALLER_PAYLOAD:-}" ]; then
         log_error "ONIE_INSTALLER_PAYLOAD is not set"
         exit 1
    fi

    unzip -o -j "${ONIE_INSTALLER_PAYLOAD}" -x "*/boot/*" "boot/*" -d "${demo_mnt}"

    # ESP Installation
    # Skip in build mode as it typically doesn't have a physical block device associated
    if [ "${install_env}" != "build" ]; then
        install_esp_bootloader "${blk_dev}" "${demo_mnt}"
    fi
}

main "$@"
