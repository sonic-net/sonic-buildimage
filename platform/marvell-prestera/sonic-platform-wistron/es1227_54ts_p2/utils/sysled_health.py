import os
import sonic_platform
import time

def main():

    pl=sonic_platform.platform.Platform()
    cha=pl.get_chassis()
    
    os.system("echo 1 > /sys/bus/i2c/devices/0-0033/sys_led")
    # Loop forever:
    while True:
        time.sleep(5)
        cha.set_status_led("green")

if __name__ == '__main__':
    main()
