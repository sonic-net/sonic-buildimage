# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import subprocess
    import re
    import os
    import threading
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 49
    PORT_END = 52
    PORTS_IN_BLOCK = 53
    EEPROM_OFFSET = 9
    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    I2C_MAX_ATTEMPT = 3
    
    SFP_STATUS_INSERTED = '1'
    SFP_STATUS_REMOVED = '0'

    TXWRT_PROTECT = 0X4E
    TXWRT_NO_PROTECT = 0X59
    
    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping ={}
    port_dict = {}
    port_presence_info = {}
    port_reset_info = {}
    port_txdis_info = {}
    port_txwrt_info = {}
    port_led_info = {}

    port_rxlos_info = {}
    port_txfault_info = {}
    port_drop_info = {}
    
    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def sfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK))

    @property
    def qsfp_ports(self):
        return []

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
            self.port_to_i2cbus_mapping[x] = (x - self.PORT_START + self.EEPROM_OFFSET)
        self.port_presence_info["/sys/bus/i2c/devices/3-0030/sfp_presence1"] = [49, 50, 51, 52]
        self.port_txdis_info["/sys/bus/i2c/devices/3-0030/tx_disable"] = [49, 50, 51, 52]
        self.port_txwrt_info["/sys/bus/i2c/devices/3-0030/tx_write_protect"] = [49, 50, 51, 52]
        
        # bit 1: los
        self.port_rxlos_info["/sys/bus/i2c/devices/3-0030/sfp_rx_loss1"] = [49, 50, 51, 52]
        
        # bit 1: fault
        self.port_txfault_info["/sys/bus/i2c/devices/3-0030/sfp_tx_fault1"] = [49, 50, 51, 52]
        
        # bit 1: drop
        self.port_drop_info["/sys/bus/i2c/devices/3-0030/sfp_drop_record1"] = [49, 50, 51, 52]
        
        SfpUtilBase.__init__(self)

    def _sfp_read_file_path(self, file_path, offset, num_bytes):
        attempts = 0
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                file_path.seek(offset)
                read_buf = file_path.read(num_bytes)
            except Exception:
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
            print(("Error writing to new device file: %s" % str(err)))
            return 1
        else:
            return 0

    def _get_port_eeprom_path(self, port_num, devid):
        if port_num in list(self.port_to_eeprom_mapping.keys()):
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
                print(("Could not find i2c bus %s. Driver not loaded?" % sysfs_sfp_i2c_adapter_path))
                return None

            sysfs_sfp_i2c_client_path = "%s/%s-00%s" % (sysfs_sfp_i2c_adapter_path,
                                                        str(i2c_adapter_id),
                                                        hex(devid)[-2:])

            # If sfp device is not present on bus, Add it
            if not os.path.exists(sysfs_sfp_i2c_client_path):
                if port_num in self.qsfp_ports:
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
            if isinstance(raw, str):
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception as err:
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

        presence_path = None
        for presence_key in self.port_presence_info:
            if port_num in self.port_presence_info[presence_key]:
                presence_path = presence_key
                presence_offset = self.port_presence_info[presence_key].index(port_num)
                break
        if presence_path == None:
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
        if result & (1 << presence_offset) == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        return False

    def set_low_power_mode(self, port_num, lpmode):
        return False

    def reset(self, port_num):
        return False

    def reset_all(self):
        return False

    def _do_write_file(self, file_handle, offset, value):
        file_handle.seek(offset)
        file_handle.write(hex(value))

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        currernt_port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print(("get_transceiver_change_event:Invalid timeout value", timeout))
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print(('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout))

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
        print ("get_transceiver_change_event: Should not reach here.")
        return False, {}

    def tx_disable(self, port_num, disable):
        if not self.get_presence(port_num):
            return False
        
        if port_num in self.sfp_ports:
            txwrt_path = None
            txdis_path = None
            txdis_offset = 0
            
            for key in self.port_txwrt_info:
                if port_num in self.port_txwrt_info[key]:
                    txwrt_path = key
                    break
            if txwrt_path == None:
                return False
            
            for key in self.port_txdis_info:
                if port_num in self.port_txdis_info[key]:
                    txdis_path = key
                    txdis_offset = self.port_txdis_info[key].index(port_num)
                    break
            if txdis_path == None:
                return False
            
            
            try:
                with open(txwrt_path, "r+") as sys_file:
                    sres = hex(self.TXWRT_NO_PROTECT)[2:]
                    sys_file.write(sres)
                
                with open(txdis_path, "r+") as sys_file:
                    txdis_data = sys_file.read(2)
                    if not txdis_data:
                        return False
                    result = int(txdis_data, 16)
                    if disable:
                        result = result | (1 << txdis_offset)
                    else:
                        result = result & (~(1 << txdis_offset))
                    sys_file.seek(0)
                    sres = hex(result)[2:]
                    print(result,sres)
                    sys_file.write(sres)
                
                with open(txwrt_path, "r+") as sys_file:
                    sres = hex(self.TXWRT_PROTECT)[2:]
                    sys_file.write(sres)
            except Exception as err:
                print(err)
                return False
            
            return True
        else:
            return False

########### sysdiag ###########
    def _get_cpld_info(self, port_num, info):
        path = None
        offset = 0
        for key in info:
            if port_num in info[key]:
                path = key
                offset = info[key].index(port_num)
                break
        return path, offset

    def get_tx_disable(self, port_num):
        # cur only support sfp moudle
        if port_num not in self.sfp_ports:
            return False
        
        if not self.get_presence(port_num):
            return False
        
        path, offset = self._get_cpld_info(port_num, self.port_txdis_info)
        if path == None:
            return False
        
        result = 0
        try:
            with open(path, "r") as sys_file:
                data = sys_file.read(2)
                result = int(data, 16)
        except Exception as e:
            print((str(e)))
            return False
        
        # 1: disable
        if result & (1 << offset):
            return True
        else:
            return False
    
    def get_rx_los(self, port_num):
        # cur only support sfp moudle
        if port_num not in self.sfp_ports:
            return False
        
        path, offset = self._get_cpld_info(port_num, self.port_rxlos_info)
        if path == None:
            return False
        
        result = 0
        try:
            with open(path, "r") as sys_file:
                data = sys_file.read(2)
                result = int(data, 16)
        except Exception as e:
            print((str(e)))
            return False
        
        # 1: los
        if result & (1 << offset):
            return True
        else:
            return False
    
    def get_tx_fault(self, port_num):
        # cur only support sfp moudle
        if port_num not in self.sfp_ports:
            return False
        
        if not self.get_presence(port_num):
            return False
        
        path, offset = self._get_cpld_info(port_num, self.port_txfault_info)
        if path == None:
            return False
        
        result = 0
        try:
            with open(path, "r") as sys_file:
                data = sys_file.read(2)
                result = int(data, 16)
        except Exception as e:
            print((str(e)))
            return False
        
        # 1: fault
        if result & (1 << offset):
            return True

        return False
