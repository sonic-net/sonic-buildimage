#!/usr/bin/env bash
#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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

#
# Script for sai failure dump
#

# Source the platform specific dump file

sai_dump_name="sai_sdk_dump_$(date +"%m_%d_%Y_%I_%M_%p")"
sai_dump_path="${DUMPDIR}/$sai_dump_name"
mkdir -p $sai_dump_path
sai_dump_file="${sai_dump_path}/$sai_dump_name"
saisdkdump -f $sai_dump_file
cd "${DUMPDIR}"
tar -cvf "$sai_dump_name".tar $sai_dump_name
gzip "$sai_dump_name".tar
rm -rf $sai_dump_name

# Update max failure dumps
if grep -q SAI_DUMP_STORE_AMOUNT /usr/share/sonic/hwsku/sai.profile; then
    SAI_MAX_FAILURE_DUMPS=$(grep SAI_DUMP_STORE_AMOUNT /usr/share/sonic/hwsku/sai.profile | cut -d '=' -f2)
fi

