# serialutil.py
#
# Platform-specific Serial interface for SONiC
#

try:
    import os
    import subprocess
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

def subprocess_os_cmd(cmd):
    if len(cmd) <= 0:
        return -1
    p = subprocess.Popen(cmd, shell=False)
    p.wait()
    returncode = p.returncode
    if returncode !=0:
        return -1
    else:
        return 0

class SerialUtil(object):
    MIN_CONSOLE = 1
    MAX_CONSOLE = 48
    serial_pci_path = "/sys/bus/pci/devices/0000:01:00.0/"
    serial_platform_path = "/sys/bus/platform/devices/mc-uart.%d/tty/ttyMC%d/"
    console_stty_cmd = "sudo stty -F /dev/ttyMC%d "
    CONSOLE_TO_DEVICE_MAP = {
        "1":"/dev/ttyMC0",
        "2":"/dev/ttyMC1",
        "3":"/dev/ttyMC2",
        "4":"/dev/ttyMC3",
        "5":"/dev/ttyMC4",
        "6":"/dev/ttyMC5",
        "7":"/dev/ttyMC6",
        "8":"/dev/ttyMC7",
        "9":"/dev/ttyMC8",
        "10":"/dev/ttyMC9",
        "11":"/dev/ttyMC10",
        "12":"/dev/ttyMC11",
        "13":"/dev/ttyMC12",
        "14":"/dev/ttyMC13",
        "15":"/dev/ttyMC14",
        "16":"/dev/ttyMC15",
        "17":"/dev/ttyMC16",
        "18":"/dev/ttyMC17",
        "19":"/dev/ttyMC18",
        "20":"/dev/ttyMC19",
        "21":"/dev/ttyMC20",
        "22":"/dev/ttyMC21",
        "23":"/dev/ttyMC22",
        "24":"/dev/ttyMC23",
        "25":"/dev/ttyMC24",
        "26":"/dev/ttyMC25",
        "27":"/dev/ttyMC26",
        "28":"/dev/ttyMC27",
        "29":"/dev/ttyMC28",
        "30":"/dev/ttyMC29",
        "31":"/dev/ttyMC30",
        "32":"/dev/ttyMC31",
        "33":"/dev/ttyMC32",
        "34":"/dev/ttyMC33",
        "35":"/dev/ttyMC34",
        "36":"/dev/ttyMC35",
        "37":"/dev/ttyMC36",
        "38":"/dev/ttyMC37",
        "39":"/dev/ttyMC38",
        "40":"/dev/ttyMC39",
        "41":"/dev/ttyMC40",
        "42":"/dev/ttyMC41",
        "43":"/dev/ttyMC42",
        "44":"/dev/ttyMC43",
        "45":"/dev/ttyMC44",
        "46":"/dev/ttyMC45",
        "47":"/dev/ttyMC46",
        "48":"/dev/ttyMC47",
    }

    def __init__(self):
        self.baudrate_sysfs_path = self.serial_pci_path + "mc_uart_baudrate%d"
        self.present_sysfs_path = self.serial_pci_path + "mc_uart_presence%d"

    def console_check(self, console):
        if console < self.MIN_CONSOLE or console > self.MAX_CONSOLE:
            return -1
        else:
            return 0

    def serial_read_sysfs_file(self, sysfspath):
        if not os.path.exists(sysfspath):
            return False, ""
        else:
            try:
                with open(sysfspath, "rb", buffering=0) as sysfsfile:
                    buff = sysfsfile.read()
            except Exception as err:
                return False, ""
            return True, buff
    
    def set_console_baudrate(self, console, new_baudrate):
        """
        Set console baudrate
        @param console: console ID, start with 1
        @param new_baudrate: new console baudrate
        @return: 0 for sucess, return -1 if error
        """
        if self.console_check(console) != 0:
            return -1
        ret = subprocess_os_cmd((self.console_stty_cmd + "%d") % (console - 1, new_baudrate))
        return ret

    def get_console_baudrate(self, console):
        """ 
        Get console baudrate.
        @param console: console ID, start with 1
        @return: the console baudrate, return -1 if error
        """ 
        baudrate_sysfs_path = self.baudrate_sysfs_path % (console - 1)
        ret, baudrate = self.serial_read_sysfs_file(baudrate_sysfs_path)
        baudrate = baudrate.strip()
        if ret and baudrate.isdigit():
            return baudrate
        return None

    def get_console_link_status(self, console):
        """ 
        Get the console link status.
        @param console: console ID, start with 1
        @return: the console link status: Present or Not present
        """ 
        index = console - 1
        present_sysfs_path = self.present_sysfs_path % (int(index / 8))
        # Present need to check console
        if self.console_check(console) != 0:
            return "Not present"
        ret, present = self.serial_read_sysfs_file(present_sysfs_path)
        present = present.strip()
        if ret and present.isdigit():
            present = int(present, 10) & (1 << (index % 8))
            if present:
                return "Present"
        return "Not present"

    def get_console_nums(self):
        return self.MAX_CONSOLE

    def get_console_device_map(self):
        return self.CONSOLE_TO_DEVICE_MAP
