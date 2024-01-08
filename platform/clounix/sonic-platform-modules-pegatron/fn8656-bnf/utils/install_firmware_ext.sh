#!/bin/sh

action=$1

[ -n "${action}" ] || {
    echo "ERROR: no action is found."
    exit 1
}

stop_plt_service()
{
    echo "service start"
    #stop service
    systemctl stop pmon
    systemctl stop system-health
    modprobe -r i2c_mux_pca9641
    echo 0 > /proc/sys/kernel/panic_on_unrecovered_nmi
}

start_plt_service()
{
    echo "service start"
    #restart platfrom servic
    systemctl stop fn8656_bnf-platform-init.service
    systemctl start fn8656_bnf-platform-init.service
    systemctl start pmon
    systemctl start system-health
}



echo "start"
case "$action" in
start)
    start_plt_service
    ;;
stop)
    stop_plt_service
    ;;
*)
    echo "Usage: $0 {cpld-mb|cpld-cpu|mcu-mb|mcu-fb|bios}"
    exit 1
    ;;
esac
echo "done"
