# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import copy
    import subprocess
    import binascii
    import re
    import os
    import sys
    import importlib
    import threading
    import traceback
    from ctypes import create_string_buffer
    from math import log10
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from sonic_sfp.inf8628 import inf8628InterfaceId
    from sonic_sfp.qsfp_dd import qsfp_dd_InterfaceId
    from portutil.baseutil import get_port_bus_map
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    port_list = []

    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    OSFP_DEVICE_TYPE = "optoe3"
    I2C_MAX_ATTEMPT = 3

    SFP_STATUS_INSERTED = '1'
    SFP_STATUS_REMOVED = '0'

    OPTOE_TYPE1 = 1
    OPTOE_TYPE2 = 2
    OPTOE_TYPE3 = 3

    QSFP_POWERMODE_OFFSET = 93
    QSFP_CONTROL_OFFSET = 86
    QSFP_CONTROL_WIDTH = 8

    port_dict = {}
    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping = {}

    qsfp_ports_list = []
    osfp_ports_list = []

    @property
    def port_start(self):
        if len(self.port_list) > 0:
            return min(self.port_list)
        else:
            return 0

    @property
    def port_end(self):
        if len(self.port_list) > 0:
            return max(self.port_list)
        else:
            return 0

    @property
    def qsfp_ports(self):
        return self.qsfp_ports_list

    @property
    def osfp_ports(self):
        return self.osfp_ports_list

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def get_port_list(self):
        return copy.deepcopy(self.port_list)

    def __init_port_bus_map(self):
        try:
            ret, msg = get_port_bus_map()
            if ret:
                for port_index, i2c_bus in msg.items():
                    self.port_list.append(int(port_index))
                    self.port_to_i2cbus_mapping[int(port_index)] = int(i2c_bus)
            else:
                print(msg)
        except Exception as e:
            print(traceback.format_exc())

    def __init__(self):
        self.__init_port_bus_map()
        self.update_ports_list()
        SfpUtilBase.__init__(self)

    def _sfp_read_file_path(self, file_path, offset, num_bytes):
        attempts = 0
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                file_path.seek(offset)
                read_buf = file_path.read(num_bytes)
            except BaseException:
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
        sysfs_i2c_adapter_base_path = "/sys/class/i2c-adapter"

        if port_num in self.port_to_eeprom_mapping.keys():
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[port_num]
        else:
            sysfs_i2c_adapter_base_path = "/sys/class/i2c-adapter"

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
                if port_num in self.osfp_ports:
                    ret = self._add_new_sfp_device(
                        sysfs_sfp_i2c_adapter_path, devid, self.OSFP_DEVICE_TYPE)
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
        except BaseException:
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

        cmd = "cat /sys/s3ip/transceiver/eth{}/present".format(str(port_num))
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
        self.osfp_ports_list = []
        for port in self.port_list:
            if (self.check_is_qsfpdd(port)):
                self.osfp_ports_list.append(port)
            else:
                self.qsfp_ports_list.append(port)

    def set_power_override(self, port_num, power_override, power_set):
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if self.get_presence(port_num) is False:
            return False
        if port_num in self.osfp_ports:
            return False
        elif port_num in self.qsfp_ports:
            offset = 0
            try:
                power_override_bit = 0
                if power_override:
                    power_override_bit |= 1 << 0

                power_set_bit = 0
                if power_set:
                    power_set_bit |= 1 << 1

                buffer = create_string_buffer(1)
                buffer[0] = chr(power_override_bit | power_set_bit)
                # Write to eeprom
                sysfs_sfp_i2c_client_eeprom_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
                with open(sysfs_sfp_i2c_client_eeprom_path, "r+b") as sysfsfile_eeprom:
                    sysfsfile_eeprom.seek(offset + self.QSFP_POWERMODE_OFFSET)
                    sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            return True
        else:
            # SFP doesn't support this feature
            return False
        return False

    def get_low_power_mode(self, port_num):
        """
        Not support LPMode pin to control lpmde.
        This function is affected by the  Power_over-ride and Power_set software control bits (byte 93 bits 0,1)
        """
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num in self.osfp_ports:
            return False
        elif port_num in self.qsfp_ports:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False
            sysfs_sfp_i2c_client_eeprom_path = self._get_port_eeprom_path(port_num, self.IDENTITY_EEPROM_ADDR)
            with open(sysfs_sfp_i2c_client_eeprom_path, "rb", buffering=0) as sysfsfile:
                dom_control_raw = self._read_eeprom_specific_bytes(sysfsfile,
                                                                   offset + self.QSFP_CONTROL_OFFSET, self.QSFP_CONTROL_WIDTH) if self.get_presence(port_num) else None
            if dom_control_raw is not None:
                dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
                lpmode = ('On' == dom_control_data['data']['PowerSet']['value'])
                power_override = ('On' == dom_control_data['data']['PowerOverride']['value'])
                if lpmode == power_override == True:
                    return True
        else:
            # SFP doesn't support this feature
            return False
        return False

    def set_low_power_mode(self, port_num, lpmode):
        """
        Not support LPMode pin to control lpmde.
        This function is affected by the  Power_over-ride and Power_set software control bits (byte 93 bits 0,1)
        """
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if lpmode:
            return self.set_power_override(port_num, True, lpmode)
        else:
            return self.set_power_override(port_num, False, lpmode)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        if port_num in self.osfp_ports or port_num in self.qsfp_ports:
            reset_path = "/sys/s3ip/transceiver/eth{}/reset".format(str(port_num))
            cmd = "echo 1 > " + reset_path
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                print("cmd: %s execution fail, output:%s" % (cmd, output))
                return False

            time.sleep(1)

            cmd = "echo 0 > " + reset_path
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                print("cmd: %s execution fail, output:%s" % (cmd, output))
                return False

            return True
        else:
            # SFP doesn't support this feature
            return False
        return False

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()
        currernt_port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:'
                  'time wrap / invalid timeout value', timeout)

            return False, {}  # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            for x in self.port_list:
                if self.get_presence(x):
                    currernt_port_dict[x] = self.SFP_STATUS_INSERTED
                else:
                    currernt_port_dict[x] = self.SFP_STATUS_REMOVED
            if (currernt_port_dict == self.port_dict):
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - time.time()
                    if timeout >= 1:
                        time.sleep(1)  # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        self.update_ports_list()
                        return True, {}
            else:
                self.update_ports_list()
                # Update reg value
                self.port_dict = currernt_port_dict
                return True, self.port_dict
        print("get_transceiver_change_event: Should not reach here.")
        return False, {}

    # Convert Hex to String
    def convert_hex_to_string(self, arr, start, end):
        try:
            ret_str = ''
            for n in range(start, end):
                ret_str += arr[n]
            return binascii.unhexlify(ret_str).decode("utf-8").strip().strip(b'\x00'.decode())
        except Exception as err:
            return str(err)

    # To check the sfp is rj loopback or not
    def check_is_rj_loopback(self, port_num):
        offset = 129
        size = 16
        try:
            if self.get_presence(port_num) is False:
                return False

            eeprom_path = self._get_port_eeprom_path(port_num, 0x50)
            with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, size)

                if eeprom_raw is None:
                    return False
                target = ["52", "75", "69", "6a", "69", "65"]
                if ''.join(target) in ''.join(eeprom_raw):
                    return True
        except Exception as e:
            pass

            return False

    # A for loop to check if there are any rj loopback modules in the switch
    def check_any_rj_loopback(self):
        is_rjlb_flag = False
        for port in self.port_list:
            if self.get_presence(port) == False:
                continue
            if port in self.osfp_ports:
                if self.check_is_rj_loopback(port):
                    is_rjlb_flag = True
                    break
        return is_rjlb_flag

    # To get the sfp's temperature
    def get_sfp_temperature(self, port):
        offset = 0
        sfp_temperature = -9999

        presence_flag = False
        read_eeprom_flag = False
        temperature_valid_flag = False

        if self.get_presence(port) == False:
            presence_flag = False
        else:
            presence_flag = True

        try:
            eeprom_path = self._get_port_eeprom_path(port, 0x50)
            with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                eeprom_raw = self._read_eeprom_specific_bytes(eeprom, 0, 16)
                if eeprom_raw is not None:
                    presence_flag = True
                    read_eeprom_flag = True
                    if (eeprom_raw[0] == '1e' or eeprom_raw[0] == '18' or eeprom_raw[0] == '19'):
                        msb = int(eeprom_raw[14], 16)
                        lsb = int(eeprom_raw[15], 16)

                        result = (msb << 8) | (lsb & 0xff)
                        result = float(result / 256.0)
                        if -50 <= result <= 200:
                            temperature_valid_flag = True
                            sfp_temperature = result
        except Exception as e:
            # print(traceback.format_exc())
            pass

        # port not presence
        if presence_flag == False:
            sfp_temperature = -10000

        # read port eeprom fail
        elif read_eeprom_flag == False:
            sfp_temperature = -9999

        # port temperature invalid
        elif read_eeprom_flag == True and temperature_valid_flag == False:
            sfp_temperature = -10000

        sfp_temperature = round(sfp_temperature, 2)
        return sfp_temperature

    def get_highest_temperature(self):
        offset = 0
        hightest_temperature = -9999

        presence_flag = False
        read_eeprom_flag = False
        temperature_valid_flag = False

        for port in self.port_list:
            if self.get_presence(port) == False:
                continue

            presence_flag = True

            if port in self.osfp_ports:
                offset = 14
            elif port in self.qsfp_ports:
                offset = 22
            else:
                offset = 256 + 96

            eeprom_path = self._get_port_eeprom_path(port, 0x50)
            try:
                with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                    read_eeprom_flag = True
                    eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, 2)
                    msb = int(eeprom_raw[0], 16)
                    lsb = int(eeprom_raw[1], 16)

                    result = (msb << 8) | (lsb & 0xff)
                    result = float(result / 256.0)
                    if -50 <= result <= 200:
                        temperature_valid_flag = True
                        if hightest_temperature < result:
                            hightest_temperature = result
            except Exception as e:
                # print(traceback.format_exc())
                pass

        # all port not presence
        if presence_flag == False:
            hightest_temperature = -10000

        # all port read eeprom fail
        elif read_eeprom_flag == False:
            hightest_temperature = -9999

        # all port temperature invalid
        elif read_eeprom_flag == True and temperature_valid_flag == False:
            hightest_temperature = -10000

        hightest_temperature = round(hightest_temperature, 2)
        return hightest_temperature

    def _twos_comp(self, num, bits):
        try:
            if ((num & (1 << (bits - 1))) != 0):
                num = num - (1 << bits)
            return num
        except BaseException:
            return 0

    def _calc_temperature(self, data_raw):
        retval = 'N/A'
        try:
            msb = int(data_raw[0], 16)
            lsb = int(data_raw[1], 16)
            result = (msb << 8) | (lsb & 0xff)
            result = self._twos_comp(result, 16)

            result = float(result / 256.0)
            retval = '%.4f' % result + 'C'
        except Exception as err:
            print(str(err))

        return retval

    def _calc_voltage(self, data_raw):
        retval = 'N/A'
        try:
            msb = int(data_raw[0], 16)
            lsb = int(data_raw[1], 16)
            result = (msb << 8) | (lsb & 0xff)

            result = float(result * 0.0001)
            retval = '%.4f' % result + 'Volts'
        except Exception as err:
            print(str(err))

        return retval

    def _check_is_flat_mem(self, port_num):
        sfpi_obj = qsfp_dd_InterfaceId()
        eeprom_path = self._get_port_eeprom_path(port_num, 0x50)
        with open(eeprom_path, mode="rb", buffering=0) as eeprom:
            eeprom_raw = self._read_eeprom_specific_bytes(eeprom, 2, 1)
        qsfp_dd_flat_memory = sfpi_obj.parse_dom_capability(eeprom_raw, 0)
        if qsfp_dd_flat_memory['data']['Flat_MEM']['value'] == 'Off':
            return False
        return True

    def _mw_to_dbm(self, mW):
        if mW == 0:
            return float("-inf")
        elif mW < 0:
            return float("NaN")
        return 10. * log10(mW)

    def _calc_power(self, data_raw):
        retval = 'N/A'
        try:
            msb = int(data_raw[0], 16)
            lsb = int(data_raw[1], 16)
            result = (msb << 8) | (lsb & 0xff)

            result = float(result * 0.0001)
            retval = "%.4f%s" % (self._mw_to_dbm(result), "dBm")
        except Exception as err:
            print(str(err))

        return retval

    def _calc_bias(self, data_raw):
        retval = 'N/A'
        try:
            msb = int(data_raw[0], 16)
            lsb = int(data_raw[1], 16)
            result = (msb << 8) | (lsb & 0xff)

            result = float(result * 0.002)
            retval = '%.4f' % result + 'mA'
        except Exception as err:
            print(str(err))

        return retval

    def _parse_lane_specific_flag(self, eeprom_data):
        if len(eeprom_data) == 0:
            return 'N/A'
        return '0x{:02x}'.format(int(eeprom_data[0], 16))

    def get_eeprom_dict(self, port_num):
        """Returns dictionary of interface and dom data.
        format: {<port_num> : {'interface': {'version' : '1.0', 'data' : {...}},
                            'dom' : {'version' : '1.0', 'data' : {...}}}}
        """

        sfp_data = {}

        eeprom_ifraw = self.get_eeprom_raw(port_num)
        if eeprom_ifraw is None:
            return None

        if not self.check_is_qsfpdd(port_num):   # QSFP
            return SfpUtilBase.get_eeprom_dict(self, port_num)

        sfpi_obj = inf8628InterfaceId(eeprom_ifraw)
        if sfpi_obj is not None:
            sfp_data['interface'] = sfpi_obj.get_data_pretty()

        sfp_data['dom'] = {
            'data': {
                'ModuleMonitorValues': {
                    'Vcc': '0.0Volts',
                    'Temperature': '0.0C',
                },
                'ChannelMonitorValues': {
                    'TX1Bias': 'N/A',
                    'TX2Bias': 'N/A',
                    'TX3Bias': 'N/A',
                    'TX4Bias': 'N/A',
                    'TX5Bias': 'N/A',
                    'TX6Bias': 'N/A',
                    'TX7Bias': 'N/A',
                    'TX8Bias': 'N/A',
                    'RX1Power': 'N/A',
                    'RX2Power': 'N/A',
                    'RX3Power': 'N/A',
                    'RX4Power': 'N/A',
                    'RX5Power': 'N/A',
                    'RX6Power': 'N/A',
                    'RX7Power': 'N/A',
                    'RX8Power': 'N/A',
                    'TX1Power': 'N/A',
                    'TX2Power': 'N/A',
                    'TX3Power': 'N/A',
                    'TX4Power': 'N/A',
                    'TX5Power': 'N/A',
                    'TX6Power': 'N/A',
                    'TX7Power': 'N/A',
                    'TX8Power': 'N/A',
                },
                'ModuleThresholdValues': {
                    'TempHighAlarm': 'N/A',
                    'TempLowAlarm': 'N/A',
                    'TempHighWarning': 'N/A',
                    'TempLowWarning': 'N/A',
                    'VccHighAlarm': 'N/A',
                    'VccLowAlarm': 'N/A',
                    'VccHighWarning': 'N/A',
                    'VccLowWarning': 'N/A',
                    'TxBiasHighAlarm': 'N/A',
                    'TxBiasLowAlarm': 'N/A',
                    'TxBiasHighWarning': 'N/A',
                    'TxBiasLowWarning': 'N/A',
                    'TxPowerHighAlarm': 'N/A',
                    'TxPowerLowAlarm': 'N/A',
                    'TxPowerHighWarning': 'N/A',
                    'TxPowerLowWarning': 'N/A',
                    'RxPowerHighAlarm': 'N/A',
                    'RxPowerLowAlarm': 'N/A',
                    'RxPowerHighWarning': 'N/A',
                    'RxPowerLowWarning': 'N/A',
                }
            }
        }
        temp_data_raw = eeprom_ifraw[14: 16]
        vcc_data_raw = eeprom_ifraw[16: 18]
        tmp = self._calc_temperature(temp_data_raw)
        vcc = self._calc_voltage(vcc_data_raw)

        sfp_data['dom']['data']['ModuleMonitorValues']['Vcc'] = vcc
        sfp_data['dom']['data']['ModuleMonitorValues']['Temperature'] = tmp

        if self.check_is_rj_loopback(port_num) or self._check_is_flat_mem(port_num) is True:
            sfp_data['dom']['data']['ChannelMonitorValues'] = 'N/A'
            sfp_data['dom']['data']['ModuleThresholdValues'] = 'N/A'

            return sfp_data

        page = 0x11
        offset = (page - 1) * 128 + 256  # page0 has 256 byte, other page has 128 byte
        # page 0x0 128byte + page 0x11 128byte
        tmp_page11_raw = self._read_eeprom_devid(port_num, self.IDENTITY_EEPROM_ADDR, offset, 128)

        # No corresponding information for 400G copper cable
        if tmp_page11_raw is None:
            return sfp_data

        new_eeprom_ifraw = eeprom_ifraw[0: 128] + tmp_page11_raw

        TXPower = new_eeprom_ifraw[154: 170]
        TXBias = new_eeprom_ifraw[170: 186]
        RXPower = new_eeprom_ifraw[186: 202]
        size = 2

        CMV = sfp_data['dom']['data']['ChannelMonitorValues']
        lane = 1
        for i in range(0, len(TXPower), size):
            key = ('TX%dPower' % lane)
            power = self._calc_power(TXPower[i: (i + size)])
            CMV[key] = power
            lane += 1
        lane = 1
        for i in range(0, len(TXBias), size):
            key = ('TX%dBias' % lane)
            Bias = self._calc_bias(TXBias[i: (i + size)])
            CMV[key] = Bias
            lane += 1
        lane = 1
        for i in range(0, len(RXPower), size):
            key = ('RX%dPower' % lane)
            power = self._calc_power(RXPower[i: (i + size)])
            CMV[key] = power
            lane += 1

        page = 0x02
        offset = (page - 1) * 128 + 256
        tmp_page2_raw = self._read_eeprom_devid(port_num, self.IDENTITY_EEPROM_ADDR, offset, 128)
        if tmp_page2_raw is None:
            return sfp_data

        new_eeprom_ifraw = eeprom_ifraw[0: 128] + tmp_page2_raw

        temp_high_alarm = self._calc_temperature(new_eeprom_ifraw[128: 130])
        temp_low_alarm = self._calc_temperature(new_eeprom_ifraw[130: 132])
        temp_high_warning = self._calc_temperature(new_eeprom_ifraw[132: 134])
        temp_low_warning = self._calc_temperature(new_eeprom_ifraw[134: 136])
        vcc_high_alarm = self._calc_voltage(new_eeprom_ifraw[136: 138])
        vcc_low_alarm = self._calc_voltage(new_eeprom_ifraw[138: 140])
        vcc_high_warning = self._calc_voltage(new_eeprom_ifraw[140: 142])
        vcc_low_warning = self._calc_voltage(new_eeprom_ifraw[142: 144])

        sfp_data['dom']['data']['ModuleThresholdValues']['TempHighAlarm'] = temp_high_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TempHighWarning'] = temp_high_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['TempLowAlarm'] = temp_low_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TempLowWarning'] = temp_low_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['VccHighAlarm'] = vcc_high_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['VccHighWarning'] = vcc_high_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['VccLowAlarm'] = vcc_low_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['VccLowWarning'] = vcc_low_warning

        tx_power_high_alarm = self._calc_power(new_eeprom_ifraw[176: 178])
        tx_power_low_alarm = self._calc_power(new_eeprom_ifraw[178: 180])
        tx_power_high_warning = self._calc_power(new_eeprom_ifraw[180: 182])
        tx_power_low_warning = self._calc_power(new_eeprom_ifraw[182: 184])
        tx_bias_high_alarm = self._calc_bias(new_eeprom_ifraw[184: 186])
        tx_bias_low_alarm = self._calc_bias(new_eeprom_ifraw[186: 188])
        tx_bias_high_warning = self._calc_bias(new_eeprom_ifraw[188: 190])
        tx_bias_low_warning = self._calc_bias(new_eeprom_ifraw[190: 192])
        rx_power_high_alarm = self._calc_power(new_eeprom_ifraw[192: 194])
        rx_power_low_alarm = self._calc_power(new_eeprom_ifraw[194: 196])
        rx_power_high_warning = self._calc_power(new_eeprom_ifraw[196: 198])
        rx_power_low_warning = self._calc_power(new_eeprom_ifraw[198: 200])

        sfp_data['dom']['data']['ModuleThresholdValues']['TxPowerHighAlarm'] = tx_power_high_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TxPowerLowAlarm'] = tx_power_low_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TxPowerHighWarning'] = tx_power_high_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['TxPowerLowWarning'] = tx_power_low_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['TxBiasHighAlarm'] = tx_bias_high_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TxBiasLowAlarm'] = tx_bias_low_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['TxBiasHighWarning'] = tx_bias_high_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['TxBiasLowWarning'] = tx_bias_low_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['RxPowerHighAlarm'] = rx_power_high_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['RxPowerLowAlarm'] = rx_power_low_alarm
        sfp_data['dom']['data']['ModuleThresholdValues']['RxPowerHighWarning'] = rx_power_high_warning
        sfp_data['dom']['data']['ModuleThresholdValues']['RxPowerLowWarning'] = rx_power_low_warning

        return sfp_data
