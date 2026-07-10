#!/bin/bash
onie_dev=$(blkid | grep ONIE-BOOT | head -n 1 | awk '{print $1}' |  sed -e 's/:.*$//')
mkdir -p /mnt/onie-boot
mount $onie_dev /mnt/onie-boot
onie_grub_cfg=/mnt/onie-boot/onie/grub/grub-machine.cfg

if [ ! -e $onie_grub_cfg ]; then
    echo "$onie_grub_cfg not found"
else
    oldoniebuilddate=`cat /host/machine.conf|grep "onie_build_date="`
    oniebuilddate=`cat $onie_grub_cfg|grep "onie_build_date="|sed "s/\"//g"`
    onieversion=`cat $onie_grub_cfg|grep "onie_version="`
    oniekernelversion=`cat $onie_grub_cfg|grep "onie_kernel_version="`
    if [ "$oldoniebuilddate" != "$oniebuilddate" ]; then
        echo "update /home/machine.conf"
        sed -i "s/onie_build_date=.*/$oniebuilddate/" /host/machine.conf
        sed -i "s/onie_version=.*/$onieversion/" /host/machine.conf
        sed -i "s/onie_kernel_version=.*/$oniekernelversion/" /host/machine.conf
        sed -i "s/\"//g" /host/machine.conf
    fi
fi
umount /mnt/onie-boot


