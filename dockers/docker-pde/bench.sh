#!/bin/bash

CSV=/tmp/bench.csv

TST_FREQ=2000
TST_CORE=4
TST_RAMSZ=8
TST_DISKSZ=14
TST_SHA=10
TST_BZ2=30
TST_AES=5
TST_DISKRD=180
TST_DISKWR=30

cpu_benchmark()
{
    export TIMEFORMAT='%1R'
    (time dd if=/dev/zero bs=1M count=500 2> /dev/null | \
     "$@" > /dev/null ) 2>&1
}

# Platform Info
PLATFORM=$(grep 'onie_platform=' /host/machine.conf | cut -d '=' -f 2)
echo "Platform:   ${PLATFORM}"

if [ -d /usr/share/sonic/platform ]; then
    HWSKU=$(head -1 /usr/share/sonic/platform/default_sku | awk '{print $1}')
else
    HWSKU=$(head -1 /usr/share/sonic/device/${PLATFORM}/default_sku | awk '{print $1}')
fi
echo "HWSKU:      ${HWSKU}"

REVISION=$(syseeprom.py 0x27)
echo "Revision:   ${REVISION}"

# Basic Info
CPU_NAME=$(awk -F: '/model name/ {name=$2} END {print name}' /proc/cpuinfo | cut -d '@' -f 1 | sed 's/^[ \t]*//;s/[ \t]*$//')
echo "CPU NAME:   ${CPU_NAME}"

if [ $(lscpu | grep -c 'CPU max MHz') -gt 0 ]; then
    CPU_FREQ=$(lscpu | grep 'CPU max MHz' | cut -d ':' -f 2 | cut -d '.' -f 1 | sed 's/^[[:space:]]*//')
else
    CPU_FREQ=$(lscpu | grep 'CPU MHz' | cut -d ':' -f 2 | cut -d '.' -f 1 | sed 's/^[[:space:]]*//')
fi
echo -n "CPU FREQ:   ${CPU_FREQ} MHz ........"
if [ ${CPU_FREQ} -ge ${TST_FREQ} ]; then
    echo "pass"
else
    echo "FAIL"
fi

CPU_CORE=$(awk -F: '/model name/ {core++} END {print core}' /proc/cpuinfo)
echo -n "CPU CORE:   ${CPU_CORE} ..............."
if [ ${CPU_CORE} -ge ${TST_CORE} ]; then
    echo "pass"
else
    echo "FAIL"
fi

RAM_SIZE=$(grep MemTotal /proc/meminfo | awk '{print $2}')
RAM_SIZE=$(expr ${RAM_SIZE} / 1024) # MB
RAM_SIZE=$(expr ${RAM_SIZE} + 1023)
RAM_SIZE=$(expr ${RAM_SIZE} / 1024) # GB
echo -n "RAM SIZE:   ${RAM_SIZE} GB ..........."
if [ ${RAM_SIZE} -ge ${TST_RAMSZ} ]; then
    echo "pass"
else
    echo "FAIL"
fi

DISK_SIZE=$(lsblk -d | grep disk | head -1 | awk '{print $4}' | cut -d 'G' -f 1)
echo -n "DISK:       ${DISK_SIZE} GB ..........."
if [ $(expr ${DISK_SIZE} \>= ${TST_DISKSZ}) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

# CPU tests
SHA_TIME=$(cpu_benchmark sha256sum)
echo -n "SHA256:     ${SHA_TIME} sec ........."
if [ $(echo "${SHA_TIME} <= ${TST_SHA}" | bc) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

BZ2_TIME=$(cpu_benchmark bzip2)
echo -n "BZIP2:      ${BZ2_TIME} sec ........."
if [ $(echo "${BZ2_TIME} <= ${TST_BZ2}" | bc) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

AES_TIME=$(cpu_benchmark openssl enc -e -aes-256-cbc -pass pass:12345678 | sed '/^\*\*\* WARNING : deprecated key derivation used\.$/d;/^Using -iter or -pbkdf2 would be better\.$/d')
echo -n "AES256:     ${AES_TIME} sec ........."
if [ $(echo "${AES_TIME} <= ${TST_AES}" | bc) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

# Disk tests
DISK_RD=$(ioping -DRL -w 5 ./ | tail -n 2 | head -n 1 | cut -d ',' -f 4 | awk '{print $1}')
echo -n "DISK READ:  ${DISK_RD} MB/s ......"
if [ $(echo "${DISK_RD} >= ${TST_DISKRD}" | bc) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

rm -f dummy.bin
DISK_WR=$(dd if=/dev/zero of=dummy.bin bs=64k count=16k conv=fdatasync 2>&1 | tail -1 | awk '{print $10}')
rm -f dummy.bin
echo -n "DISK WRITE: ${DISK_WR} MB/s ......."
if [ $(echo "${DISK_WR} >= ${TST_DISKWR}" | bc) -eq 1 ]; then
    echo "pass"
else
    echo "FAIL"
fi

# Report
rm -f ${CSV}
echo "Platform,HwSKU,Revision,Processor,CPU cores,Frequency(MHz),RAM(G),Disk(G),SHA256(s),bzip2(s),AES(s),sequential read(MiB/s),sequential write(MiB/s)," >> ${CSV}

echo -n "${PLATFORM}," >> ${CSV}
echo -n "${HWSKU}," >> ${CSV}
echo -n "${REVISION}," >> ${CSV}
echo -n "${CPU_NAME}," >> ${CSV}
echo -n "${CPU_CORE}," >> ${CSV}
echo -n "${CPU_FREQ}," >> ${CSV}
echo -n "${RAM_SIZE}," >> ${CSV}
echo -n "${DISK_SIZE}," >> ${CSV}
echo -n "${SHA_TIME}," >> ${CSV}
echo -n "${BZ2_TIME}," >> ${CSV}
echo -n "${AES_TIME}," >> ${CSV}
echo -n "${DISK_RD}," >> ${CSV}
echo -n "${DISK_WR}," >> ${CSV}
echo "" >> ${CSV}
