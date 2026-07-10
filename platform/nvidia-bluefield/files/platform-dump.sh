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

DUMP_FOLDER="/tmp/platform-dump"

dump_cmd () {
	cmd=$1
	output_fname=$2
	timeout=$3
	cmd_name=${cmd%% *}

	if [ -x "$(command -v $cmd_name)" ];
	then
		# ignore shellcheck message SC2016. Arguments should be single-quoted (')
		run_cmd="$cmd > $DUMP_FOLDER/$output_fname"
		timeout "$timeout" bash -c "$run_cmd"
	fi
}

rm -rf $DUMP_FOLDER
mkdir $DUMP_FOLDER

ls -Rla /sys/ > $DUMP_FOLDER/sysfs_tree
uname -a > $DUMP_FOLDER/sys_version
mkdir $DUMP_FOLDER/bin/
cp /usr/bin/platform-dump.sh $DUMP_FOLDER/bin/
cat /etc/os-release >> $DUMP_FOLDER/sys_version
cat /proc/interrupts > $DUMP_FOLDER/interrupts

dump_cmd "dmesg" "dmesg" "10"
dump_cmd "dmidecode -t1 -t2 -t 11" "dmidecode" "3"
dump_cmd "lsmod" "lsmod" "3"
dump_cmd "lspci -vvv" "lspci" "5"
dump_cmd "top -SHb -n 1 | tail -n +8 | sort -nrk 11" "top" "5"
dump_cmd "tail /sys/kernel/debug/mlxbf-ptm/monitors/status/*" "mlxbf-ptm-dump" "3"

pushd /dev/mst/
mstdevs=$(ls mt*)
popd

for mstdev in $mstdevs; do
	dump_cmd "mstdump -full /dev/mst/$mstdev" "mstdump_$mstdev" "20"
done

tar czf /tmp/platform-dump.tar.gz -C $DUMP_FOLDER .
rm -rf $DUMP_FOLDER
