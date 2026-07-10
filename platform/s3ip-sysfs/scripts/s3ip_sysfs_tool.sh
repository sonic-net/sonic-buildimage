#! /bin/bash

s3ip_start(){
        sudo insmod /lib/modules/s3ip/s3ip_sysfs.ko
        sudo insmod /lib/modules/s3ip/syseeprom_device_driver.ko
        sudo insmod /lib/modules/s3ip/fan_device_driver.ko
        sudo insmod /lib/modules/s3ip/cpld_device_driver.ko
        sudo insmod /lib/modules/s3ip/sysled_device_driver.ko
        sudo insmod /lib/modules/s3ip/psu_device_driver.ko
        sudo insmod /lib/modules/s3ip/transceiver_device_driver.ko
        sudo insmod /lib/modules/s3ip/temp_sensor_device_driver.ko
        sudo insmod /lib/modules/s3ip/vol_sensor_device_driver.ko
        sudo insmod /lib/modules/s3ip/fpga_device_driver.ko
        sudo insmod /lib/modules/s3ip/watchdog_device_driver.ko
        sudo insmod /lib/modules/s3ip/curr_sensor_device_driver.ko
        sudo insmod /lib/modules/s3ip/slot_device_driver.ko
        sudo rm -rf /sys_switch
        sudo /usr/bin/s3ip_load.py
        echo "s3ip service start"
}
s3ip_stop(){
        sudo rmmod slot_device_driver
        sudo rmmod curr_sensor_device_driver
        sudo rmmod watchdog_device_driver
        sudo rmmod fpga_device_driver
        sudo rmmod vol_sensor_device_driver
        sudo rmmod temp_sensor_device_driver
        sudo rmmod transceiver_device_driver
        sudo rmmod psu_device_driver
        sudo rmmod sysled_device_driver
        sudo rmmod cpld_device_driver
        sudo rmmod fan_device_driver
        sudo rmmod syseeprom_device_driver
        sudo rmmod s3ip_sysfs
        sudo rm -rf /sys_switch
	echo "s3ip service stop"

}

case "$1" in
    start)
	s3ip_start
        ;;
    stop)
	s3ip_stop
	;;
    status)
        sudo tree -l /sys_switch
	;;
    restart)
	s3ip_stop
	s3ip_start
	;;	
    *)
        echo "Usage: $0 {start|stop|status|restart}"
	exit 1
esac
exit 

