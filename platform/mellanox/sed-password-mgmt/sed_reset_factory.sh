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

# This script will reset the SED password to the default one

source /usr/local/bin/sed_pw_util.sh

find_disk_name
res_find_disk=$?
if [ $res_find_disk != 0 ]; then
    log_warn "Block device cannot be determined"
    exit 1
fi

if ! check_sed_ready; then
    log_warn "SED is not ready for operations"
    exit 1
fi

read_default_sed_pwd
if [ $? -ne 0 ] || [ -z "$sed_pw_default" ]; then
    log_error "Failed to retrieve default SED password from TPM"
    exit 1
fi

validate_sed_pw $sed_pw_default
res_val_default=$?
if [ $res_val_default = 0 ]; then
    log_info "SED default password is the existing one"
    exit 0
else
    log_info "Resetting SED password to the default"
    /usr/local/bin/sed_pw_change.sh -p $sed_pw_default
    if [ $? -ne 0 ]; then
        log_error "SED password change failed"
        exit 1
    else
        log_info "SED password change succeed"
        exit 0
    fi
fi
