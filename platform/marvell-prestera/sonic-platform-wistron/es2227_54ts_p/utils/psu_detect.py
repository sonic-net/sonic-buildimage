import sonic_platform
import time
import subprocess
import os

class psu_monitor(object):
    def __init__(self):
        self.pl = sonic_platform.platform.Platform()
        self.cha = self.pl.get_chassis()
        self.pre_sta = [True] * 2
        for i in range(0, 2):
            psu = self.cha.get_psu(i)
            self.pre_sta[i] = psu.get_presence()
            if not self.pre_sta[i]:
                self.remove_device(i)


    def remove_device(self, index):
        psu = self.cha.get_psu(index)
        path="/sys/bus/i2c/devices/i2c-3/delete_device"
        cmd="echo 0x" + psu.i2c_addr + " > " + path
        driver_path="/sys/bus/i2c/devices/3-00"+psu.i2c_addr
        if os.path.exists(driver_path):
            subprocess.getstatusoutput(cmd)
        cmd="echo 0x" + str(int(psu.i2c_addr) - 8) + " > " + path
        driver_path="/sys/bus/i2c/devices/3-00"+ str(int(psu.i2c_addr) - 8)
        if os.path.exists(driver_path):
            subprocess.getstatusoutput(cmd)

    def probe_device(self, index):
        psu = self.cha.get_psu(index)
        path="/sys/bus/i2c/devices/i2c-3/new_device"
        cmd="echo pmbus 0x" + psu.i2c_addr + " > " + path
        driver_path="/sys/bus/i2c/devices/3-00"+psu.i2c_addr
        if not os.path.exists(driver_path):
            subprocess.getstatusoutput(cmd)
        cmd="echo wistron_eeprom 0x" + str(int(psu.i2c_addr) - 8) + " > " + path
        driver_path="/sys/bus/i2c/devices/3-00"+str(int(psu.i2c_addr) - 8)
        if not os.path.exists(driver_path):
            subprocess.getstatusoutput(cmd)

    def update_psu(self):
        #fan status detect
        for i in range(0, 2):
            psu = self.cha.get_psu(i)
            pres = psu.get_presence()

            if pres and not self.pre_sta[i]:
                self.probe_device(i)
            elif not pres and self.pre_sta[i]:
                self.remove_device(i)

            self.pre_sta[i] = pres

def main():
    monitor = psu_monitor()

    # Loop forever:
    while True:
        time.sleep(1)
        monitor.update_psu()

if __name__ == '__main__':
    main()
