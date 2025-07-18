#!/bin/sh

firmware=$1
firmware_file=$2
bin_path=/usr/local/bin
log_file=/run/fw_updates_log
identify_file=/usr/local/bin/fw_updates_identify

[ -n "${firmware_file}" ] || {
    echo "ERROR: unable to read firmware Image"
    exit 1
}

stop_plt_service()
{
    #stop service
    systemctl stop pmon
    systemctl stop system-health
    modprobe -r i2c_mux_pca9641
    echo 0 > /proc/sys/kernel/panic_on_unrecovered_nmi
}

start_plt_service()
{
    #auto restart sytem for firmware upgrade later
    return
    #restart platfrom servic
    systemctl stop fn8656_bnf-platform-init.service
    systemctl start fn8656_bnf-platform-init.service
    systemctl start pmon
    systemctl start system-health
}

mcu_fw_install()
{
    _file_name=$firmware_file

    _mb_type=0
    if [ "$firmware" = "mcu-mb" ]; then
        _mb_type=1
    fi
    echo "${firmware} mcu_fw_install, please wait..." >> $log_file
    stop_plt_service
    #configuraton for programming
    i2cset -y 0 0x71 0x3 0x00
    i2cset -y 0 0x71 0x1 0x7
    i2cset -y 0 0x72 0x0 0x7
    #GPIO17_UART1_SEL:0 is for upgrade MUC used and 1 is for i2c bridge used(default)
    #GPIO54_MB_FCn_MCU_SEL:0 is for Fan control board MCU upgrade and 1 is for MB MCU upgrade
    gpio_uart_sel=453 #436+17=453
    gpio_mcu_sel=490 #436+54=490
    echo $gpio_uart_sel > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio453/direction
    echo 0 > /sys/class/gpio/gpio453/value #pull low for GPIO-17 to UART selection
    echo $gpio_mcu_sel > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio490/direction
    if [ $_mb_type = 1 ]; then
        echo 1 > /sys/class/gpio/gpio490/value #pull high for GPIO-54 to UART selection MB
    else
        echo 0 > /sys/class/gpio/gpio490/value #pull low for GPIO-54 to UART selection FB
    fi
    if [ $_mb_type = 1 ]; then
        #write 0xA5 to MB_FW_UG
        i2cset -y 0 0x70 0x0 0xa5
    else
        #write 0xA5 to FB_FW_UG
        i2cset -y 0 0x70 0x1 0xa5
    fi
    #i2cget -y 0 0x70 0x1
    #upgrade MCU FW
    cd /usr/local/bin/mcu_upgrade
    echo "${firmware} mcu_fw_install, please wait..." >> $log_file
    python3 $bin_path/mcu_upgrade/efm8load.py -p /dev/ttyS1 $_file_name
    echo "${firmware} mcu_fw_install, done" >> $log_file
    cd -
    #for store GPIOs used
    echo $gpio_uart_sel > /sys/class/gpio/unexport
    echo $gpio_mcu_sel > /sys/class/gpio/unexport
    start_plt_service

}

#loading for main board CPLD A/B/C
cpld_mb_install()
{
    _file_name=$firmware_file
    
    stop_plt_service
    #configuraton for GPIO
    #GPIO6    GPIO6_CPLD_DTO       JTAG_TDO
    #GPIO24   GPIO24_SUS_CPLD_TCK  JTAG_TCK
    #GPIO32   GPIO32_CPLD_TDI      JTAG_TDI
    #GPIO50   GPIO50_CPLD_TM5      JTAG_TM5 
    echo 442 > /sys/class/gpio/export 
    echo 460 > /sys/class/gpio/export 
    echo 468 > /sys/class/gpio/export 
    echo 486 > /sys/class/gpio/export 
    echo "out" > /sys/class/gpio/gpio486/direction
    echo "out" > /sys/class/gpio/gpio468/direction
    echo "out" > /sys/class/gpio/gpio460/direction
    echo "in" > /sys/class/gpio/gpio442/direction
    #must copy to same folder
    cp $_file_name $bin_path/cpld_upgrade
    tmp_file=$bin_path/cpld_upgrade/${_file_name##*/}
    #upgrade CPLD load
    $bin_path/cpld_upgrade/cpld_upgrade_bdxde_mb $tmp_file
    rm $tmp_file

    #restore GPOIs
    echo 442 > /sys/class/gpio/unexport
    echo 460 > /sys/class/gpio/unexport
    echo 468 > /sys/class/gpio/unexport
    echo 486 > /sys/class/gpio/unexport
    start_plt_service
}

#loading for CPU board CPLD D
cpld_cpu_install()
{
    _file_name=$firmware_file

    stop_plt_service
    #configuraton for GPIO
    #GPIO27   GPIO27_CPLD_DTO      JTAG_TDO
    #GPIO61   GPIO61_SUS_CPLD_TCK  JTAG_TCK
    #GPIO68   GPIO68_CPLD_TDI      JTAG_TDI
    #GPIO12   GPIO12_CPLD_TM5      JTAG_TM5 
    #GPIO25   GPIO25_SUS_CPLD_DOWNLOAD_SEL
    echo 448 > /sys/class/gpio/export 
    echo 461 > /sys/class/gpio/export 
    echo 463 > /sys/class/gpio/export 
    echo 497 > /sys/class/gpio/export 
    echo 504 > /sys/class/gpio/export 
    echo "out" > /sys/class/gpio/gpio448/direction
    echo "out" > /sys/class/gpio/gpio461/direction
    echo "out" > /sys/class/gpio/gpio497/direction
    echo "out" > /sys/class/gpio/gpio504/direction
    echo "in" > /sys/class/gpio/gpio463/direction
    echo 1 > /sys/class/gpio/gpio461/value
    echo 1 > /sys/class/gpio/gpio448/value
    echo 1 > /sys/class/gpio/gpio504/value 
    #About Upgrading CPU CPLD, due to JTAG-MUX set to BMC channel,
    #so here need additional step to control CPLD(D) BMCR1 register bit [0] to CPU sid
    i2cset -y 0 0x71 0x3 0x00
    i2cset -y 0 0x71 0x1 0x7
    i2cset -y 0 0x18 0x0B 0x29
    #must copy to same folder
    cp $_file_name $bin_path/cpld_upgrade
    tmp_file=$bin_path/cpld_upgrade/${_file_name##*/}
    #upgrade CPLD load
    $bin_path/cpld_upgrade/cpld_upgrade_bdxde_cpu $tmp_file
    rm $tmp_file

    #resore GPIOs
    echo 448 > /sys/class/gpio/unexport
    echo 461 > /sys/class/gpio/unexport
    echo 463 > /sys/class/gpio/unexport
    echo 497 > /sys/class/gpio/unexport
    echo 504 > /sys/class/gpio/unexport
    start_plt_service
}

echo "${firmware} loading, please wait..."
touch $log_file
touch $identify_file
echo "${firmware} loading, please wait..." >> $log_file
case "$firmware" in
mcu-fb | mcu-mb)
    mcu_fw_install
    ;;
cpld-mb)
    cpld_mb_install
    ;;
cpld-cpu)
    cpld_cpu_install
    ;;
fpga)
    cd /usr/local/bin/fpga_bin
    ./fpga_bin -f ${firmware_file} -b 3
    ;;
bios)
    cd /usr/local/bin/bios_bin
    ./afulnx_64 ${firmware_file} /p /b /n
    ;;
*)
    echo "Usage: $0 {cpld-mb|cpld-cpu|mcu-mb|mcu-fb|bios}"
    exit 1
    ;;
esac
echo "${firmware} upgrade is done."
echo "${firmware} upgrade done." >> $log_file
echo "${firmware} upgrade done" >> $identify_file
