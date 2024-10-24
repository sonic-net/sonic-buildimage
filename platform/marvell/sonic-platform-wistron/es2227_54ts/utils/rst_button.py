import subprocess
import time
from sonic_py_common import device_info

CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/0-0033"

class rst_button_monitor(object):
    def __init__(self):
        self.intr_path = "{}/rst_button_10s".format(CPLD_SYSFS_DIR)

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def check_intr(self):
        val=self.__read_txt_file(self.intr_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return True

    def restore_default(self):
        version_info = device_info.get_sonic_version_info()
        subprocess.run(["rm", "/etc/sonic/config_db.json"])
        time.sleep(0.5)
        image_ver=version_info['build_version']
        path="/host/image-{}/platform/firsttime".format(image_ver)
        cmd="touch {}".format(path)
        subprocess.run(cmd.split())
        time.sleep(0.5)
        subprocess.run(["sync"])
        time.sleep(0.5)
        subprocess.run(["sync"])
        time.sleep(0.5)
        cmd = ["passwd", "admin"]
        try:
            p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
            p.stdin.write("YourPaSsWoRd\n".encode('utf-8'))
            p.stdin.flush()
            p.stdin.write("YourPaSsWoRd\n".encode('utf-8'))
            p.stdin.flush()
        except Exception as e:
            print("restore_default failed: {}".format(e))

def main():
    monitor = rst_button_monitor()
    cont=True

    # Loop forever:
    while cont:
        time.sleep(1)
        cont=monitor.check_intr()

    monitor.restore_default()
    time.sleep(2)

    subprocess.run(["reboot"])


if __name__ == '__main__':
    main()
