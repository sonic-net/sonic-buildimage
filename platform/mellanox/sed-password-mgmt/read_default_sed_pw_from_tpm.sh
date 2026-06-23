#!/bin/bash

#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# Mellanox: read default SED password from TPM bank 3 (0x81010003).
# Outputs the password to stdout on success; nothing on failure.

TPM_BANK_DEFAULT="0x81010003"
tpm_sed_auth=""

source /usr/local/bin/sed_pw_utils.sh

get_tpm_sed_auth

if [ -z "$tpm_sed_auth" ]; then
    out=$(tpm2_unseal -c "$TPM_BANK_DEFAULT" 2>/dev/null)
else
    out=$(tpm2_unseal -c "$TPM_BANK_DEFAULT" -p "$tpm_sed_auth" 2>/dev/null)
fi
if [ -n "$out" ]; then
    printf '%s' "$out"
fi
