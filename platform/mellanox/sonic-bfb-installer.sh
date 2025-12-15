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


# Lock file to prevent multiple instances
LOCK_FILE="/var/lock/sonic-bfb-installer.lock"
LOCK_FD=200

# Function to log messages with priority
log() {
    local priority=$1
    local message=$2

    # Always log to syslog
    logger -p "$priority" -t "$(basename "$0")" "$message"

    echo "$message"
}

# Helper functions for different log levels
log_error() {
    log "error" "$1"
}

log_warning() {
    log "warning" "$1" 
}

log_info() {
    log "info" "$1"
}

log_debug() {
    log "debug" "$1"
}

# Function to acquire lock
acquire_lock() {
    eval "exec $LOCK_FD>'$LOCK_FILE'"
    if ! flock -n $LOCK_FD; then
        log_error "Another instance of $(basename "$0") is already running"
        exit 1
    fi
    echo $$ > "$LOCK_FILE"
}

# Function to release lock
release_lock() {
    flock -u $LOCK_FD
    eval "exec $LOCK_FD>&-"
    rm -f "$LOCK_FILE"
}

# Acquire lock at script start
acquire_lock
trap 'release_lock' EXIT

[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment
PLATFORM=${PLATFORM:-`sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform`}
PLATFORM_JSON=/usr/share/sonic/device/$PLATFORM/platform.json

declare -A rshim2dpu
declare -r bfsoc_dev_id="15b3:c2d5"
declare -r cx7_dev_id="15b3:a2dc"

EXTRACTED_BFB_PATH=""
EXTRACTED_CHECKSUM_PATH=""

# Local functions to replace dpumap.sh
rshim2dpu_map() {
    local rshim=$1
    # Extract DPU name from platform.json for the given rshim
    local dpu=$(jq -r --arg rshim "$rshim" '.DPUS | to_entries[] | select(.value.rshim_info == $rshim) | .key' "$PLATFORM_JSON")
    if [ -z "$dpu" ]; then
        return 1
    fi
    echo "$dpu"
    return 0
}

dpu2rshim_map() {
    local dpu=$1
    # Extract rshim bus info from platform.json for the given DPU
    local rshim=$(jq -r --arg dpu "$dpu" '.DPUS[$dpu].rshim_info // empty' "$PLATFORM_JSON")
    if [ -z "$rshim" ]; then
        return 1
    fi
    echo "$rshim"
    return 0
}

list_dpus_map() {
    # Extract DPU names from platform.json
    local dpus=$(jq -r '.DPUS | keys[]' "$PLATFORM_JSON")
    if [ -z "$dpus" ]; then
        return 1
    fi
    echo "$dpus"
    return 0
}

usage(){
    echo "Syntax: $(basename "$0") -b|--bfb <BFB_Image_Path> --rshim|-r <rshim1,..rshimN> --dpu|-d <dpu1,..dpuN> --verbose|-v --config|-c <Options> --help|-h"
    echo "Arguments:"
    echo "-b|--bfb		Provide custom path for bfb tar archive"
    echo "-r|--rshim		Install only on DPUs connected to rshim interfaces provided, mention all if installation is required on all connected DPUs"
    echo "-d|--dpu		Install on specified DPUs, mention all if installation is required on all connected DPUs"
    echo "-s|--skip-extract	Skip extracting the bfb image"
    echo "-v|--verbose		Verbose installation result output"
    echo "-c|--config		Config file"
    echo "-h|--help		Help"
}
WORK_DIR=`mktemp -d -p "$DIR"`

validate_platform(){
    if [[ ! -f $PLATFORM_JSON ]]; then
        log_error "platform.json file not found. Exiting script"
        exit 1
    fi
}

# Function to detect PCI device for a DPU and device ID
detect_pcie() {
    local dpu=$1
    local device_id=$2
    local bus_info rshim_bus_info

    # Get bus_info and rshim_bus_info for this DPU from platform.json
    bus_info=$(jq -r --arg dpu "$dpu" '.DPUS[$dpu].bus_info // empty' "$PLATFORM_JSON")
    rshim_bus_info=$(jq -r --arg dpu "$dpu" '.DPUS[$dpu].rshim_bus_info // empty' "$PLATFORM_JSON")

    # Check if bus_info exists and device matches device ID
    if [ -n "$bus_info" ] && lspci -D -n | grep "${bus_info}" | grep -q "${device_id}"; then
        echo "$bus_info"
        return 0
    fi

    # Check if rshim_bus_info exists and device matches device ID
    if [ -n "$rshim_bus_info" ] && lspci -D -n | grep "${rshim_bus_info}" | grep -q "${device_id}"; then
        echo "$rshim_bus_info"
        return 0
    fi

    return 1
}

# Function to detect the BlueField SoC PCI device for a DPU
detect_bfsoc_pcie() {
    local dpu=$1
    detect_pcie "$dpu" "${bfsoc_dev_id}"
}

detect_cx_pcie() {
    local dpu=$1
    detect_pcie "$dpu" "${cx7_dev_id}"
}

wait_for_rshim_boot() {
    local -r rshim=$1
    local timeout=10
    
    while [ ! -e "/dev/${rshim}/boot" ] && [ $timeout -gt 0 ]; do
        sleep 1
        ((timeout--))
    done

    if [ ! -e "/dev/${rshim}/boot" ]; then
        log_error "$rshim: Error: Boot file did not appear after 10 seconds"
        return 1
    fi
    return 0
}

remove_cx_pci_device() {
    local -r rshim=$1
    local -r dpu=$2
    
    # Get bus_id and rshim_bus_id for this DPU
    local bus_id=$(detect_cx_pcie $dpu)

    # Check if bus_id device exists
    if [ -n "$bus_id" ]; then
        if lspci -D | grep -q "$bus_id"; then
            log_info "$rshim: Removing PCI device $bus_id"
            echo 1 > /sys/bus/pci/devices/$bus_id/remove
        fi
    fi
}

monitor_installation() {
    local -r rid=$1
    local -r pid=$2
    local -r total_time=$3
    local elapsed=0
    
    # Random interval between 3-10 seconds for progress updates
    local interval=$(($RANDOM%(10-3+1)+3))
    
    while kill -0 $pid 2>/dev/null; do
        sleep $interval
        elapsed=$((elapsed + interval))
        # Use printf with \r for proper line handling
        printf "\r%s: Installing... %d/%d seconds elapsed" "$rid" "$elapsed" "$total_time"
        if [ $elapsed -ge $total_time ]; then
            break
        fi
    done
    # Add newline after progress
    printf "\n"
}

# Function to start rshim daemon
start_rshim_daemon() {
    local -r rid=$1
    local -r pci_bus=$2
    
    if ! start-stop-daemon --start --quiet --background \
        --make-pidfile --pidfile "/var/run/rshim_${rid}.pid" \
        --exec /usr/sbin/rshim -- -f -i "$rid" -d "pcie-$pci_bus"; then
        log_error "Failed to start rshim for rshim$rid"
        return 1
    fi
    return 0
}

# Function to stop rshim daemon
stop_rshim_daemon() {
    local -r rid=$1
    local -r pid_file="/var/run/rshim_${rid}.pid"
    
    # Only try to stop if pidfile exists
    if [ -f "$pid_file" ]; then
        start-stop-daemon --stop --quiet --pidfile "$pid_file" --remove-pidfile --retry TERM/15/KILL/5 2>/dev/null
    fi
    
    return 0
}

bfb_install_call() {
    local -r rshim=$1
    local -r dpu=$2 
    local -r bfb=$3
    local -r appendix=$4
    local -r rid=${rshim#rshim}
    local -r result_file=$(mktemp "${WORK_DIR}/result_file.XXXXX")
    local -r timeout_secs=1200
    
    # Get PCI bus info for the DPU
    local pci_bus=$(detect_bfsoc_pcie "$dpu")
    if [ -z "$pci_bus" ]; then
        log_error "Error: Could not find PCI bus for DPU $dpu"
        exit 1
    fi

    # Start rshim application
    if ! start_rshim_daemon "$rid" "$pci_bus"; then
        exit 1
    fi
    
    # Ensure rshim is stopped on exit
    trap "stop_rshim_daemon $rid" EXIT

    # Wait for boot file and remove PCI device
    if ! wait_for_rshim_boot "$rshim"; then
        stop_rshim_daemon "$rid"
        exit 1
    fi
    remove_cx_pci_device "$rshim" "$dpu"

    # Construct bfb-install command
    local cmd="timeout ${timeout_secs}s bfb-install -b $bfb -r $rshim"
    if [ -n "$appendix" ]; then
        cmd="$cmd -c $appendix"
    fi
    log_info "Installing bfb image on DPU connected to $rshim using $cmd"

    # Run installation with progress monitoring
    trap 'kill_ch_procs' SIGINT SIGTERM SIGHUP
    eval "$cmd" > >(while IFS= read -r line; do printf "%s: %s\n" "$rid" "$line"; done >> "$result_file") 2>&1 &
    local cmd_pid=$!

    monitor_installation "$rid" $cmd_pid $timeout_secs

    # Check installation result
    wait $cmd_pid
    local exit_status=$?
    if [ $exit_status -ne 0 ]; then
        log_error "$rid: Error: Installation failed on connected DPU!"
    else
        log_info "$rid: Installation Successful"
    fi

    # Show detailed output if verbose or error
    if [ $exit_status -ne 0 ] || [ $verbose = true ]; then
        cat "$result_file"
    fi

    # Stop rshim application and reset DPU
    stop_rshim_daemon "$rid"
    log_info "$rid: Resetting DPU $dpu"

    local reset_cmd="dpuctl dpu-reset --force $dpu"
    if [[ $verbose == true ]]; then
        reset_cmd="$reset_cmd -v"
    fi
    eval $reset_cmd
}

file_cleanup(){
    rm -rf "$WORK_DIR"
}

is_url() {
    local link=$1
    if [[ $link =~ https?:// ]]; then 
        log_debug "Detected URL. Downloading file"
        filename="${WORK_DIR}/sonic-nvidia-bluefield.bfb"
        curl -L -o "$filename" "$link"
        res=$?
        if test "$res" != "0"; then
            log_error "the curl command failed with: $res"
            exit 1
        fi
        bfb="$filename"
        log_debug "bfb path changed to $bfb"
    fi
}

extract_bfb() {
    local bfb_file=$1
    
    if [ ! -f "$bfb_file" ]; then
        log_error "BFB file not found: $bfb_file"
        exit 1
    fi
    
    local file_type=$(file -b "$bfb_file")
    if [[ $file_type == *"tar archive"* ]]; then
        log_info "Detected tar archive extracting BFB and SHA256 hash..."
        
        if ! tar -xf "$bfb_file" -C "${WORK_DIR}" 2>/dev/null; then
            log_error "Failed to extract tar archive: $bfb_file"
            exit 1
        fi
        
        local extracted_bfb=$(find "${WORK_DIR}" -maxdepth 1 -name "*bfb-intermediate"  | grep "$(basename "$bfb_file")" | head -n 1)
        if [ -z "$extracted_bfb" ]; then
            log_error "No BFB file found in tar archive"
            exit 1
        fi
        
        log_info "Extracted BFB file: $extracted_bfb"
        
        EXTRACTED_BFB_PATH="$extracted_bfb"
        
        chmod +x "$extracted_bfb"
        
        local extracted_sha256="${extracted_bfb}.sha256"
        if [ -f "$extracted_sha256" ]; then
            log_info "Found SHA256 hash file: $extracted_sha256"
        else
            log_warning "SHA256 hash file not found in tar archive"
        fi

        EXTRACTED_CHECKSUM_PATH="$extracted_sha256"
    else
        log_error "File is not a tar archive: $bfb_file! Please provide a tar archive with .bfb extension containing BFB and SHA256 hash."
        exit 1
    fi
}

validate_bfb_sha256() {
    if [ -f "$EXTRACTED_CHECKSUM_PATH" ]; then
        local expected_hash=$(cat "$EXTRACTED_CHECKSUM_PATH")

        log_info "Verifying SHA256 checksum..."
        local actual_hash=$(sha256sum "$EXTRACTED_BFB_PATH" | awk '{print $1}')
        
        if [ "$expected_hash" != "$actual_hash" ]; then
            log_error "SHA256 checksum mismatch!"
            log_error "Expected: $expected_hash"
            log_error "Actual:   $actual_hash"
            log_error "BFB file may be corrupted or tampered with."
            exit 1
        fi
        
        log_info "SHA256 checksum verification successful"
    else
        log_error "SHA256 hash file not found: $EXTRACTED_CHECKSUM_PATH"
        exit 1
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
            log_debug "$item1 is not detected! Please provide proper rshim interface list!"
            exit 1
        fi
    done
}

get_mapping(){
    local provided_list=("$@")

    for item1 in "${provided_list[@]}"; do
        var=$(rshim2dpu_map "$item1")
        if [ $? -ne 0 ]; then
            log_debug "$item1 does not have a valid dpu mapping!"
            exit 1
        fi
        rshim2dpu["$item1"]="$var"
    done
}

validate_dpus(){
    local provided_list=("$@")
    for item1 in "${provided_list[@]}"; do
        var=$(dpu2rshim_map "$item1")
        if [ $? -ne 0 ]; then
            log_debug "$item1 does not have a valid rshim mapping!"
            exit 1
        fi
        rshim2dpu["$var"]="$item1"
        dev_names+=("$var")
    done
}

check_for_root(){
    if [ "$EUID" -ne 0 ]
        then log_debug "Please run the script in sudo mode" 
        exit
    fi
}

detect_rshims_from_pci(){
    # Get list of supported DPUs from platform.json
    local dpu_list=$(list_dpus_map)
    if [ $? -ne 0 ] || [ -z "$dpu_list" ]; then
        log_debug "No supported DPUs found"
        return 1
    fi

    # For each DPU, check if its PCI exists and get corresponding rshim
    local detected_rshims=()
    while read -r dpu; do
        local pci=$(detect_bfsoc_pcie "$dpu")
        if [ $? -eq 0 ] && [ -n "$pci" ]; then
            detected_rshims+=($(dpu2rshim_map "$dpu"))
        fi
    done <<< "$dpu_list"

    if [ ${#detected_rshims[@]} -eq 0 ]; then
        log_debug "No rshim devices detected"
        return 1
    fi

    # Return unique sorted list of detected rshim devices
    printf '%s\n' "${detected_rshims[@]}" | sort -u
    return 0
}

main() {
    check_for_root
    validate_platform

    # Parse command line arguments
    local config= bfb= rshim_dev= dpus= skip_extract= verbose=false
    parse_arguments "$@"

    # Validate BFB image
    if [ -z "$bfb" ]; then
        log_debug "Error: bfb image is not provided."
        usage
        exit 1
    fi
    is_url "$bfb"
    
    if [ "$skip_extract" = true ]; then
        EXTRACTED_BFB_PATH="$bfb"
    else
        extract_bfb "$bfb"
        validate_bfb_sha256
    fi

    trap "file_cleanup" EXIT

    # Detect available rshim interfaces
    local dev_names_det=($(detect_rshims_from_pci))
    if [ "${#dev_names_det[@]}" -eq 0 ]; then
        log_debug "No rshim interfaces detected! Make sure to run the $command_name script from the host device/switch!"
        exit 1
    fi

    # Handle rshim/dpu selection
    local dev_names=()
    if [ -z "$rshim_dev" ]; then
        if [ -z "$dpus" ]; then
            log_debug "No rshim interfaces provided!"
            usage
            exit 1
        fi
        if [ "$dpus" = "all" ]; then
            rshim_dev="all"
        else
            IFS=',' read -ra dpu_names <<< "$dpus"
            validate_dpus "${dpu_names[@]}"
        fi
    fi

    if [ "$rshim_dev" = "all" ]; then
        dev_names=("${dev_names_det[@]}")
        log_info "${#dev_names_det[@]} rshim interfaces detected: ${dev_names_det[*]}"
    else
        if [ ${#dev_names[@]} -eq 0 ]; then
            IFS=',' read -ra dev_names <<< "$rshim_dev"
        fi
        validate_rshim "${dev_names[@]}"
    fi

    if [ ${#rshim2dpu[@]} -eq 0 ]; then
        get_mapping "${dev_names[@]}"
    fi

    # Sort devices and handle config files
    local sorted_devs=($(printf '%s\n' "${dev_names[@]}" | sort))
    local arr=()
    
    if [ -n "$config" ]; then
        log_info "Using ${config} file/s"
        if [[ "$config" == *","* ]]; then
            IFS=',' read -r -a arr <<< "$config"
        else
            arr=("$config")
            for ((i=1; i<${#dev_names[@]}; i++)); do
                arr+=("$config")
            done
        fi

        validate_config_files "${sorted_devs[@]}" "${arr[@]}"
    fi

    # Install BFB on each device
    trap 'kill_ch_procs' SIGINT SIGTERM SIGHUP
    
    for i in "${!sorted_devs[@]}"; do
        rshim_name=${sorted_devs[$i]}
        dpu_name=${rshim2dpu[$rshim_name]}
        bfb_install_call "$rshim_name" "$dpu_name" "$EXTRACTED_BFB_PATH" "${arr[$i]}" &
    done
    wait
}

# Helper function to parse command line arguments
parse_arguments() {
    while [ "$1" != "--" ] && [ -n "$1" ]; do
        case $1 in
            --help|-h)
                usage
                exit 0
                ;;
            --bfb|-b)
                shift
                bfb=$1
                ;;
            --rshim|-r)
                shift
                rshim_dev=$1
                ;;
            --dpu|-d)
                shift
                dpus=$1
                ;;
            --skip-extract|-s)
                skip_extract=true
                ;;
            --config|-c)
                shift
                config=$1
                ;;
            --verbose|-v)
                verbose=true
                ;;
        esac
        shift
    done
}

# Helper function to validate config files
validate_config_files() {
    local -a sorted_devs=("${@:1:${#sorted_devs[@]}}")
    local -a arr=("${@:$((${#sorted_devs[@]}+1))}")

    if [ ${#arr[@]} -ne ${#sorted_devs[@]} ]; then
        log_debug "Length of config file list does not match the devices selected: ${sorted_devs[*]} and ${arr[*]}"
        exit 1
    fi

    for config_file in "${arr[@]}"; do
        if [ ! -f "$config_file" ]; then
            log_debug "Config provided $config_file is not a file! Please check the config file path"
            exit 1
        fi
    done
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
    log_debug "Installation Interrupted.. killing All child procs"
    kill_all_descendant_procs $$
}

appendix=
verbose=false
main "$@"

