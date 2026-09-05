# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    import re
    import socket
    from sonic_sfp.sfputilbase import SfpUtilBase
    from collections import OrderedDict
    from sonic_sfp.sff8472 import sff8472Dom
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VLOT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6

NETLINK_KOBJECT_UEVENT = 15
monitor = None


class SWPSEventMonitor(object):

    def __init__(self):
        self.recieved_events = OrderedDict()
        self.socket = socket.socket(socket.AF_NETLINK, socket.SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)

    def start(self):
        self.socket.bind((os.getpid(), -1))

    def stop(self):
        self.socket.close()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()

    def __iter__(self):
        global monitor
        while True:
            for item in monitor.next_events():
                yield item

    def next_events(self):
        data = self.socket.recv(16384)
        event = {}
        for item in data.split(b'\x00'):
            if not item:
                # check if we have an event and if we already received it
                if event and event['SEQNUM'] not in self.recieved_events:
                    self.recieved_events[event['SEQNUM']] = None
                    if (len(self.recieved_events) > 100):
                        self.recieved_events.popitem(last=False)
                    yield event
                event = {}
            else:
                try:
                    k, v = item.split(b'=', 1)
                    event[k.decode('ascii')] = v.decode('ascii')
                except ValueError:
                    pass


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping = {
        0: 12,
        1: 13,
        2: 14,
        3: 15,
        4: 16,
        5: 17,
        6: 18,
        7: 19,
        8: 20,
        9: 21,
        10: 22,
        11: 23,
        12: 24,
        13: 25,
        14: 26,
        15: 27,
        16: 28,
        17: 29,
        18: 30,
        19: 31,
        20: 32,
        21: 33,
        22: 34,
        23: 35,
        24: 36,
        25: 37,
        26: 38,
        27: 39,
        28: 40,
        29: 41,
        30: 42,
        31: 43
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(0, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2cbus_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        self.phy_port_dict = {}
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/present")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/lpmod")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/lpmod", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        lpmode = int(reg_file.readline().rstrip())

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = 1
        else:
            reg_value = 0

        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def reset(self, port_num):
        QSFP_RESET_REGISTER_DEVICE_FILE = "/sys/class/swps/port"+str(port_num)+"/reset"
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 0
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 2 second to allow it to settle
        time.sleep(2)

        # Flip the value back write back to the register to take port out of reset
        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def _get_port_eeprom_path(self, port_num, devid):
        sysfs_i2c_adapter_base_path = "/sys/class/i2c-adapter"
        if devid == self.IDENTITY_EEPROM_ADDR:
            return SfpUtilBase._get_port_eeprom_path(self, port_num, devid)
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
                ret = self._add_new_sfp_device(
                    sysfs_sfp_i2c_adapter_path, devid)
                if ret != 0:
                    print("Error adding sfp device")
                    return None

            sysfs_sfp_i2c_client_eeprom_path = "%s/eeprom" % sysfs_sfp_i2c_client_path

        return sysfs_sfp_i2c_client_eeprom_path

    def get_transceiver_dom_info_dict(self, port_num):
        if port_num in self.qsfp_ports:
            return SfpUtilBase.get_transceiver_dom_info_dict(self, port_num)
        else:
            transceiver_dom_info_dict = {}

            offset = 0
            file_path = self._get_port_eeprom_path(port_num, self.DOM_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = open(file_path, "rb")
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None

            sfpd_obj = sff8472Dom(None, 1)
            if sfpd_obj is None:
                return None

            dom_temperature_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                print(dom_temperature_raw)
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
                print(dom_temperature_data)
            else:
                return None

            dom_voltage_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_VLOT_OFFSET), SFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                print(dom_voltage_raw)
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            else:
                return None

            dom_channel_monitor_raw = self._read_eeprom_specific_bytes(
                sysfsfile_eeprom, (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
            else:
                return None

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']
            transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RXPower']['value']
            transceiver_dom_info_dict['rx2power'] = 'N/A'
            transceiver_dom_info_dict['rx3power'] = 'N/A'
            transceiver_dom_info_dict['rx4power'] = 'N/A'
            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TXBias']['value']
            transceiver_dom_info_dict['tx2bias'] = 'N/A'
            transceiver_dom_info_dict['tx3bias'] = 'N/A'
            transceiver_dom_info_dict['tx4bias'] = 'N/A'
            transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TXPower']['value']
            transceiver_dom_info_dict['tx2power'] = 'N/A'
            transceiver_dom_info_dict['tx3power'] = 'N/A'
            transceiver_dom_info_dict['tx4power'] = 'N/A'

            return transceiver_dom_info_dict

    def get_transceiver_change_event(self, timeout=0):
        global monitor
        port_dict = {}
        with SWPSEventMonitor() as monitor:
            for event in monitor:
                if event['SUBSYSTEM'] == 'swps':
                    #print('SWPS event. From %s, ACTION %s, IF_TYPE %s, IF_LANE %s' % (event['DEVPATH'], event['ACTION'], event['IF_TYPE'], event['IF_LANE']))
                    portname = event['DEVPATH'].split("/")[-1]
                    rc = re.match(r"port(?P<num>\d+)", portname)
                    if rc is not None:
                        if event['ACTION'] == "remove":
                            remove_num = int(rc.group("num"))
                            port_dict[remove_num] = "0"
                            #port_dict[rc.group("num")] = "0"
                        if event['ACTION'] == "add":
                            add_num = int(rc.group("num"))
                            port_dict[add_num] = "1"
                            #port_dict[rc.group("num")] = "1"
                        return True, port_dict
                    return False, {}
