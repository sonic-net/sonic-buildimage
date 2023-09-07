#!/bin/bash

# This script is for kubernetes master image usage
# Will mount kubernetes master disk and execute kubernetes entrance script

set -x

# Mount disk from host
mount_point="/from_host"
disk_by_path_dir="/dev/disk/by-path"
# We can't ensure the mounted disk name is sda or sdb
# Currently we specify the disk logic unit number to 2 when create master VM
# We find the correct disk by the disk logic unit number from by-path directory
target_str="lun-2-part1"
disk_path_file=$(ls $disk_by_path_dir | grep $target_str)

# Check whether the disk path file exists
if [ -z "$disk_path_file" ]; then
    echo "Error: Disk path file not found."
    exit 1
fi

# Check the number of lines returned
line_count=$(echo "$disk_path_file" | wc -l)

# If there are multiple lines, exit with status 1
if [ "$line_count" -ne 1 ]; then
    echo "Error: multiple disk path files found."
    exit 1
fi

disk="${disk_by_path_dir}/${disk_path_file}"
mkdir -p $mount_point
mount $disk $mount_point

# check whether it is the first time to boot
first_boot_flag_file="/from_host/first_boot_flag"
if [ -f $first_boot_flag_file ]; then
    exit 0
fi
touch $first_boot_flag_file

# execute entrance script
init_file_name="entrance.sh"
init_file=${mount_point}/${init_file_name}
chmod +x $init_file
source $init_file
