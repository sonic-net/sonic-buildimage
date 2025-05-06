#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
declare -A rshim2dpu

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
    local appendix=$4
    local -r rshim=$1
    local dpu=$2
    local bfb=$3
    local result_file=$(mktemp "${WORK_DIR}/result_file.XXXXX")
    if [ -z "$appendix" ]; then
        local cmd="timeout 600s bfb-install -b $bfb -r $1"
    else
        local cmd="timeout 600s bfb-install -b $bfb -r $1 -c $appendix"
    fi
    echo "Installing bfb image on DPU connected to $rshim using $cmd"
    local indicator="$rshim:"
    trap 'kill_ch_procs' SIGINT SIGTERM SIGHUP
    eval "$cmd"  > >(while IFS= read -r line; do echo "$indicator $line"; done >> "$result_file") 2>&1 &
    cmd_pid=$!
    local total_time=600
    local elapsed=0
    # Interval is selected at random so all the processes can print to same line
    local interval=$(($RANDOM%(10-3+1)+3))
    while kill -0 $cmd_pid 2>/dev/null; do
        sleep $interval
        elapsed=$((elapsed + interval))
        echo -ne "\r$indicator Installing... $elapsed/$total_time seconds elapsed"
        if [ $elapsed -ge $total_time ]; then
            break
        fi
    done
    wait $cmd_pid
    local exit_status=$?
    if [ $exit_status  -ne 0 ]; then
        echo "$rshim: Error: Installation failed on connected DPU!"
    else
        echo "$rshim: Installation Successful"
    fi
    if [ $exit_status -ne 0 ] ||[ $verbose = true ]; then
        cat "$result_file"
    fi
    echo "$rshim: Resetting DPU $dpu"
    cmd="dpuctl dpu-reset --force $dpu"
    if [[ $verbose == true ]]; then
        cmd="$cmd -v"
    fi
    eval $cmd
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

get_mapping(){
    local provided_list=("$@")

    for item1 in "${provided_list[@]}"; do
        var=$(dpumap.sh rshim2dpu $item1)
        if [ $? -ne 0 ]; then
            echo "$item1 does not have a valid dpu mapping!"
            exit 1
        fi
        rshim2dpu["$item1"]="$var"
    done
}

validate_dpus(){
    local provided_list=("$@")
    for item1 in "${provided_list[@]}"; do
        var=$(dpumap.sh dpu2rshim $item1)
        if [ $? -ne 0 ]; then
            echo "$item1 does not have a valid rshim mapping!"
            exit 1
        fi
        rshim2dpu["$var"]="$item1"
        dev_names+=("$var")
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
            --dpu|-d)
                shift;
                dpus=$1
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
    dev_names_det+=($(
        ls -d /dev/rshim? | awk -F'/' '{print $NF}'
    ))
    if [ "${#dev_names_det[@]}" -eq 0 ]; then
        echo "No rshim interfaces detected! Make sure to run the $command_name script from the host device/ switch!"
        exit 1
    fi
    if [ -z "$rshim_dev" ]; then
        if [ -z "$dpus" ]; then
        	echo "No rshim interfaces provided!"
        	usage
        	exit 1
       fi
       if [ "$dpus" = "all" ]; then
            rshim_dev="$dpus" 
       else
            IFS=',' read -ra dpu_names <<< "$dpus"
            validate_dpus ${dpu_names[@]}
       fi
    fi


    if [ "$rshim_dev" = "all" ]; then
            dev_names=("${dev_names_det[@]}")
            echo "${#dev_names_det[@]} rshim interfaces detected:"
            echo "${dev_names_det[@]}"
        else
            if [ ${#dev_names[@]} -eq 0 ]; then
                # If the list is not empty, the list is obtained from the DPUs
                IFS=',' read -ra dev_names <<< "$rshim_dev"
            fi
            validate_rshim ${dev_names[@]}
    fi
    if [ ${#rshim2dpu[@]} -eq 0 ]; then
        get_mapping ${dev_names[@]}
    fi
    # Sort list of rshim interfaces so that config is applied in a known order
    sorted_devs=($(for i in "${dev_names[@]}"; do echo $i; done | sort))
    if [ ! -z ${config} ]; then
        echo "Using ${config} file/s"
        if [[ "$config" == *","* ]]; then
            IFS=',' read -r -a arr <<< "$config"
        else
            arr=()
            for ((i=0; i<${#dev_names[@]}; i++)); do
                arr+=("$config")
            done
        fi
        if [ ${#arr[@]} -ne ${#sorted_devs[@]} ]; then
            echo "Length of config file list does not match the devices selected: ${sorted_devs[@]} and ${arr[@]}"
            exit 1
        fi
        for i in "${!arr[@]}"
        do
            if [ ! -f ${arr[$i]} ]; then
                echo "Config provided ${arr[$i]} is not a file! Please check"
                exit 1
            fi
        done
    fi
    trap 'kill_ch_procs' SIGINT SIGTERM SIGHUP
    for i in "${!sorted_devs[@]}"
    do
        :
        rshim_name=${sorted_devs[$i]}
        dpu_name=${rshim2dpu[$rshim_name]}
        bfb_install_call ${rshim_name} ${dpu_name} $bfb ${arr[$i]} &
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

