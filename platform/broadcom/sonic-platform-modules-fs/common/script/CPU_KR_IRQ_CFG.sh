#!/bin/bash

# <<< How to run >>>
# 1. Upload this script to SONiC
# 2. Execute this script, bash xxx.sh
grep -Po '\d+(?=:.*eth0)' /proc/interrupts |xargs -i bash -c 'echo 0002 > /proc/irq/{}/smp_affinity'
grep -Po '\d+(?=:.*ice-eth1)' /proc/interrupts |xargs -i bash -c 'echo 0100 > /proc/irq/{}/smp_affinity'
grep -Po '\d+(?=:.*ice-eth2)' /proc/interrupts |xargs -i bash -c 'echo 0200 > /proc/irq/{}/smp_affinity'
grep -Po '\d+(?=:.*ice-eth3)' /proc/interrupts |xargs -i bash -c 'echo 0400 > /proc/irq/{}/smp_affinity'
grep -Po '\d+(?=:.*ice-eth4)' /proc/interrupts |xargs -i bash -c 'echo 0800 > /proc/irq/{}/smp_affinity'