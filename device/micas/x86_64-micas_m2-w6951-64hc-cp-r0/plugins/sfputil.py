# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    import traceback
    import subprocess
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 65
    PORTS_IN_BLOCK = 66

    EEPROM_OFFSET = 201
    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    QSFP_DD_DEVICE_TYPE = "optoe3"
    I2C_MAX_ATTEMPT = 3

    OPTOE_TYPE1 = 1
    OPTOE_TYPE2 = 2
    OPTOE_TYPE3 = 3

    SFP_STATUS_INSERTED = '1'
    SFP_STATUS_REMOVED = '0'

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping ={}
    port_dict = {}

    qsfp_ports_list = []
    qsfp_dd_ports_list = []

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return self.qsfp_ports_list

    @property
    def qsfp_dd_ports(self):
        return self.qsfp_dd_ports_list

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK-2):
            remainder = x % 2
            remain = (x)//2
            group = remainder + remain
            if x in [2,3,6,7,10,11,14,15,18,19,22,23,26,27,30,31,34,35,38,39,42,43,46,47,50,51,54,55,58,59,62,63]:
                self.port_to_i2cbus_mapping[x] = 200 + group
            elif x in [0,1,4,5,8,9,12,13,16,17,20,21,24,25,28,29,32,33,36,37,40,41,44,45,48,49,52,53,56,57,60,61]:
                self.port_to_i2cbus_mapping[x] = 233 + group
        self.port_to_i2cbus_mapping[64] = 265
        self.port_to_i2cbus_mapping[65] = 265
        self.update_ports_list()

        SfpUtilBase.__init__(self)

    def _sfp_read_file_path(self, file_path, offset, num_bytes):
        attempts = 0
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                file_path.seek(offset)
                read_buf = file_path.read(num_bytes)
            except:
                attempts += 1
                time.sleep(0.05)
            else:
                return True, read_buf
        return False, None

    def _sfp_eeprom_present(self, sysfs_sfp_i2c_client_eeprompath, offset):
        """Tries to read the eeprom file to determine if the
        device/sfp is present or not. If sfp present, the read returns
        valid bytes. If not, read returns error 'Connection timed out"""

        if not os.path.exists(sysfs_sfp_i2c_client_eeprompath):
            return False
        else:
            with open(sysfs_sfp_i2c_client_eeprompath, "rb", buffering=0) as sysfsfile:
                rv, buf = self._sfp_read_file_path(sysfsfile, offset, 1)
                return rv

    def _add_new_sfp_device(self, sysfs_sfp_i2c_adapter_path, devaddr, devtype):
        try:
            sysfs_nd_path = "%s/new_device" % sysfs_sfp_i2c_adapter_path

            # Write device address to new_device file
            nd_file = open(sysfs_nd_path, "w")
            nd_str = "%s %s" % (devtype, hex(devaddr))
            nd_file.write(nd_str)
            nd_file.close()

        except Exception as err:
            print("Error writing to new device file: %s" % str(err))
            return 1
        else:
            return 0

    def _get_port_eeprom_path(self, port_num, devid):
        sysfs_i2c_adapter_base_path = "/sys/bus/i2c/devices"

        if port_num in self.port_to_eeprom_mapping.keys():
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[port_num]
        else:
            sysfs_i2c_adapter_base_path = "/sys/bus/i2c/devices"

            i2c_adapter_id = self._get_port_i2c_adapter_id(port_num)
            if i2c_adapter_id is None:
                print("Error getting i2c bus num")
                return None

            # Get i2c virtual bus path for the sfp
            sysfs_sfp_i2c_adapter_path = "%s/i2c-%s" % (sysfs_i2c_adapter_base_path,
                                                        str(i2c_adapter_id))

            # If i2c bus for port does not exist
            if not os.path.exists(sysfs_sfp_i2c_adapter_path):
                print("Could not find i2c bus %s. Driver not loaded?" % sysfs_sfp_i2c_adapter_path)
                return None

            sysfs_sfp_i2c_client_path = "%s/%s-00%s" % (sysfs_sfp_i2c_adapter_path,
                                                        str(i2c_adapter_id),
                                                        hex(devid)[-2:])

            # If sfp device is not present on bus, Add it
            if not os.path.exists(sysfs_sfp_i2c_client_path):
                if port_num in self.qsfp_dd_ports:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.QSFP_DD_DEVICE_TYPE)
                elif port_num in self.qsfp_ports:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.QSFP_DEVICE_TYPE)
                else:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.SFP_DEVICE_TYPE)
                if ret != 0:
                    print("Error adding sfp device")
                    return None

            sysfs_sfp_i2c_client_eeprom_path = "%s/eeprom" % sysfs_sfp_i2c_client_path

        return sysfs_sfp_i2c_client_eeprom_path

    def _read_eeprom_specific_bytes(self, sysfsfile_eeprom, offset, num_bytes):
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        rv, raw = self._sfp_read_file_path(sysfsfile_eeprom, offset, num_bytes)
        if rv == False:
            return None

        try:
            if len(raw) == 0:
                return None
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except:
            return None

        return eeprom_raw

    def get_eeprom_dom_raw(self, port_num):
        if port_num in self.qsfp_ports:
            # QSFP DOM EEPROM is also at addr 0x50 and thus also stored in eeprom_ifraw
            return None
        else:
            # Read dom eeprom at addr 0x51
            return self._read_eeprom_devid(port_num, self.IDENTITY_EEPROM_ADDR, 256)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        cmd = "cat /sys/s3ip/transceiver/eth{}/present".format(str(port_num + 1))
        ret, output = subprocess.getstatusoutput(cmd)
        if ret != 0:
            return False
        if output == "1":
            return True
        return False

    def check_is_qsfpdd(self, port_num):
        try:
            if self.get_presence(port_num) == False:
                return False

            eeprom_path = self._get_port_eeprom_path(port_num, 0x50)
            with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                eeprom_raw = self._read_eeprom_specific_bytes(eeprom, 0, 1)
                if eeprom_raw is None:
                    return False
                # according to sff-8024 A0h Byte 0 is '1e','18' or '19' means the transceiver is qsfpdd,
                if (eeprom_raw[0] == '1e' or eeprom_raw[0] == '18' or eeprom_raw[0] == '19'):
                    return True
        except Exception as e:
            print(traceback.format_exc())

        return False

    def check_optoe_type(self, port_num, optoe_type):
        if self.get_presence(port_num) == False:
            return True
        try:
            eeprom_path = self._get_port_eeprom_path(port_num, 0x50)
            dev_class_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/dev_class'
            i2c_path = dev_class_path.format(str(self.port_to_i2cbus_mapping[port_num]))
            cmd = "cat " + i2c_path
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                print("cmd: %s execution fail, output:%s" % (cmd, output))
                return False
            if int(output) != optoe_type:
                cmd = "echo " + str(optoe_type) + " > " + i2c_path
                ret, output = subprocess.getstatusoutput(cmd)
                if ret != 0:
                    print("cmd: %s execution fail, output:%s" % (cmd, output))
                    return False
            return True

        except Exception as e:
            print(traceback.format_exc())
            return False

    def update_ports_list(self):
        self.qsfp_ports_list = []
        self.qsfp_dd_ports_list = []
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
            if (self.check_is_qsfpdd(x)):
                self.qsfp_dd_ports_list.append(x)
            else:
                self.qsfp_ports_list.append(x)

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return True

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        current_port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print ("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print ('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
                if self.get_presence(x):
                    current_port_dict[x] = self.SFP_STATUS_INSERTED
                else:
                    current_port_dict[x] = self.SFP_STATUS_REMOVED
            if (current_port_dict == self.port_dict):
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - time.time()
                    if timeout >= 1:
                        time.sleep(1) # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        self.update_ports_list()
                        return True, {}
            else:
                # Update reg value
                self.update_ports_list()
                self.port_dict = current_port_dict
                return True, self.port_dict
        print ("get_transceiver_change_event: Should not reach here.")
        return False, {}

    def _twos_comp(self, num, bits):
        try:
            if ((num & (1 << (bits - 1))) != 0):
                num = num - (1 << bits)
            return num
        except:
            return 0

    def get_highest_temperature(self):
        with open("/tmp/sfp_temp_debug.log", "a") as f:
            f.write(f"{time.strftime('%F %T')} enter get_highest_temperature\n")

        highest_temperature = -9999

        presence_flag = False
        read_eeprom_flag = False
        temperature_supported_flag = False
        temperature_valid_flag = False

        for port in range(self.PORT_START, self.PORT_END + 1):
            if port == 65:
                continue
            try:
                present = self.get_presence(port)
                with open("/tmp/sfp_temp_debug.log", "a") as f:
                    f.write(f"port={port} present={present}\n")

                if not present:
                    continue

                presence_flag = True

                eeprom_path = self._get_port_eeprom_path(port, 0x50)
                with open("/tmp/sfp_temp_debug.log", "a") as f:
                    f.write(f"port={port} eeprom_path={eeprom_path}\n")

                try:
                    with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                        read_eeprom_flag = True

                        if port in self.qsfp_dd_ports:
                            offset = 14
                            temperature_supported_flag = True

                        elif port in self.qsfp_ports:
                            offset = 22
                            temperature_supported_flag = True

                        else:
                            # SFP/SFP28: check A0h byte 92 bit6 before reading A2h temperature.
                            # bit6 = 1 means Digital Diagnostic Monitoring is implemented.
                            ddm_raw = self._read_eeprom_specific_bytes(eeprom, 92, 1)
                            with open("/tmp/sfp_temp_debug.log", "a") as f:
                                f.write(f"port={port} DDM support byte={ddm_raw}\n")

                            if ddm_raw is None or len(ddm_raw) != 1:
                                return 9999

                            ddm_type = int(ddm_raw[0], 16)
                            if (ddm_type & 0x40) == 0:
                                # DDM is not supported by this SFP/SFP28 module.
                                # Skip this port instead of treating it as a temperature fault.
                                with open("/tmp/sfp_temp_debug.log", "a") as f:
                                    f.write(f"port={port} DDM not supported, skip temperature\n")
                                continue

                            offset = 256 + 96
                            temperature_supported_flag = True

                        with open("/tmp/sfp_temp_debug.log", "a") as f:
                            f.write(f"port={port} temp_offset={offset}\n")

                        eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, 2)
                        with open("/tmp/sfp_temp_debug.log", "a") as f:
                            f.write(f"port={port} eeprom_raw={eeprom_raw}\n")

                        if eeprom_raw is None or len(eeprom_raw) != 2:
                            return 9999

                        msb = int(eeprom_raw[0], 16)
                        lsb = int(eeprom_raw[1], 16)

                        result = (msb << 8) | (lsb & 0xff)
                        result = self._twos_comp(result, 16)
                        result = float(result / 256.0)

                    with open("/tmp/sfp_temp_debug.log", "a") as f:
                        f.write(f"port={port} temp={result}\n")

                    if -50 <= result <= 200:
                        temperature_valid_flag = True
                        highest_temperature = max(highest_temperature, result)
                    else:
                        return 8000

                except Exception as e:
                    with open("/tmp/sfp_temp_debug.log", "a") as f:
                        f.write(f"port={port} inner_exception={repr(e)}\n")
                    continue

            except Exception as e:
                with open("/tmp/sfp_temp_debug.log", "a") as f:
                    f.write(f"port={port} outer_exception={repr(e)}\n")
                continue

         # all port not presence
        if not presence_flag:
            highest_temperature = -10000
        elif not read_eeprom_flag:
            # At least one transceiver is present, but all EEPROM reads failed.
            highest_temperature = 9998
        elif not temperature_supported_flag:
            # EEPROM is readable, but no present transceiver supports temperature monitoring.
            highest_temperature = -20000
        elif not temperature_valid_flag:
            # Temperature monitoring is supported, but no valid temperature was read.
            highest_temperature = 8000

        highest_temperature = round(highest_temperature, 2)

        with open("/tmp/sfp_temp_debug.log", "a") as f:
            f.write(f"{time.strftime('%F %T')} final highest={highest_temperature}\n")

        return highest_temperature
