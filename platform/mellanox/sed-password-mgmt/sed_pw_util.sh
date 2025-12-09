#!/bin/bash

#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

# This script is an utility for supporting SED password change.

sed_pw_bank_1=
sed_pw_bank_2=
sed_pw_default=
tpm_reg=0x81010001
tpm_reg_2=0x81010002
tpm_reg_3=0x81010003
disk_name=
filename="$(basename "$0")"

TPM_SED_AUTH_GUID="36bfcbde-d710-4903-ba2e-c03ec245dcee"
TPM_SED_AUTH_NAME="TpmSealCtx"
tpm_sed_auth=""

log_error() {
    logger -t "$filename" -p user.err "$1"
}

log_info() {
    logger -t "$filename" -p user.info "$1"
}

log_warn() {
    logger -t "$filename" -p user.warning "$1"
}

log_debug() {
    logger -t "$filename" -p user.debug "$1"
}

get_tpm_sed_auth() {
    if [ -n "$tpm_sed_auth" ]; then
        log_info "TPM SED auth already retrieved"
        return 0
    fi
    log_info "Trying to read TPM SED auth from UEFI variable"
    efi_file="/sys/firmware/efi/efivars/${TPM_SED_AUTH_NAME}-${TPM_SED_AUTH_GUID}"
    if [ -f "$efi_file" ]; then
        auth_enc=$(dd if="$efi_file" bs=1 skip=4 2>/dev/null)
        tpm_sed_auth=$(echo "$auth_enc" | base64 -d)
        log_info "TPM SED auth retrieved from UEFI variable"
        return 0
    fi
    log_info "TPM SED auth UEFI variable not found (may not be provisioned with auth)"
}

find_disk_name() {
    non_removable_mountables_disks=""
    non_removable_mountables_disks_count=0
    mountables_partitions=$(cat /proc/partitions | grep -v "^major" | grep -v -E 'ram|loop' | awk '{{print $4}}')
    for block_device_name in ${mountables_partitions}; do
        block_device_link=$(find /sys/bus /sys/class /sys/block/ -name "${block_device_name}")
        for first_block_device_link in $block_device_link; do
            if [ -e "${first_block_device_link}"/removable ]; then
                if [ "0" = $(cat "${first_block_device_link}"/removable) ]; then
                    non_removable_mountables_disks_count=$((non_removable_mountables_disks_count+1))
                    if [ "1" = "${non_removable_mountables_disks_count}" ]; then
                        non_removable_mountables_disks="${block_device_name}"
                    else
                        non_removable_mountables_disks="${non_removable_mountables_disks} ${block_device_name}"
                    fi
                fi
            fi
            break
        done
    done
    if [ "1" = "${non_removable_mountables_disks_count}" ]; then
        disk_name="/dev/${non_removable_mountables_disks}"
        log_debug "disk_name: $disk_name"
        return 0
    else
        log_warn "find disk name: $disk_name failed."
        return 1
    fi
}

store_sed_pwd_in_tpm() {
    log_info "Storing SED password in TPM"
    if [ -n "$1" ] && [ -n "$2" ]; then
        log_debug "Two arguments provided: TPM-REG: ** and SED-PW: **"
    else
        log_error "Two arguments are required! TPM-REG & SED-PW"
        exit 2
    fi
    _tpm_reg=$1
    _sed_pwd=$2
    get_tpm_sed_auth
    tpm2_evictcontrol -C o -c "$_tpm_reg" > /dev/null 2>&1
    tpm2_createprimary -C o --key-algorithm=rsa --key-context=prim.ctx > /dev/null 2>&1
    if [ -z "$tpm_sed_auth" ]; then
        log_debug "Storing without TPM auth"
        echo "$_sed_pwd" | tpm2_create -g sha256 -u seal.pub -r seal.priv -C prim.ctx -i - > /dev/null 2>&1
    else
        log_debug "Storing with TPM auth" 
        echo "$_sed_pwd" | tpm2_create -g sha256 -u seal.pub -r seal.priv -C prim.ctx -p "$tpm_sed_auth" -i - > /dev/null 2>&1
    fi
    tpm2_load -C prim.ctx -u seal.pub -r seal.priv -n seal.name -c seal.ctx
    tpm2_evictcontrol -C o -c seal.ctx "$_tpm_reg"
    rc=$?
    rm -f seal.* prim.ctx
    if [ $rc -ne 0 ]; then
        log_error "Failed access TPM."
        exit 1
    fi
    log_info "Storing SED PW in TPM - Done"
}

read_sed_pwd() {
    get_tpm_sed_auth
    if [ -z "$tpm_sed_auth" ]; then
        log_info "TPM SED auth not available"
        log_debug "tpm2_unseal -c <tpm_reg>"
        sed_pw_bank_1=$(tpm2_unseal -c $tpm_reg)
        if [ $? -ne 0 ]; then
            log_error "Failed when reading from Bank 1."
        else
            log_debug "Read from Bank 1 succeed."
        fi
        log_debug "tpm2_unseal -c <tpm_reg_2>"
        sed_pw_bank_2=$(tpm2_unseal -c $tpm_reg_2)
        if [ $? -ne 0 ]; then
            log_error "Failed when reading from Bank 2."
        else
            log_debug "Read from Bank 2 succeed."
        fi
    else
        log_info "TPM SED auth available"
        log_debug "tpm2_unseal -c <tpm_reg> -p <auth>"
        sed_pw_bank_1=$(tpm2_unseal -c $tpm_reg -p "$tpm_sed_auth")
        if [ $? -ne 0 ]; then
            log_error "Failed when reading from Bank 1."
        else
            log_debug "Read from Bank 1 succeed."
        fi
        log_debug "tpm2_unseal -c <tpm_reg_2> -p <auth>"
        sed_pw_bank_2=$(tpm2_unseal -c $tpm_reg_2 -p "$tpm_sed_auth")
        if [ $? -ne 0 ]; then
            log_error "Failed when reading from Bank 2."
        else
            log_debug "Read from Bank 2 succeed."
        fi
    fi
}

read_default_sed_pwd() {
    log_debug "Retrieving default SED password from TPM bank $tpm_reg_3"
    get_tpm_sed_auth
    if [ -z "$tpm_sed_auth" ]; then
        log_debug "tpm2_unseal -c $tpm_reg_3"
        sed_pw_default=$(tpm2_unseal -c $tpm_reg_3)
        if [ $? -ne 0 ]; then
            log_error "Failed when reading default password from Bank 3."
        else
            log_debug "Read default password from Bank 3 succeed."
        fi
    else
        log_debug "tpm2_unseal -c $tpm_reg_3 -p <auth>"
        sed_pw_default=$(tpm2_unseal -c $tpm_reg_3 -p "$tpm_sed_auth")
        if [ $? -ne 0 ]; then
            log_error "Failed when reading default password from Bank 3."
        else
            log_debug "Read default password from Bank 3 succeed."
        fi
    fi
}

validate_sed_pw() {
    if [ -z $1 ]; then
        log_error "SED password param to validate is empty"
        return 1
    fi
    log_debug "sedutil-cli --listLockingRanges <SED-PW> "${disk_name}""
    sedutil-cli --listLockingRanges $1 "${disk_name}"
    rc_sed=$?
    if [ $rc_sed -eq 0 ]; then
        log_debug "SED listLockingRanges passed"
        return 0
    else
        log_warn "SED listLockingRanges failed"
        return 1
    fi
}

check_sed_support() {
    checked_disk=$disk_name
    log_debug "check_sed_support: $checked_disk"
    # sedutil reports e.g. nvme0 and not nvme0n1 
    if echo "${disk_name}" | grep -q nvme; then
        checked_disk=$(echo "${disk_name}" | cut -d / -f 3 | cut -b-5)
    fi
    log_debug "checked_disk: $checked_disk"
    sed_ind=$(sedutil-cli --scan | grep "${checked_disk}" | awk '{print $2}')
    if [ "$sed_ind" = "2" ]; then
        return 0
    else
        return 1
    fi
}

check_sed_locking_enabled() {
    log_debug "Checking if SED locking is enabled on $disk_name"
    output=$(sedutil-cli --query $disk_name)
    if [ $? = 0 ]; then
        lockEnReg=$(echo "$output" | grep "LockingEnabled")
        lockingEnabled=$(echo $lockEnReg | awk -F', ' '{for(i=1;i<=NF;i++) if($i ~ /^LockingEnabled =/) print $i}' | awk -F' = ' '{print $2}')
        log_debug "Found LockingEnabled line: $lockEnReg"
        log_debug "lockingEnabled==$lockingEnabled"
        if [ "$lockingEnabled" = "Y" ]; then
            log_debug "SED LockingEnabled attribute is enabled"
            return 0
        else
            log_warn "SED LockingEnabled attribute not enabled"
            return 1
        fi
    else
        log_warn "sedutil-cli --query $disk_name failed"
        return 1
    fi
}

check_tpm_configured() {
    log_debug "Checking if TPM is configured with required banks"
    persistent_handles=$(tpm2_getcap handles-persistent 2>/dev/null)
    if [ $? -ne 0 ]; then
        log_warn "Failed to query TPM persistent handles"
        return 1
    fi
    log_debug "TPM persistent handles: $persistent_handles"
    if ! echo "$persistent_handles" | grep -q "$tpm_reg"; then
        log_warn "TPM bank $tpm_reg not configured"
        return 1
    fi
    if ! echo "$persistent_handles" | grep -q "$tpm_reg_2"; then
        log_warn "TPM bank $tpm_reg_2 not configured"
        return 1
    fi
    if ! echo "$persistent_handles" | grep -q "$tpm_reg_3"; then
        log_warn "TPM bank $tpm_reg_3 not configured"
        return 1
    fi
    log_debug "All required TPM SED password banks are configured"
    return 0
}

check_sed_ready() {
    log_debug "Check SED is ready for operations"
    # Check if TPM is configured with required banks
    if ! check_tpm_configured; then
        log_warn "TPM is not configured with required banks"
        return 1
    fi
    # Check opal compliant disk
    if ! check_sed_support; then
        log_warn "SED is not supported on disk $disk_name"
        return 1
    fi
    # Check if LockingEnabled=Y
    if ! check_sed_locking_enabled; then
        log_warn "SED locking is not enabled on disk $disk_name"
        return 1
    fi
    log_info "SED is ready for operations on disk $disk_name"
    return 0
}
