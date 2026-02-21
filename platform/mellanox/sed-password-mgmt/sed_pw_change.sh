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

# This script will store new SSD password in TPM and in SED.

# Algorithm to store in TPM and SED
# There are 2 Banks where to store new password.
# 0. Read TPM Banks 1&2
# 1. The code will first validate Banks.
# 2. The code will store the new password in the bank that succeeded first in decrypting the SED
# 3. The code will store the new password in SED.
# 4. The code will store the secondary Bank with the new password.

sed_pw_bank_1=
sed_pw_bank_2=
tpm_reg=0x81010001
tpm_reg_2=0x81010002
disk_name=
tpm_reg_next=

source /usr/local/bin/sed_pw_util.sh

usage() {
    echo "This script is used for saving a new SSD password in the TPM and SED"
    _msg_usg="Usage:  $0 -p <new_sed_password>"
    echo $_msg_usg
    log_info "$_msg_usg"
    exit 1
}

while getopts "p:" opt; do
    case $opt in
        p)
            SED_NEW_PW=$OPTARG
            ;;
        *)
            usage
            ;;
    esac
done

if [ -z $SED_NEW_PW ]; then
    _msg="New SED password does not exist, please call this script with Parameter: -p <password>"
    log_info "$_msg"
    echo $_msg
    exit 1
fi

log_info "SED new password start."

find_disk_name
if [ $? -ne 0 ]; then
    log_warn "Block device cannot be determined"
    exit 1
fi

if ! check_sed_ready; then
    log_warn "SED is not ready for operations"
    exit 1
fi

# 0. Read TPM Banks 1&2
read_sed_pwd

# 1. The code will first validate Banks.
validate_sed_pw $sed_pw_bank_1
res_val_1=$?
validate_sed_pw $sed_pw_bank_2
res_val_2=$?

log_debug "Old Password Validation - Bank 1: $res_val_1, Bank 2: $res_val_2"

# 2. The code will store the new password in the TPM Bank.
# In case one of the Bank have wrong data it is first storing in it.
if [ $res_val_1 -eq 0 ] && [ $res_val_2 -eq 0 ]; then
    old_good_pw=$sed_pw_bank_1
    curr_tpm_reg=$tpm_reg
    FLAG_NEXT_BANK_TO_STORE='B'
elif [ $res_val_1 -ne 0 ] && [ $res_val_2 -eq 0 ]; then
    old_good_pw=$sed_pw_bank_2
    curr_tpm_reg=$tpm_reg
    FLAG_NEXT_BANK_TO_STORE='B'
elif [ $res_val_1 -eq 0 ] && [ $res_val_2 -ne 0 ]; then
    old_good_pw=$sed_pw_bank_1
    curr_tpm_reg=$tpm_reg_2
    FLAG_NEXT_BANK_TO_STORE='A'
else
    log_error "Validation of old password in both SED Banks failed."
    exit 1
fi

store_sed_pwd_in_tpm $curr_tpm_reg $SED_NEW_PW
res_store_sed=$?

if [ $res_store_sed -eq 0 ]; then
    # 3. The code will store the new password in SED.
    log_debug "sedutil-cli --setadmin1pwd <old_good_pw> <SED_NEW_PW> ${disk_name}"
    sedutil-cli --setadmin1pwd $old_good_pw $SED_NEW_PW ${disk_name}
    if [ $? -ne 0 ]; then
        log_error "sedutil-cli --setadmin1pwd ** ** ${disk_name} failed"
        exit 1
    fi
else
    log_error "Failed when storing new password in TPM"
    exit 1
fi

if [ "$FLAG_NEXT_BANK_TO_STORE" = "A" ]; then
    log_debug "Storing Bank A"
    tpm_reg_next=$tpm_reg
elif [ "$FLAG_NEXT_BANK_TO_STORE" = "B" ]; then
    log_debug "Storing Bank B"
    tpm_reg_next=$tpm_reg_2
else
    log_error "No secondary Bank was updated."
    exit 1
fi

# 4. The code will store the secondary Bank with the new password.
store_sed_pwd_in_tpm $tpm_reg_next $SED_NEW_PW
if [ $? -ne 0 ]; then
    log_error "Store new password in the secondary Bank failed."
    exit 1
fi
log_info "SED new password done"
