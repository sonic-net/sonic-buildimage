# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 55
    PORTS_IN_BLOCK = 56

    EEPROM_OFFSET = 11
    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    I2C_MAX_ATTEMPT = 3

    SFP_STATUS_INSERTED = '1'
    SFP_STATUS_REMOVED = '0'

    QSFP_POWERMODE_OFFSET = 93
    QSFP_CONTROL_OFFSET = 86
    QSFP_CONTROL_WIDTH = 8

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping ={}
    port_dict = {} 

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(48, self.PORTS_IN_BLOCK)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
            self.port_to_i2cbus_mapping[x] = (x + self.EEPROM_OFFSET)
            if self.get_presence(x):
                self.port_dict[x] = self.SFP_STATUS_INSERTED
            else:
                self.port_dict[x] = self.SFP_STATUS_REMOVED
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
                if port_num in self.qsfp_ports:
                    self._add_new_sfp_device(
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
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
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

        if port_num <= 7:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence1"
        elif port_num >= 8 and port_num <= 15:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence2"
        elif port_num >= 16 and port_num <= 23:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence3"
        elif port_num >= 24 and port_num <= 31:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence4"
        elif port_num >= 32 and port_num <= 39:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence5"
        elif port_num >= 40 and port_num <= 47:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence6"
        elif port_num >= 48 and port_num <= 55:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence7"
        else:
            return False

        try:
            data = open(presence_path, "rb")
        except IOError:
            return False

        presence_data = data.read(2)
        if presence_data == "":
            return False
        result = int(presence_data, 16)
        data.close()

        # ModPrsL is active low
        if result & (1 << (port_num % 8)) == 0:
            return True

        return False

    def set_power_override(self, port_num, power_override, power_set):
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if self.get_presence(port_num) is False:
            return False
        if port_num in self.qsfp_ports:
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
                sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "r+b")
                sysfsfile_eeprom.seek(offset + self.QSFP_POWERMODE_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
        else:
            # SFP doesn't support this feature
            return False

    def get_low_power_mode(self, port_num):
        """
        Not support LPMode pin to control lpmde.
        This function is affected by the  Power_over-ride and Power_set software control bits (byte 93 bits 0,1)
        """
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num in self.qsfp_ports:
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
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num in self.qsfp_ports:
            reset_path = "/sys/bus/i2c/devices/1-0036/qsfp_reset"
            try:
                data = open(reset_path, "r+")
                reset_data = data.read(2)
                if reset_data == "":
                    return False
                result = int(reset_data, 16)
                result = result & (~(1 << (port_num % 8)))
                data.seek(0)
                sres = hex(result)[2:]
                data.write(sres)
                data.close()

                time.sleep(1)

                data = open(reset_path, "r+")
                reset_data = data.read(2)
                if reset_data == "":
                    return False
                result = int(reset_data, 16)
                data.seek(0)
                result = result | (1 << (port_num % 8))
                sres = hex(result)[2:]
                data.write(sres)
                data.close()
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            return True
        else:
            # SFP doesn't support this feature
            return False

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        currernt_port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print("get_transceiver_change_event: time wrap / invalid timeout value", timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
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
                        time.sleep(1) # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        return True, {}
            else:
                # Update reg value
                self.port_dict = currernt_port_dict
                return True, self.port_dict
        print("get_transceiver_change_event: Should not reach here.")
        return False, {}
