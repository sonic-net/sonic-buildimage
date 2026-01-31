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

# This script will verify the SED password stored in the TPM banks 1&2, and recover the wrong bank.

disk_name=
tpm_reg=0x81010001
tpm_reg_2=0x81010002
sed_pw_bank_1=
sed_pw_bank_2=

export PATH=/run:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

source /usr/local/bin/sed_pw_util.sh

# Function is checking the password from TPM banks 1&2 according to the recovery algorithm
# Both TPM Banks should have same password and passed SED validation.
recover_bank_logic() {
    validate_sed_pw $sed_pw_bank_1
    rc_sed_1=$?
    if [ $sed_pw_bank_1 = $sed_pw_bank_2 ] && [ $rc_sed_1 -eq 0 ]; then
        log_info "SED TPM Banks are aligned and passed authentication"
        exit 0
    fi
    log_info "Validation SED TPM Bank 1 res: $rc_sed_1"

    validate_sed_pw $sed_pw_bank_2
    rc_sed_2=$?
    log_debug "Validation SED TPM Bank 2 res: $rc_sed_2"

    if [ "$rc_sed_1" -ne 0 ] && [ "$rc_sed_2" -ne 0 ]; then
        log_error "Both TPM Banks 1&2 not passing authentication"
        exit 2
    elif [ $rc_sed_1 -ne 0 ] && [ $rc_sed_2 -eq 0 ]; then
        log_warn "Authentication Bank 2 passed and Bank 1 failed."
        log_info "Storing PW from Bank 2 to Bank 1"
        store_sed_pwd_in_tpm $tpm_reg $sed_pw_bank_2
        log_info "Stored PW from Bank 2 to Bank 1 succeed"
    elif [ "$rc_sed_1" -eq 0 ] && [ "$rc_sed_2" -ne 0 ]; then
        log_warn "Authentication Bank 1 passed and Bank 2 failed."
        log_info "Storing PW from Bank 1 to Bank 2"
        store_sed_pwd_in_tpm $tpm_reg_2 $sed_pw_bank_1
        log_info "Stored PW from Bank 1 to Bank 2 succeed"
    fi
}

log_info "SED password TPM Banks validation start."

find_disk_name
if [ $? -ne 0 ]; then
    log_warn "Block device cannot be determined"
    exit 1
fi

if ! check_sed_ready; then
    log_warn "SED is not ready for operations"
    exit 1
fi

# Read SED TPM Banks 1&2
read_sed_pwd

if [ -z $sed_pw_bank_1 ] && [ -z $sed_pw_bank_2 ]; then
    log_error "SED TPM Banks 1&2 are empty"
    exit 1
fi

recover_bank_logic

log_info "SED password Banks validation done."

exit 0
