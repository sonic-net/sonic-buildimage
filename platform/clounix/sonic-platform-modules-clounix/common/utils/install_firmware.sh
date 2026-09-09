#!/bin/sh

firmware=$1
bios_file=$2

[ -n "${bios_file}" ] || {
    echo "ERROR: unable to read UEFI Image"
    exit 1
}

case "$firmware" in
cpld | mcu)
    echo "${firmware} upgrade did not implement"
    exit 1
    ;;
bios)
    cd /usr/local/bin/bios_bin
    ./afulnx_64 ${bios_file} /p /b /n
    ;;
*)
    echo "Usage: $0 {cpld|mcu|bios}"
    exit 1
    ;;
esac

