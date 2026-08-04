#!/bin/bash

#platform init script for Dell Z9100

source dell_i2c_utils.sh

init_devnum() {
    found=0
    for devnum in 0 1; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # iSMT adapter can be at either dffd0000 or dfff0000
        if [[ $devname == 'SMBus iSMT adapter at '* ]]; then
            found=1
            break
        fi
    done

    [[ $found -eq 0 ]] && echo "cannot find iSMT" && exit 1
}

# Attach/Detach CPU board mux @ 0x70
cpu_board_mux() {
    case $1 in
        "new_device")    i2c_mux_create pca9547 0x70 $devnum 2
                         ;;
        "delete_device") i2c_mux_delete 0x70 $devnum
                         ;;
        *)               echo "z9100_platform: cpu_board_mux: invalid command !"
                         ;;
    esac
}

# Attach/Detach switch board MUX to IOM CPLDs @ 0x71
switch_board_mux() {
    case $1 in
        "new_device")    i2c_mux_create pca9548 0x71 4 10
                         ;;
        "delete_device") i2c_mux_delete 0x71 4
                         ;;
        *)               echo "z9100_platform: switch_board_mux : invalid command !"
                         ;;
    esac
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         ;;
        "delete_device") i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         ;;
        *)               echo "z9100_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach cpld devices to drivers for each iom
switch_board_cpld() {
    case $1 in
        "new_device")    
                      for ((i=14;i<=17;i++));
                      do
                          i2c_config "echo  dell_z9100_iom_cpld 0x3e > /sys/bus/i2c/devices/i2c-$i/$1"
                      done
                      ;;
        "delete_device")
                      for ((i=14;i<=17;i++));
                      do
                          i2c_config "echo  0x3e > /sys/bus/i2c/devices/i2c-$i/$1"
                      done
                      ;;
        *)            echo "z9100_platform: switch_board_cpld : invalid command !"
                      ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_mux() {
    case $1 in
        "new_device")
                      # The mux for the QSFPs spawn {18..25}, {26..33}, {34..41} and {42..49}
                      # starting at chennel 18 and 8 channels per mux.
                      channel_first=18
                      for ((i=9;i>=6;i--));
                      do
                          # 0x71 mux on the IOM 1
                          mux_index=$(expr $i - 5)
                          echo "Attaching PCA9548 $mux_index"
                          i2c_mux_create pca9548 0x71 $i $channel_first
                          channel_first=$(expr $channel_first + 8)
                      done
                      ;;
        "delete_device")
                      for ((i=9;i>=6;i--));
                      do
                          # 0x71 mux on the IOM 1
                          mux_index=$(expr $i - 5)
                          echo "Detaching PCA9548 $mux_index"
                          i2c_mux_delete 0x71 $i
                      done
                      ;;
        *)            echo "z9100_platform: switch_board_qsfp_mux: invalid command !"
                      ;;
    esac
}

#Attach/Detach the SFP modules on PCA9548_2
switch_board_sfp() {
    case $1 in
        "new_device")    i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-11/$1"
                         i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-12/$1"
                         ;;
        "delete_device") i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-11/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-12/$1"
                         ;;
        *)               echo "z9100_platform: switch_board_sfp: invalid command !"
                         ;;
    esac
}

# Attach/Detach 32 instances of EEPROM driver QSFP ports on IO module 1
#eeprom can dump data using below command
switch_board_qsfp() {
    case $1 in
        "new_device")
                        for ((i=18;i<=49;i++));
                        do
                            i2c_config "echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-$i/$1"
                        done
                        ;;
        "delete_device")
                        for ((i=18;i<=49;i++));
                        do
                            i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1"
                        done
                        ;;

        *)              echo "z9100_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

# Enable/Disable xcvr presence interrupts
xcvr_presence_interrupts() {
    case $1 in
        "enable")
                      for ((i=14;i<=16;i++));
                      do
                          echo 0x0 > /sys/bus/i2c/devices/i2c-$i/$i-003e/qsfp_abs_mask
                      done
                      ;;
        "disable")
                      for ((i=14;i<=16;i++));
                      do
                          echo 0xffff > /sys/bus/i2c/devices/i2c-$i/$i-003e/qsfp_abs_mask
                      done
                      ;;
        *)            echo "z9100_platform: xcvr_presence_interrupts: invalid command !"
                      ;;
    esac
}

# Reset the mux tree
reset_muxes() {
    # Reset the IOM muxes and the switch card mux
    io_rd_wr.py --set --val 0xe0 --offset 0x110
    io_rd_wr.py --set --val 0xff --offset 0x110

    # Reset the CPU Card PCA9547
    io_rd_wr.py --set --val 0xfd --offset 0x20b
    io_rd_wr.py --set --val 0xff --offset 0x20b
}

# Copy led_proc_init.soc file according to the HWSKU
init_switch_port_led() {
    T0="Force10-Z9100-C8D48"
    T1="Force10-Z9100-C32"
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
    hwsku=$(cat /etc/sonic/config_db.json | grep -A2 "DEVICE_METADATA" | grep "hwsku" | cut -d ":" -f2 | sed 's/"//g' | sed 's/,//g'| xargs )

    if [ -z "$hwsku" ]; then
          #Check minigraph for hwsku
          cat /etc/sonic/minigraph.xml | grep $T1  > /dev/null
          if [ $? -eq 0 ]; then
                  hwsku=$T1
          else
                  hwsku=$T0
          fi
    fi

    led_proc_init="$device/$platform/$hwsku/led_proc_init.soc"

    # Remove old HWSKU LED file..
    rm -rf $device/$platform/led_proc_init.soc

    if [ -e $led_proc_init ] && [ ! -e $device/$platform/led_proc_init.soc ]; then
      cp $led_proc_init $device/$platform
    fi

}

install_python_api_package() {
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
    rv=$(pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl)
}

remove_python_api_package() {
    rv=$(pip3 show sonic-platform > /dev/null 2>/dev/null)
    if [ $? -eq 0 ]; then
        rv=$(pip3 uninstall -y sonic-platform > /dev/null 2>/dev/null)
    fi
}

init_devnum

if [[ "$1" == "init" ]]; then
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe dell_ich
    modprobe dell_mailbox
    modprobe dell_z9100_cpld

    cpu_board_mux "new_device"
    switch_board_mux "new_device"
    sys_eeprom "new_device"
    switch_board_cpld "new_device"
    switch_board_qsfp_mux "new_device"
    switch_board_sfp "new_device"
    switch_board_qsfp "new_device"
    xcvr_presence_interrupts "enable"

    #Copy led_proc_init.soc
    init_switch_port_led
    install_python_api_package

    value=0x0
    echo $value > /sys/bus/i2c/devices/i2c-14/14-003e/qsfp_lpmode
    echo $value > /sys/bus/i2c/devices/i2c-15/15-003e/qsfp_lpmode
    echo $value > /sys/bus/i2c/devices/i2c-16/16-003e/qsfp_lpmode

elif [[ "$1" == "deinit" ]]; then
    xcvr_presence_interrupts "disable"
    switch_board_sfp "delete_device"
    switch_board_cpld "delete_device"
    switch_board_mux "delete_device"
    sys_eeprom "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_qsfp_mux "delete_device"
    cpu_board_mux "delete_device"

    modprobe -r dell_z9100_cpld
    modprobe -r dell_mailbox
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    modprobe -r dell_ich
    remove_python_api_package
else
     echo "z9100_platform : Invalid option !"
fi
