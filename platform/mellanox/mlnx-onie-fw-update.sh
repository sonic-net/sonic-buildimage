#!/bin/bash
#
# Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


this_script="$(basename $(realpath ${0}))"
lock_file="/var/run/${this_script%.*}.lock"

onie_mount=/mnt/onie-boot
onie_lib=/lib/onie
os_boot=/host

print_help() {
cat <<EOF
  update
    The 'update' command will reboot system to ONIE update mode
    and ONIE will perform automatically update of previously
    added (i.e. pending) FW (ONIE itself, BIOS or CPLD) image.

EOF
}

enable_onie_access() {
    if [[ ! -d "${onie_mount}" ]]; then
        mkdir ${onie_mount}
    fi

    if ! mountpoint -q "${onie_mount}"; then
        mount LABEL="ONIE-BOOT" ${onie_mount}
    fi

    if [[ ! -e "${onie_lib}" ]]; then
        ln -s ${onie_mount}/onie/tools/lib/onie ${onie_lib}
    fi
}

disable_onie_access() {
    if [[ -e "${onie_lib}" ]]; then
        unlink ${onie_lib}
    fi

    if mountpoint -q "${onie_mount}"; then
        umount -rf ${onie_mount}
    fi

    if [[ -d "${onie_mount}" ]]; then
        rmdir ${onie_mount}
    fi
}

enable_onie_fw_update_mode() {
    if [[ ! -f ${os_boot}/grub/grubenv || ! -f ${onie_mount}/grub/grubenv ]]; then
        return 1
    fi

    register_terminate_handler

    grub-editenv ${os_boot}/grub/grubenv set onie_entry="ONIE" || return $?
    grub-editenv ${onie_mount}/grub/grubenv set onie_mode="update" || return $?

    return 0
}

disable_onie_fw_update_mode() {
    if [[ ! -f ${os_boot}/grub/grubenv || ! -f ${onie_mount}/grub/grubenv ]]; then
        return 1
    fi

    grub-editenv ${os_boot}/grub/grubenv unset onie_entry || return $?
    grub-editenv ${onie_mount}/grub/grubenv set onie_mode="install" || return $?

    return 0
}

show_pending() {
    if [[ ! -d ${onie_mount}/onie/update/pending ]]; then
        return 0
    fi

    num=$(find ${onie_mount}/onie/update/pending -type f | wc -l)
    if [[ ${num} -ge 1 ]]; then
        ${onie_mount}/onie/tools/bin/onie-fwpkg show-pending
    fi

    return ${num}
}

system_reboot() {
    echo "INFO: Rebooting in 5 sec..."

    # Give user some time to cancel the update
    sleep 5s

    # Use SONiC reboot scenario
    /usr/local/bin/reboot
    exit $?
}

terminate_handler() {
    local -r _rc="$?"
    local -r _sig="${1}"

    echo
    echo "WARNING: Interrupted by ${_sig}: disable ONIE firmware update mode"
    echo

    enable_onie_access
    disable_onie_fw_update_mode
    rc=$?
    disable_onie_access

    if [[ ${rc} -ne 0 ]]; then
        echo "ERROR: failed to disable ONIE firmware update mode"
        exit ${rc}
    fi

    exit ${_rc}
}

register_terminate_handler() {
    trap "terminate_handler SIGHUP" SIGHUP
    trap "terminate_handler SIGINT" SIGINT
    trap "terminate_handler SIGQUIT" SIGQUIT
    trap "terminate_handler SIGTERM" SIGTERM
}

unlock_handler() {
    /usr/bin/flock -u ${1}
}

register_unlock_handler() {
    trap "unlock_handler ${1}" EXIT
}

unlock_script_state_change() {
    /usr/bin/flock -u ${lock_fd}
}

lock_script_state_change(){
    exec {lock_fd}>${lock_file}
    /usr/bin/flock -x ${lock_fd}
    register_unlock_handler ${lock_fd}
}

# Multiprocessing synchronization
lock_script_state_change

# Process command arguments
cmd="${1}"

# Optional argument
arg="${2}"

if [[ -z "${cmd}" ]]; then
    # Default to 'show' if no command is specified.
    cmd="show"
fi

case "${cmd}" in
    add|remove)
        if [[ -z "${arg}" ]]; then
            echo "ERROR: This command requires a firmware update file name"
            echo "Run: '${this_script} help' for complete details"
            exit 1
        fi
        ;;
    update)
        enable_onie_access
        show_pending
        rc=$?
        if [[ ${rc} -ne 0 ]]; then
            enable_onie_fw_update_mode
            rc=$?
            disable_onie_access
            if [[ ${rc} -eq 0 ]]; then
                if [[ "${arg}" == "--no-reboot" ]]; then
                    echo "INFO: ONIE firmware update successfully STAGED for install at NEXT reboot. Please reboot manually to complete installation."
                    exit 0
                else
                    system_reboot
                fi
            else
                echo "ERROR: failed to enable ONIE firmware update mode"
                exit ${rc}
            fi
        else
            echo "ERROR: No firmware images for update"
            echo "Run: '${this_script} add <image>' before update"
            disable_onie_access
            exit 1
        fi
        ;;
    purge|show-pending|show-results|show|show-log|help)
        ;;
    *)
        echo "ERROR: Unknown command: ${cmd}"
        exit 1
        ;;
esac

enable_onie_access
${onie_mount}/onie/tools/bin/onie-fwpkg "$@"
rc=$?
if [[ "${cmd}" = "help" ]]; then
    print_help
fi
disable_onie_access

exit ${rc}
