import os
import json
import fcntl
import glob
from sonic_py_common import device_info
from sonic_py_common import logger
from threading import Lock
from typing import cast
from sonic_py_common.general import getstatusoutput_noshell_pipe
from sonic_py_common.general import getstatusoutput_noshell

HOST_CHK_CMD = ["docker"]
EMPTY_STRING = ""
MACHINE_CONF_FILE = "/host/machine.conf"
HOST_PLATFORM_JSON_FILE = "/usr/share/sonic/device/{}/platform.json"
PMON_PLATFORM_JSON_FILE = "/usr/share/sonic/platform/platform.json"
INTF_KEY = "interfaces"


class APIHelper():

    def __init__(self):
        pass

    def is_host(self):
        try:
            status, output = getstatusoutput_noshell(HOST_CHK_CMD)
            return status == 0
        except Exception:
            return False

    def read_txt_file(self, file_path):
        try:
            with open(file_path, encoding='unicode_escape', errors='replace') as fd:
                data = fd.read()
                ret = data.strip()
                if len(ret) > 0:
                    return ret
        except IOError:
            pass
        return None

    def glob_read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, encoding='unicode_escape', errors='replace') as fd:
                    data = fd.read()
                    ret = data.strip()
                    if len(ret) > 0:
                        return ret
            except IOError as e:
                pass

        return None

    def write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
                fd.flush()
        except IOError:
            return False
        return True

    def glob_write_txt_file(self, file_path, value):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'w') as fd:
                    fd.write(str(value))
                    fd.flush()
                return True
            except IOError as e:
                pass

        return False

    def ipmi_raw(self, netfn, cmd):
        status = True
        result = ""
        try:
            err, raw_data = getstatusoutput_noshell_pipe(['ipmitool', 'raw', str(netfn), str(cmd)])
            if err == [0]:
                result = raw_data.strip()
            else:
                status = False
        except Exception:
            status = False
        return status, result

    def ipmi_fru_id(self, id, key=None):
        status = True
        result = ""
        try:
            if (key is None):
                err, raw_data = getstatusoutput_noshell_pipe(['ipmitool', 'fru', 'print', str(id)])
            else:
                err, raw_data = getstatusoutput_noshell_pipe(['ipmitool', 'fru', 'print', str(id)], ['grep', str(key)])
            if err == [0] or err == [0, 0]:
                result = raw_data.strip()
            else:
                status = False
        except Exception:
            status = False
        return status, result

    def ipmi_set_ss_thres(self, id, threshold_key, value):
        status = True
        result = ""
        try:
            err, raw_data = getstatusoutput_noshell_pipe(['ipmitool', 'sensor', 'thresh', str(id), str(threshold_key), str(value)])
            if err == [0]:
                result = raw_data.strip()
            else:
                status = False
        except Exception:
            status = False
        return status, result

    def get_intf_name(self):
        """
        Fetches interface names indexed by port numbers from platform.json file.

        Returns:
        - A dictionary with port indices as keys and interface names as values.
          Example: {1: 'Ethernet0', ...}.
        - Returns an empty dictionary on errors or if key not found.
        """
        platform_json_file_path = PMON_PLATFORM_JSON_FILE
        if self.is_host():
            platform = "None"
            with open(MACHINE_CONF_FILE, 'r') as file:
                for line in file:
                    if 'onie_platform=' in line:
                        platform = line.strip().split('=')[1]
                        break
            platform_json_file_path = HOST_PLATFORM_JSON_FILE.format(platform)

        port_dict = {}
        try:
            with open(platform_json_file_path) as fp:
                port_dict = json.load(fp)
        except Exception as e:
            pass

        intf_name_by_index = {}
        if INTF_KEY in port_dict:
            for intf_name, port_info in port_dict[INTF_KEY].items():
                if 'index' in port_info:
                    fp_port_index = int(port_info['index'].split(",")[0])
                    intf_name_by_index[fp_port_index] = intf_name

        return intf_name_by_index


class FileLock:
    """
    Due to pmon docker not installing the py-filelock, this class 
    implements a simple file lock feature.
    Ref: https://github.com/tox-dev/py-filelock/blob/main/src/filelock/
    """

    def __init__(self, lock_file):
        self._lock_file = lock_file
        self._thread_lock = Lock()
        self.is_locked = False

    def acquire(self):
        with self._thread_lock:
            if self.is_locked:
                return

            fd = os.open(self._lock_file, flags=(os.O_RDWR | os.O_CREAT | os.O_TRUNC))
            fcntl.flock(fd, fcntl.LOCK_EX)
            self._lock_file_fd = fd
            self.is_locked = True

    def release(self):
        with self._thread_lock:
            if self.is_locked:
                fd = cast(int, self._lock_file_fd)
                self._lock_file_fd = None
                fcntl.flock(fd, fcntl.LOCK_UN)
                os.close(fd)
                self.is_locked = False

    def __enter__(self):
        self.acquire()
        return self

    def __exit__(self, exc_type, exc_val, traceback):
        self.release()

    def __del__(self):
        self.release()


DEVICE_THRESHOLD_JSON_PATH = "/tmp/device_threshold.json"

class DeviceThreshold:
    HIGH_THRESHOLD = 'high_threshold'
    LOW_THRESHOLD = 'low_threshold'
    HIGH_CRIT_THRESHOLD = 'high_critical_threshold'
    LOW_CRIT_THRESHOLD = 'low_critical_threshold'
    NOT_AVAILABLE = 'N/A'

    def __init__(self, th_name = NOT_AVAILABLE):
        self.flock = FileLock("{}.lock".format(DEVICE_THRESHOLD_JSON_PATH))
        self.name = th_name
        self.__log = logger.Logger(log_identifier="DeviceThreshold")

        self.__db_data = {}
        self.__db_mtime = 0

    def __reload_db(self):
        try:
            db_data = {}
            with self.flock:
                with open(DEVICE_THRESHOLD_JSON_PATH, "r") as db_file:
                    db_data = json.load(db_file)
        except Exception as e:
            return None

        return db_data

    def __get_data(self, field):
        """
        Retrieves data frome JSON file by field

        Args :
            field: String

        Returns:
            A string if getting is successfully, 'N/A' if not
        """
        if os.path.exists(DEVICE_THRESHOLD_JSON_PATH):
            new_mtime = os.path.getmtime(DEVICE_THRESHOLD_JSON_PATH)
            if new_mtime != self.__db_mtime:
                new_data = self.__reload_db()
                if new_data is not None:
                    self.__db_data = new_data
                    self.__db_mtime = new_mtime

        if self.name not in self.__db_data.keys():
            return self.NOT_AVAILABLE

        if field not in self.__db_data[self.name].keys():
            return self.NOT_AVAILABLE

        return self.__db_data[self.name][field]

    def __set_data(self, field, new_val):
        """
        Set data to JSON file by field

        Args :
            field: String
            new_val: String

        Returns:
            A boolean, True if setting is set successfully, False if not
        """
        if self.name not in self.__db_data.keys():
            self.__db_data[self.name] = {}

        old_val = self.__db_data[self.name].get(field, None)
        if old_val is not None and old_val == new_val:
            return True

        self.__db_data[self.name][field] = new_val

        try:
            with self.flock:
                db_data = {}
                mode = "r+" if os.path.exists(DEVICE_THRESHOLD_JSON_PATH) else "w+"
                with open(DEVICE_THRESHOLD_JSON_PATH, mode) as db_file:
                    if mode == "r+":
                        db_data = json.load(db_file)

                    if self.name not in db_data.keys():
                        db_data[self.name] = {}

                    db_data[self.name][field] = new_val

                    if mode == "r+":
                        db_file.seek(0)
                        # erase old data
                        db_file.truncate(0)
                    # write all data
                    json.dump(db_data, db_file, indent=4)
                self.__db_mtime = os.path.getmtime(DEVICE_THRESHOLD_JSON_PATH)
        except Exception as e:
            self.__log.log_error('{}'.format(str(e)))
            return False

        return True

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature from JSON file.

        Returns:
            string : the high threshold temperature of thermal,
                     e.g. "30.125"
        """
        return self.__get_data(self.HIGH_THRESHOLD)

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A string of temperature, e.g. "30.125"
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        if isinstance(temperature, str) is not True:
            raise TypeError('The parameter requires string type.')

        try:
            if temperature != self.NOT_AVAILABLE:
                float(temperature)
        except ValueError:
            raise ValueError('The parameter requires a float string. ex:\"30.1\"')

        return self.__set_data(self.HIGH_THRESHOLD, temperature)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature from JSON file.

        Returns:
            string : the low threshold temperature of thermal,
                     e.g. "30.125"
        """
        return self.__get_data(self.LOW_THRESHOLD)

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal
        Args :
            temperature: A string of temperature, e.g. "30.125"
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        if isinstance(temperature, str) is not True:
            raise TypeError('The parameter requires string type.')

        try:
            if temperature != self.NOT_AVAILABLE:
                float(temperature)
        except ValueError:
            raise ValueError('The parameter requires a float string. ex:\"30.1\"')

        return self.__set_data(self.LOW_THRESHOLD, temperature)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature from JSON file.

        Returns:
            string : the high critical threshold temperature of thermal,
                     e.g. "30.125"
        """
        return self.__get_data(self.HIGH_CRIT_THRESHOLD)

    def set_high_critical_threshold(self, temperature):
        """
        Sets the high critical threshold temperature of thermal
        Args :
            temperature: A string of temperature, e.g. "30.125"
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        if isinstance(temperature, str) is not True:
            raise TypeError('The parameter requires string type.')

        try:
            if temperature != self.NOT_AVAILABLE:
                float(temperature)
        except ValueError:
            raise ValueError('The parameter requires a float string. ex:\"30.1\"')

        return self.__set_data(self.HIGH_CRIT_THRESHOLD, temperature)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature from JSON file.

        Returns:
            string : the low critical threshold temperature of thermal,
                     e.g. "30.125"
        """
        return self.__get_data(self.LOW_CRIT_THRESHOLD)

    def set_low_critical_threshold(self, temperature):
        """
        Sets the low critical threshold temperature of thermal
        Args :
            temperature: A string of temperature, e.g. "30.125"
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        if isinstance(temperature, str) is not True:
            raise TypeError('The parameter requires string type.')

        try:
            if temperature != self.NOT_AVAILABLE:
                float(temperature)
        except ValueError:
            raise ValueError('The parameter requires a float string. ex:\"30.1\"')

        return self.__set_data(self.LOW_CRIT_THRESHOLD, temperature)
