#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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


PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}
PLATFORM_JSON=/usr/share/sonic/device/$PLATFORM/platform.json

usage(){
    echo "Usage: $0 {dpu2pcie|dpu2rshim|rshim2dpu|pcie2dpu} name"
}
declare -A dpu2pcie

validate_platform(){
    if [[ ! -f $PLATFORM_JSON ]]; then
        echo "platform.json file not found. Exiting script"
        exit 1
    fi
}


validate_platform
case $1 in
    "dpu2rshim")
	jq_query='.DPUS[$dpu].rshim_info'
	var="dpu"
	;;
    "dpu2pcie")
	jq_query='.DPUS[$dpu].bus_info'
	var="dpu"
	;;
    "pcie2dpu")
	jq_query='.DPUS | to_entries[] | select(.value.bus_info == $bus) | .key'
	var="bus"
	;;
    "pcie2rshim")
	jq_query='.DPUS | to_entries[] | select(.value.bus_info == $bus) | .value.rshim_info'
	var="bus"
	;;
    "rshim2dpu")
	jq_query='.DPUS | to_entries[] | select(.value.rshim_info == $rshim) | .key'
	var="rshim"
	;;
    "rshim2pcie")
	jq_query='.DPUS | to_entries[] | select(.value.rshim_info == $rshim) | .value.bus_info'
	var="rshim"
	;;
    *)
        echo "Invalid usage of script!"
        usage
        exit 1
esac

IFS=',' read -r -a identifier_array <<< "$2"
for identifier in "${identifier_array[@]}"; do
	op=$(jq -r --arg "$var" "$identifier" "$jq_query" "$PLATFORM_JSON")
	if [[ "$op" != "null" ]]; then
		echo "$op"
	else
		echo "Invald entry! $identifier"
		exit 1
	fi
done

