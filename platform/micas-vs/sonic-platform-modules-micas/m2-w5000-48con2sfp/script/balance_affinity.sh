#!/bin/bash
balance_core_uart(){
    first_irq=`grep -m1 mc-serial /proc/interrupts | sed -n 's/^[ \t]*\([0-9]\+\).*/\1/p'`
    last_irq=`expr $first_irq + 23`
    for i in $(seq $first_irq $last_irq)
    do
        if [ $i -ge $first_irq ] && [ $i -le `expr $first_irq + 3` ]; then
            echo "4" > /proc/irq/$i/smp_affinity
        elif [ $i -ge `expr $first_irq + 4` ] && [ $i -le `expr $first_irq + 7` ]; then
            echo "8" > /proc/irq/$i/smp_affinity
        elif [ $i -ge `expr $first_irq + 8` ] && [ $i -le `expr $first_irq + 11` ]; then
            echo "10" > /proc/irq/$i/smp_affinity
        elif [ $i -ge `expr $first_irq + 12` ] && [ $i -le `expr $first_irq + 15` ]; then
            echo "20" > /proc/irq/$i/smp_affinity
        elif [ $i -ge `expr $first_irq + 16` ] && [ $i -le `expr $first_irq + 19` ]; then
            echo "40" > /proc/irq/$i/smp_affinity
        elif [ $i -ge `expr $first_irq + 20` ] && [ $i -le $last_irq ]; then
            echo "80" > /proc/irq/$i/smp_affinity
        fi
    done
}
process=`cat /proc/cpuinfo | grep "processor" | wc -l`
if [ $process -eq 8 ]; then
    echo "====balancing uart smp affinity===="
    balance_core_uart
fi
