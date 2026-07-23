#!/usr/bin/env python

try:
    import os
    import sys
    import re
    import time
    import struct
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_sfp.sff8472 import sff8472Dom
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'
SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_MODULE_THRESHOLD_OFFSET = 112
SFP_MODULE_THRESHOLD_WIDTH = 5
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 6

class SfpUtil(SfpUtilBase):
    """Platform specific sfputil class"""

    PORT_START = 0
    PORT_END = 55
    PORTS_IN_BLOCK = 55
    QSFP_PORT_START = 0
    QSFP_PORT_END = 55
    cplda_sfp_num = 0


    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    @property
    def qsfp_port_end(self):
        return self.QSFP_PORT_END

    @property
    def qsfp_ports(self):
        return range(self.QSFP_PORT_START, self.PORTS_IN_BLOCK + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping


    def __init__(self):
        for x in range(self.port_start, self.port_end+1):
            eeprom_path = '/sys_switch/transceiver/eth{}/eeprom'.format(x + 1)
            self.port_to_eeprom_mapping[x] = eeprom_path
        SfpUtilBase.__init__(self)


    def get_presence(self, port_num):
        if port_num not in self.qsfp_ports:
            return False
        presence_path = '/sys_switch/transceiver/eth{}/present'.format(port_num + 1)

        try:
            file = open(presence_path)
        except IOError as e:
            print ("Error: unable to open file: %s" % str(e))
            return False

        value = int(file.readline().rstrip(),16)

        file.close()
        if value & 0x1 == 1:
            return True

        return False

    def get_power(self, port_num):
        if port_num not in self.qsfp_ports:
            return False
        power_path = '/sys_switch/transceiver/eth{}/power_on'.format(port_num + 1)

        try:
            file = open(power_path)
        except IOError as e:
            print ("Error: unable to open file: %s" % str(e))
            return False

        value = int(file.readline().rstrip(),16)

        file.close()
        if value & 0x1 == 1:
            return True

        return False

    def set_power(self, port_num, lpmode):
        if port_num not in self.qsfp_ports:
            return False
        power_path = '/sys_switch/transceiver/eth{}/power_on'.format(port_num + 1)
        
        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            value = 1
        else:
            value = 0

        try:
            file = open(power_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(value))
        file.close()

        return True

    def set_eeprom(self, port_num, page, offset, data):
        if port_num not in self.qsfp_ports:
            return False
        sysfs_eeprom_path = '/sys_switch/transceiver/eth{}/eeprom'.format(port_num + 1)
        buffer = create_string_buffer(1)
        buffer[0] = struct.pack('B',data)
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode='rb+', buffering=0)
            sysfsfile_eeprom.seek(offset + page*128)
            sysfsfile_eeprom.write(buffer[0])
            sysfsfile_eeprom.close()
        except OSError as e:
            print(" %s" % str(e), True)
            return False

        return True

    def get_eeprom(self, port_num, page, offset, length):
        data = None
        if port_num < self.port_start or port_num > self.port_end:
            return False
        sysfs_eeprom_path = '/sys_switch/transceiver/eth{}/eeprom'.format(port_num + 1)
        
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode='rb+', buffering=0)
            sysfsfile_eeprom.seek(offset + page*128)
            data = bytearray(sysfsfile_eeprom.read(length))
            sysfsfile_eeprom.close()
        except OSError as e:
            print(" %s" % str(e), True)

        return data
        
    def get_low_power_mode(self, port_num):
        if port_num not in self.qsfp_ports:
            return False
        lowpower_path = '/sys_switch/transceiver/eth{}/lpmode'.format(port_num + 1)
        
        try:
            file = open(lowpower_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        value = int(file.readline().rstrip(),16)

        file.close()
        if value == 1:
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        if port_num not in self.qsfp_ports:
            return False
        lowpower_path = '/sys_switch/transceiver/eth{}/lpmode'.format(port_num + 1)
        
        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            value = 1
        else:
            value = 0

        try:
            file = open(lowpower_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(value))
        file.close()

        return True

    def reset(self, port_num):
        if port_num not in self.qsfp_ports:
            return False
        reset_path = '/sys_switch/transceiver/eth{}/reset'.format(port_num + 1)

        try:
            file = open(reset_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(1))
        file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        try:
            file = open(reset_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(0))
        file.close()

        return True

    def read_porttab_mappings(self, porttabfile, asic_inst=0):
        logical = []
        logical_to_bcm = {}
        logical_to_physical = {}
        physical_to_logical = {}
        last_fp_port_index = 0
        last_portname = ""
        first = 1
        port_pos_in_file = 0
        parse_fmt_port_config_ini = False

        try:
            f = open(porttabfile)
        except:
            raise

        parse_fmt_port_config_ini = (os.path.basename(porttabfile) == "port_config.ini")

        # Read the porttab file and generate dicts
        # with mapping for future reference.
        # XXX: move the porttab
        # parsing stuff to a separate module, or reuse
        # if something already exists
        for line in f:
            line.strip()
            if re.search("^#", line) is not None:
                continue

            # Parsing logic for 'port_config.ini' file
            if (parse_fmt_port_config_ini):
                # bcm_port is not explicitly listed in port_config.ini format
                # Currently we assume ports are listed in numerical order according to bcm_port
                # so we use the port's position in the file (zero-based) as bcm_port
                portname = line.split()[0]

                bcm_port = str(port_pos_in_file)

                if len(line.split()) >= 4:
                    fp_port_index = int(line.split()[4])
                else:
                    fp_port_index = portname.split("Ethernet").pop()
                    fp_port_index = int(fp_port_index.split("s").pop(0))/4
            else:  # Parsing logic for older 'portmap.ini' file
                (portname, bcm_port) = line.split("=")[1].split(",")[:2]

                fp_port_index = portname.split("Ethernet").pop()
                fp_port_index = int(fp_port_index.split("s").pop(0))/4

            if ((len(self.sfp_ports) > 0) and (fp_port_index not in self.sfp_ports)):
                continue

            if(fp_port_index > self.QSFP_PORT_END):
                continue

            if first == 1:
                # Initialize last_[physical|logical]_port
                # to the first valid port
                last_fp_port_index = fp_port_index
                last_portname = portname
                first = 0

            logical.append(portname)

            logical_to_bcm[portname] = "xe" + bcm_port
            logical_to_physical[portname] = [fp_port_index]
            if physical_to_logical.get(fp_port_index) is None:
                physical_to_logical[fp_port_index] = [portname]
            else:
                physical_to_logical[fp_port_index].append(
                    portname)

            if (fp_port_index - last_fp_port_index) > 1:
                # last port was a gang port
                for p in range(last_fp_port_index+1, fp_port_index):
                    logical_to_physical[last_portname].append(p)
                    if physical_to_logical.get(p) is None:
                        physical_to_logical[p] = [last_portname]
                    else:
                        physical_to_logical[p].append(last_portname)

            last_fp_port_index = fp_port_index
            last_portname = portname

            port_pos_in_file += 1

        self.logical = logical
        self.logical_to_bcm = logical_to_bcm
        self.logical_to_physical = logical_to_physical
        self.physical_to_logical = physical_to_logical

        """
        print "logical: " +  self.logical
        print "logical to bcm: " + self.logical_to_bcm
        print "logical to physical: " + self.logical_to_physical
        print "physical to logical: " + self.physical_to_logical
        """

    @property
    def _get_presence_bitmap(self):

        bits = []
        for x in range(self.port_start, self.port_end+1):
          bits.append(str(int(self.get_presence(x))))

        rev = "".join(bits[::-1])
        return int(rev,2)

    data = {'present':1}
    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range (self.port_start, self.port_end+1):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - self.port_start))
                if changed_ports & mask:
                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            return True, port_dict
        else:
            return True, {}
        return False, {}

    def get_transceiver_dom_info_dict(self, port_num):
        transceiver_dom_info_dict = {}

        dom_info_dict_keys = ['temperature', 'voltage',  'rx1power',
                              'rx2power',    'rx3power', 'rx4power',
                              'tx1bias',     'tx2bias',  'tx3bias',
                              'tx4bias',     'tx1power', 'tx2power',
                              'tx3power',    'tx4power',
                             ]
        transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        if port_num in self.qsfp_ports:
            return SfpUtilBase.get_transceiver_dom_info_dict(self, port_num)
        elif port_num in self.osfp_ports:
            return SfpUtilBase.get_transceiver_dom_info_dict(self, port_num)
        else:
            bdata=''
            offset = 256
            file_path = self._get_port_eeprom_path(port_num, self.DOM_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
                bdata = sysfsfile_eeprom.read()
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None
            if len(bdata) <= 256:
                sysfsfile_eeprom.close()
                return None

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None
            dom_temperature_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom, (offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            else:
                return None

            dom_voltage_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom, (offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            else:
                return None

            dom_channel_monitor_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom, (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
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

    def get_transceiver_dom_threshold_info_dict(self, port_num):
        transceiver_dom_threshold_info_dict = {}

        dom_info_dict_keys = ['temphighalarm',    'temphighwarning',
                              'templowalarm',     'templowwarning',
                              'vcchighalarm',     'vcchighwarning',
                              'vcclowalarm',      'vcclowwarning',
                              'rxpowerhighalarm', 'rxpowerhighwarning',
                              'rxpowerlowalarm',  'rxpowerlowwarning',
                              'txpowerhighalarm', 'txpowerhighwarning',
                              'txpowerlowalarm',  'txpowerlowwarning',
                              'txbiashighalarm',  'txbiashighwarning',
                              'txbiaslowalarm',   'txbiaslowwarning'
                             ]
        transceiver_dom_threshold_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')
        if port_num in self.qsfp_ports:
            return SfpUtilBase.get_transceiver_dom_threshold_info_dict(self, port_num)
        elif port_num in self.osfp_ports:
            # Below part is added to avoid fail xcvrd, shall be implemented later
            return SfpUtilBase.get_transceiver_dom_threshold_info_dict(self, port_num)
        else:
            offset = 256
            bdata=''
            file_path = self._get_port_eeprom_path(port_num, self.DOM_EEPROM_ADDR)
            if not self._sfp_eeprom_present(file_path, 0):
                return None

            try:
                sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
                bdata = sysfsfile_eeprom.read()
            except IOError:
                print("Error: reading sysfs file %s" % file_path)
                return None
            if len(bdata) <= 256:
                sysfsfile_eeprom.close()
                return None

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            dom_module_threshold_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom,
                                         (offset + SFP_MODULE_THRESHOLD_OFFSET),
                                         SFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_module_monitor_params(dom_module_threshold_raw, 0)
            else:
                return None

            dom_channel_threshold_raw = self._read_eeprom_specific_bytes(sysfsfile_eeprom,
                                         (offset + SFP_CHANNL_THRESHOLD_OFFSET),
                                         SFP_CHANNL_THRESHOLD_WIDTH)
            if dom_channel_threshold_raw is not None:
                dom_channel_threshold_data = sfpd_obj.parse_channel_thresh_monitor_params(dom_channel_threshold_raw, 0)
            else:
                return None

            try:
                sysfsfile_eeprom.close()
            except IOError:
                print("Error: closing sysfs file %s" % file_path)
                return None

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_channel_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_channel_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_channel_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_channel_threshold_data['data']['BiasLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_channel_threshold_data['data']['TxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_channel_threshold_data['data']['TxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_channel_threshold_data['data']['TxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_channel_threshold_data['data']['TxPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_channel_threshold_data['data']['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_channel_threshold_data['data']['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_channel_threshold_data['data']['RxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_channel_threshold_data['data']['RxPowerLowWarning']['value']

        return transceiver_dom_threshold_info_dict
