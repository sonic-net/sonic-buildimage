#!/usr/bin/env bash
#
# Script for sai failure dump
#

# Source the platform specific dump file

sai_dump_name="sai_sdk_dump_$(date +"%m_%d_%Y_%I_%M_%p")"
sai_dump_path="${DUMPDIR}/$sai_dump_name"
mkdir -p $sai_dump_path
sai_dump_file="${sai_dump_path}/$sai_dump_name"
saisdkdump -f $sai_dump_file
cd "${DUMPDIR}"
tar -cvf "$sai_dump_name".tar $sai_dump_name
gzip "$sai_dump_name".tar
rm -rf $sai_dump_name

# Update max failure dumps
if grep -q SAI_DUMP_STORE_AMOUNT /usr/share/sonic/hwsku/sai.profile; then
    SAI_MAX_FAILURE_DUMPS=$(grep SAI_DUMP_STORE_AMOUNT /usr/share/sonic/hwsku/sai.profile | cut -d '=' -f2)
fi

