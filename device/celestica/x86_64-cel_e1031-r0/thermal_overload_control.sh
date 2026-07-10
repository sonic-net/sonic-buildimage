#!/bin/bash
#
# Copyright 2020-present Celestica. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
SETREG_FILE=/sys/devices/platform/e1031.smc/setreg
TOVERREG=0x115
CPUOVER=0xa1
ASICOVER=0xa2

prog="$0"
command="$1"

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

usage() {
    echo "Usage: thermal_overload_control.sh [option] <command>"
    echo
    echo "Options:"
    echo "  -h, --help          : to print this message."
    echo
    echo "Commands:"
    echo
    echo "  cpu:  To enabling CPU thermal overload handler"
    echo
    echo "  asic : To enabling ASIC thermal overload handler"
    echo
}

cpu_overload() {
	logger "Enable CPU thermal overload control"
	set_reg=`echo ${TOVERREG} ${CPUOVER} > ${SETREG_FILE}`
}

asic_overload() {
	logger "Enable ASIC thermal overload control"
	set_reg=`echo ${TOVERREG} ${ASICOVER} > ${SETREG_FILE}`
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

case "$command" in
-h | --help)
    usage
    ;;
cpu)
	cpu_overload
	;;
asic)
	asic_overload
	;;
*)
	usage
	exit -1
	;;
esac

exit $?
