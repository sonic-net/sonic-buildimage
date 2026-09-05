#!/usr/bin/env python

try:
    import ast
    import syslog
    import time
    from multiprocessing import Lock
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform_base.sonic_xcvr.api.public.c_cmis import CmisApi
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self.eeprom_lock = Lock()

    # Provide the functions/variables below for which implementation is to be overwritten
    # Add reties to work around FPGAPCI 0050/eeprom: offset 0x0: sometimes read failed
    def __read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
            offset :
                Integer, the offset from which the read transaction will start
            num_bytes:
                Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        buf = None
        eeprom_raw = []
        sysfs_sfp_i2c_client_eeprom_path = self.eeprom_path

        if not self.get_presence():
            return None

        sysfsfile_eeprom = None
        attempts = 0
        max_retries = 5
        success = False
        while attempts < max_retries and not success:
            try:
                if attempts > 0:
                    time.sleep(0.2)
                sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "rb", 0)
                sysfsfile_eeprom.seek(offset)
                buf = sysfsfile_eeprom.read(num_bytes)
                success = True
            except Exception as ex:
                attempts += 1
                # Eliminate the redundant errors by showing errors only for lower page and page 0
                if attempts == max_retries:
                    if offset < 256:
                        syslog.syslog(syslog.LOG_INFO, "port {0}: {1}: offset {2}: read reach retry limit, refer to last eeprom cache".format(self.port_index, sysfs_sfp_i2c_client_eeprom_path, hex(offset)))
                    return None
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()

        if buf is None:
            return None

        for x in buf:
            eeprom_raw.append(x)

        while len(eeprom_raw) < num_bytes:
            eeprom_raw.append(0)
        return bytes(eeprom_raw)

    # Read out any bytes from any offset
    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        self.eeprom_lock.acquire()
        bytes = self.__read_eeprom(offset, num_bytes)
        self.eeprom_lock.release()
        return bytes

    def get_presence(self):
        presence = PddfSfp.get_presence(self)
        if not presence and self._xcvr_api != None:
            self._xcvr_api = None

        return presence

    def get_xcvr_api(self):
        if self._xcvr_api is None and self.get_presence():
            self.refresh_xcvr_api()

            # Find and update the right optoe driver
            api_to_driver_map = {\
                'Sff8636Api': 'optoe1',\
                'Sff8472Api': 'optoe2',\
                'CmisApi': 'optoe3',\
                'CCmisApi': 'optoe3',\
                'Sff8436Api': 'sff8436'\
            }
            create_dev = False
            path_list = self.eeprom_path.split('/')
            name_path = '/'.join(path_list[:-1]) + '/name'
            del_dev_path = '/'.join(path_list[:-2]) + '/delete_device'
            new_dev_path = '/'.join(path_list[:-2]) + '/new_device'
            api_name = type(self._xcvr_api).__name__
            new_driver = api_to_driver_map.get(api_name, 'optoe1')

            try:
                with open(name_path, 'r') as fd:
                    cur_driver = fd.readline().strip()
            except FileNotFoundError:
                create_dev = True
            else:
                if cur_driver != new_driver:
                    with open(del_dev_path, 'w') as fd:
                        fd.write("0x50")
                    create_dev = True

            if create_dev:
                with open(new_dev_path, 'w') as fd:
                    fd.write("{} 0x50".format(new_driver))

            if api_name == 'Sff8636Api' or \
                api_name == 'Sff8436Api':
                self.write_eeprom(93,1,bytes([0x04]))

        return self._xcvr_api

    def get_platform_media_key(self, transceiver_dict, port_speed, lane_count):
        api = self.get_xcvr_api()
        api_name = type(api).__name__
        if api_name in ['CmisApi', 'CCmisApi']:
            is_cmis = True
        else:
            is_cmis = False

        # Per lane speed
        media_key = str(int(port_speed / lane_count))
        if is_cmis:
            media_compliance_code = transceiver_dict['specification_compliance']
            if 'copper' in media_compliance_code:
                media_len = transceiver_dict['cable_length']
                media_key += '-copper-' + str(media_len) + 'M'
            else:
                media_key += '-optical'
        else:
            media_compliance_dict = ast.literal_eval(transceiver_dict['specification_compliance'])
            eth_compliance_str = '10/40G Ethernet Compliance Code'
            ext_compliance_str = 'Extended Specification Compliance'
            media_compliance_code = ''
            if eth_compliance_str in media_compliance_dict:
                media_compliance_code = media_compliance_dict[eth_compliance_str]
            if ext_compliance_str in media_compliance_dict:
                media_compliance_code = media_compliance_code + ' ' + media_compliance_dict[ext_compliance_str]
            if 'CR' in media_compliance_code or "copper" in transceiver_dict['specification_compliance'].lower():
                media_len = transceiver_dict['cable_length']
                media_key += '-copper-' + str(media_len) + 'M'
            else:
                media_key += '-optical'

        return {\
            'vendor_key': '',\
            'media_key': media_key,\
            'lane_speed_key': ''\
        }
