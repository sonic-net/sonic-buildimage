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

command_name="sonic-bfb-installer.sh"
usage(){
    echo "Syntax: $command_name -b|--bfb <BFB_Image_Path> --rshim|-r <rshim1,..rshimN> --verbose|-v --config|-c <Options> --help|h"
    echo "Arguments:"
    echo "-b			Provide custom path for bfb image"
    echo "-r			Install only on DPUs connected to rshim interfaces provided, mention all if installation is requried on all connected DPUs"
    echo "-v			Verbose installation result output"
    echo "-c			Config file"
    echo "-h		        Help"
}
WORK_DIR=`mktemp -d -p "$DIR"`

bfb_install_call(){
    #Example:sudo bfb-install -b <full path to image> -r rshim<id>
    local result_file=$(mktemp "${WORK_DIR}/result_file.XXXXX")
    local cmd="timeout 600s bfb-install -b $2 -r $1 $appendix"
    echo "Installing bfb image on DPU connected to $1 using $cmd"
    local indicator="$1:"
    eval "$cmd" > "$result_file" 2>&1 > >(while IFS= read -r line; do echo "$indicator $line"; done > "$result_file")
    local exit_status=$?
    if [ $exit_status  -ne 0 ]; then
        echo "$1: Error: Installation failed on connected DPU!"
    else
        echo "$1: Installation Successful"
    fi
    if [ $exit_status -ne 0 ] ||[ $verbose = true ]; then
        cat "$result_file"
    fi
}

file_cleanup(){
    rm -rf "$WORK_DIR"
}

is_url() {
    local link=$1
    if [[ $link =~ https?:// ]]; then 
        echo "Detected URL. Downloading file"
        filename="${WORK_DIR}/sonic-nvidia-bluefield.bfb"
        curl -L -o "$filename" "$link"
        res=$?
        if test "$res" != "0"; then
            echo "the curl command failed with: $res"
            exit 1
        fi
        bfb="$filename"
        echo "bfb path changed to $bfb"
    fi
}

validate_rshim(){
    local provided_list=("$@")
    for item1 in "${provided_list[@]}"; do
        local found=0
        for item2 in "${dev_names_det[@]}"; do
            if [[ "$item1" = "$item2" ]]; then
                found=1
                break
            fi
        done
        if [[ $found -eq 0 ]]; then
            echo "$item1 is not detected! Please provide proper rshim interface list!"
            exit 1
        fi
    done
}

check_for_root(){
    if [ "$EUID" -ne 0 ]
        then echo "Please run the script in sudo mode"
        exit
    fi
}

main(){
    check_for_root
    local config=
    while [ "$1" != "--" ]  && [ -n "$1" ]; do
        case $1 in
            --help|-h)
                usage;
                exit 0
            ;;
            --bfb|-b)
                shift;
                bfb=$1
            ;;
            --rshim|-r)
                shift;
                rshim_dev=$1
            ;;
            --config|-c)
                shift;
                config=$1
            ;;
            --verbose|-v)
                verbose=true
            ;;
        esac
        shift
    done
    if [ -z "$bfb" ]; then
        echo "Error : bfb image is not provided."
        usage
        exit 1
    else
        is_url $bfb
    fi
    trap "file_cleanup" EXIT
    if [[ -f ${config} ]]; then
        echo "Using ${config} file"
        appendix="-c ${config}"
    fi
    dev_names_det+=($(
        ls -d /dev/rshim? | awk -F'/' '{print $NF}'
    ))
    if [ "${#dev_names_det[@]}" -eq 0 ]; then
        echo "No rshim interfaces detected! Make sure to run the $command_name script from the host device/ switch!"
        exit 1
    fi
    if [ -z "$rshim_dev" ]; then
        echo "No rshim interfaces provided!"
        usage
        exit 1
    else
        if [ "$rshim_dev" = "all" ]; then
            dev_names=("${dev_names_det[@]}")
            echo "${#dev_names_det[@]} rshim interfaces detected:"
            echo "${dev_names_det[@]}"
        else
            IFS=',' read -ra dev_names <<< "$rshim_dev"
            validate_rshim ${dev_names[@]}
        fi
    fi
    trap 'kill_ch_procs' SIGINT SIGTERM SIGHUP
    for i in "${dev_names[@]}"
    do
        :
        bfb_install_call $i $bfb &
    done
    wait
}

kill_all_descendant_procs() {
    local pid="$1"
    local self_kill="${2:-false}"
    if children="$(pgrep -P "$pid")"; then
        for child in $children; do
            kill_all_descendant_procs "$child" true
        done
    fi
    if [[ "$self_kill" == true ]]; then
        kill -9 "$pid" > /dev/null 2>&1
    fi
}


kill_ch_procs(){
    echo ""
    echo "Installation Interrupted.. killing All child procs"
    kill_all_descendant_procs $$
}
appendix=
verbose=false
main "$@"
