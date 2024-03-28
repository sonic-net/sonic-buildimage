#!/bin/bash
#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

sdk_ver=$1
path=$2
gh_token=$3

repo=nvidia-sonic/sonic-bluefield-packages

asset_id=$($(dirname "$0")/get_package_gh_asset_id.sh $sdk_ver VERSIONS_FOR_SONIC_BUILD $gh_token)

cmd="/usr/bin/curl -s -L -f -H 'Accept: application/octet-stream' -H 'Authorization: token $gh_token' --output $path https://api.github.com/repos/$repo/releases/assets/$asset_id"
eval "$cmd"
