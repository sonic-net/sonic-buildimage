# Platform-specific SFP transceiver interface for SONiC
#

try:
    import os
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    PLATFORM_PATH = "/sys/devices/platform/"
    SWITCH_BRD_PLATFORM = "switchboard"
    POLL_INTERVAL = 1
    NUM_SFP = 32

    PORT_INFO_DIR = 'SFF'
    PATH_INT_SYSFS = "{0}/{port_name}/{type_prefix}_isr_flags"
    PATH_INTMASK_SYSFS = "{0}/{port_name}/{type_prefix}_isr_mask"
    PATH_PRS_SYSFS = "{0}/{port_name}/{prs_file_name}"
    PRESENT_EN = 0x01

    def __init__(self):
        self._initialize_interrupts()

    def _read_txt_file(self, path):
        with open(path, 'r') as f:
            output = f.readline()
        return output.strip('\n')

    def _write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def _initialize_interrupts(self):
        sfp_info_obj = {}
        port_info_path = os.path.join(
            self.PLATFORM_PATH, self.SWITCH_BRD_PLATFORM, self.PORT_INFO_DIR)

        for index in range(self.NUM_SFP):
            port_num = index + 1
            port_name = "QSFP{}".format(port_num)
            port_type = "qsfp"
            sysfs_prs_file = "{}_modprs".format(port_type)

            sfp_info_obj[index] = {}
            sfp_info_obj[index]['intmask_sysfs'] = \
                self.PATH_INTMASK_SYSFS.format(
                    port_info_path,
                    port_name=port_name,
                    type_prefix=port_type)

            sfp_info_obj[index]['int_sysfs'] = self.PATH_INT_SYSFS.format(
                port_info_path,
                port_name=port_name,
                type_prefix=port_type)

            sfp_info_obj[index]['prs_sysfs'] = self.PATH_PRS_SYSFS.format(
                port_info_path,
                port_name=port_name,
                prs_file_name=sysfs_prs_file)

            self._write_txt_file(
                sfp_info_obj[index]["intmask_sysfs"], hex(self.PRESENT_EN))

        self.sfp_info_obj = sfp_info_obj

    def _is_port_device_present(self, port_idx):
        prs_path = self.sfp_info_obj[port_idx]["prs_sysfs"]
        is_present = 1 - int(self._read_txt_file(prs_path))
        return is_present

    def _update_port_event_object(self, interrup_devices, port_dict):
        for port_idx in interrup_devices:
            device_id = str(port_idx + 1)
            port_dict[device_id] = str(self._is_port_device_present(port_idx))
        return port_dict

    def _clear_event_flag(self, path):
        self._write_txt_file(path, hex(0xff))
        time.sleep(0.1)
        self._write_txt_file(path, hex(0x0))

    def _check_all_port_interrupt_event(self):
        interrupt_devices = {}
        for i in range(self.NUM_SFP):
            int_sysfs = self.sfp_info_obj[i]["int_sysfs"]
            interrupt_flags = self._read_txt_file(int_sysfs)
            if interrupt_flags != '0x00':
                interrupt_devices[i] = 1
                self._clear_event_flag(int_sysfs)
        return interrupt_devices

    def get_event(self, timeout):
        sleep_time = min(timeout, self.POLL_INTERVAL) if \
            timeout != 0 else self.POLL_INTERVAL
        start_t = int(round(time.time() * 1000))
        int_sfp = {}

        while True:
            chk_sfp = self._check_all_port_interrupt_event()
            int_sfp = self._update_port_event_object(
                chk_sfp, int_sfp) if chk_sfp else int_sfp
            current_t = int(round(time.time() * 1000))
            if (int_sfp) or (timeout != 0 and current_t - start_t > timeout):
                break

            time.sleep(sleep_time)

        return True, int_sfp


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 33
    QSFP_PORT_START = 1
    QSFP_PORT_END = 32

    EEPROM_OFFSET = 1
    PORT_INFO_PATH = '/sys/class/seastone2_fpga'

    _port_name = ""
    _port_to_eeprom_mapping = {}
    _port_to_i2cbus_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.QSFP_PORT_START, self.QSFP_PORT_END + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def port_to_i2cbus_mapping(self):
        return self._port_to_i2cbus_mapping

    def get_port_name(self, port_num):
        if port_num in self.qsfp_ports:
            self._port_name = "QSFP" + str(port_num - self.QSFP_PORT_START + 1)
        else:
            self._port_name = "SFP" + str(port_num)
        return self._port_name

    def __init__(self):
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'

        for x in range(self.PORT_START, self.PORT_END+1):
            self.port_to_i2cbus_mapping[x] = (x + self.EEPROM_OFFSET)
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                x + self.EEPROM_OFFSET)
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):

        # Check for invalid port_num
        if port_num not in list(range(self.port_start, self.port_end + 1)):
            return False

        # Get path for access port presence status
        port_name = self.get_port_name(port_num)
        sysfs_filename = "qsfp_modprs"
        reg_path = "/".join([self.PORT_INFO_PATH, port_name, sysfs_filename])

        # Read status
        try:
            reg_file = open(reg_path)
            content = reg_file.readline().rstrip()
            reg_value = int(content)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Module present is active low
        if reg_value == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid QSFP port_num
        if port_num not in self.qsfp_ports:
            return False

        try:
            port_name = self.get_port_name(port_num)
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_lpmode"]),
                "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Read status
        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        # ModPrsL is active low
        if reg_value == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid QSFP port_num
        if port_num not in self.qsfp_ports:
            return False

        try:
            port_name = self.get_port_name(port_num)
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_lpmode"]),
                "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = hex(lpmode)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid QSFP port_num
        if port_num not in self.qsfp_ports:
            return False

        try:
            port_name = self.get_port_name(port_num)
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_reset"]), "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(0))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to unset reset
        try:
            reg_file = open(
                "/".join([self.PORT_INFO_PATH, port_name, "qsfp_reset"]), "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_file.write(hex(1))
        reg_file.close()

        return True

    def get_transceiver_change_event(self, timeout=0):
        """
         The method is a blocking call.
         When timeout is zero, it only returns when there is change event,
         i.e., transceiver plug-in/out event.When timeout is non-zero,
         the function can also return when the timer expires.
         When timer expires, the return status is True and events is empty.

        :param timeout in milliseconds.
        :returns: (status, events)
        :status: Boolean,
         True if call successful and no system level event/error occurred,
         False if call not success or system level event/error occurred.
        :events: dictionary for physical port index and the SFP status,
         status='1' represent plug in,
                '0' represent plug out like {'0': '1', '31':'0'}
         when it comes to system level event/error, the index will be '-1',
         and status can be 'system_not_ready', 'system_become_ready',
         'system_fail', like {'-1':'system_not_ready'}.
        """
        event_class = SfpEvent()
        return event_class.get_event(timeout)
