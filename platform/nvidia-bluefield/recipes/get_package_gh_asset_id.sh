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

tag=$1
package=$2
gh_token=$3

repo=nvidia-sonic/sonic-bluefield-packages
assetsfile='/tmp/dpu-sdk-fw-assets-'$tag
lockfile='/tmp/dpu-sdk-fw-assets-lock-'$tag

if [ ! -f $assetsfile ]; then
  cmd="/usr/bin/curl -s -L -f -H 'Authorization: token $gh_token' 'https://api.github.com/repos/$repo/releases/tags/dpu-$tag' | jq -r '.assets[] | (.name) + \" \" + (.id | tostring)'"
  eval "flock -w 5 $lockfile $cmd > $assetsfile"
fi

echo $(grep $package $assetsfile | awk '{print $2}')
