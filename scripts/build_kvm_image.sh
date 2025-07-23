#!/bin/bash -ex

#  Copyright (C) 2014 Curt Brune <curt@cumulusnetworks.com>
#
#  SPDX-License-Identifier:     GPL-2.0

MEM=10240
DISK=$1
ONIE_RECOVERY_ISO=$2
INSTALLER=$3
DISK_SIZE=$4

INSTALLER_DISK="./sonic-installer.img"

# VM will listen on telnet port $KVM_PORT
KVM_PORT=9000

on_exit()
{
    rm -f $kvm_log
}

on_error()
{
    netstat -antp
    ps aux
    echo "============= kvm_log =============="
    cat $kvm_log
}

create_disk()
{
    echo "Creating SONiC kvm disk : $DISK of size $DISK_SIZE GB"
	qemu-img create -f qcow2 $DISK ${DISK_SIZE}G
}

prepare_installer_disk()
{
    fallocate -l 5120M $INSTALLER_DISK

    mkfs.ext2 $INSTALLER_DISK

    tmpdir=$(mktemp -d)

    mount -o loop $INSTALLER_DISK $tmpdir

    cp $INSTALLER $tmpdir/onie-installer.bin

    umount $tmpdir
}

wait_kvm_ready()
{
    local count=30
    local waiting_in_seconds=2.0
    for ((i=1; i<=$count; i++)); do
        sleep $waiting_in_seconds
        echo "$(date) [$i/$count] waiting for the port $KVM_PORT ready"
        if netstat -l | grep -q ":$KVM_PORT"; then
          break
        fi
    done
}

apt-get install -y net-tools
create_disk
prepare_installer_disk

echo "Prepare memory for KVM build: $vs_build_prepare_mem"
mount proc /proc -t proc || true
free -m
if [[ "$vs_build_prepare_mem" == "yes" ]]; then
    # Force o.s. to drop cache and compact memory so that KVM can get 2G memory
    bash -c 'echo 1 > /proc/sys/vm/drop_caches'
    # Not all kernels support compact_memory
    if [[ -w '/proc/sys/vm/compact_memory' ]]
    then
        bash -c 'echo 1 > /proc/sys/vm/compact_memory'
    fi
    free -m
fi

kvm_log=$(mktemp)
trap on_exit EXIT
trap on_error ERR

echo "Installing SONiC"

/usr/bin/kvm -m $MEM \
    -name "onie" \
    -boot "order=cd,once=d" -cdrom "$ONIE_RECOVERY_ISO" \
    -device e1000,netdev=onienet \
    -netdev user,id=onienet,hostfwd=:0.0.0.0:3041-:22 \
    -vnc 0.0.0.0:0 \
    -vga std \
    -drive file=$DISK,media=disk,if=virtio,index=0 \
    -drive file=$INSTALLER_DISK,if=virtio,index=1 \
    -serial telnet:127.0.0.1:$KVM_PORT,server > $kvm_log 2>&1 &

kvm_pid=$!

wait_kvm_ready

[ -d "/proc/$kvm_pid" ] || {
        echo "ERROR: kvm died."
        cat $kvm_log
        exit 1
}

echo "to kill kvm:  sudo kill $kvm_pid"

./install_sonic.py

kill $kvm_pid

echo "Booting up SONiC"

/usr/bin/kvm -m $MEM \
    -name "onie" \
    -device e1000,netdev=onienet \
    -netdev user,id=onienet,hostfwd=:0.0.0.0:3041-:22 \
    -vnc 0.0.0.0:0 \
    -vga std \
    -snapshot \
    -drive file=$DISK,media=disk,if=virtio,index=0 \
    -serial telnet:127.0.0.1:$KVM_PORT,server > $kvm_log 2>&1 &

kvm_pid=$!

wait_kvm_ready

[ -d "/proc/$kvm_pid" ] || {
        echo "ERROR: kvm died."
        cat $kvm_log
        exit 1
}

echo "to kill kvm:  sudo kill $kvm_pid"

./check_install.py -u $SONIC_USERNAME -P $PASSWD -p $KVM_PORT

kill $kvm_pid

exit 0
