#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

echo "SDK $SDK_VERSION" > temp_versions_file
echo "FW $BF3_FW_VERSION" >> temp_versions_file
echo "SAI $DPU_SAI_VERSION" >> temp_versions_file

SAI_API_VERSION_PATH="/sonic/src/sonic-sairedis/SAI/inc/saiversion.h"
if [ -f "$SAI_API_VERSION_PATH" ]; then
    SAI_MAJOR=$(grep "SAI_MAJOR" "$SAI_API_VERSION_PATH" | grep -oE "[0-9]+")
    SAI_MINOR=$(grep "SAI_MINOR" "$SAI_API_VERSION_PATH" | grep -oE "[0-9]+")
    SAI_REVISION=$(grep "SAI_REVISION" "$SAI_API_VERSION_PATH" | grep -oE "[0-9]+")
    if [ -n "$SAI_MAJOR" ] && [ -n "$SAI_MINOR" ] && [ -n "$SAI_REVISION" ]; then
        echo "SAI_API_HEADERS $SAI_MAJOR.$SAI_MINOR.$SAI_REVISION" >> temp_versions_file
    else
        echo "SAI_API_HEADERS N/A" >> temp_versions_file
    fi
else
    echo "SAI_API_HEADERS N/A" >> temp_versions_file
fi

echo "BFSOC $BFSOC_VERSION-$BFSOC_REVISION" >> temp_versions_file
echo "MFT $MFT_VERSION-$MFT_REVISION" >> temp_versions_file
echo "KERNEL $KVERSION_SHORT" >> temp_versions_file