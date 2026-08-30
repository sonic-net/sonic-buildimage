#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    import time
    from multiprocessing import Lock

    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform_base.sonic_xcvr.api.public.cmis import CmisApi
    from sonic_platform_base.sonic_xcvr.api.public.sff8636 import Sff8636Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8436 import Sff8436Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8472 import Sff8472Api
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self.eeprom_lock = Lock()
        self._xcvr_api = None

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_device_type(self):
        device = 'PORT{}'.format(self.port_index)
        return self.pddf_obj.get_device_type(device)

    def get_reset_status(self):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'): 
            return False
        else:
            return super().get_reset_status()

    def get_lpmode(self):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'):
            return super().get_tx_disable()
        else:
            return super().get_lpmode()

    def reset(self):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'):
            return False
        else:
            status = False
            device = 'PORT{}'.format(self.port_index)
            path = self.pddf_obj.get_path(device, 'xcvr_reset')

            if path:
                try:
                    f = open(path, 'r+')
                except IOError as e:
                    return False

                try:
                    f.seek(0)
                    f.write('0')
                    time.sleep(2)
                    f.seek(0)
                    f.write('1')

                    f.close()
                    status = True
                except IOError as e:
                    status = False
            else:
                # Use common SfpOptoeBase implementation for reset
                status = super().reset()

            return status

    def set_lpmode(self, lpmode):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'):
            if lpmode == False:
                super().tx_disable(True)
                time.sleep(0.1)
            return super().tx_disable(lpmode)
        else:
            if lpmode == False:
                super().set_lpmode(True)
                time.sleep(0.1)
            return super().set_lpmode(lpmode)

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
                if attempts == max_retries:
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
        modpres = super(Sfp, self).get_presence()
        if not modpres and self._xcvr_api != None:
            self._xcvr_api = None

        return modpres

    def get_xcvr_api(self):
        if self._xcvr_api is None and self.get_presence():
            self.refresh_xcvr_api()

            # Find and update the right optoe driver
            create_dev = False
            path_list = self.eeprom_path.split('/')
            name_path = '/'.join(path_list[:-1]) + '/name'
            del_dev_path = '/'.join(path_list[:-2]) + '/delete_device'
            new_dev_path = '/'.join(path_list[:-2]) + '/new_device'

            if isinstance(self._xcvr_api, CmisApi):
                new_driver = 'optoe3'
            elif isinstance(self._xcvr_api, Sff8636Api):
                new_driver = 'optoe1'
            elif isinstance(self._xcvr_api, Sff8436Api):
                new_driver = 'sff8436'
            elif isinstance(self._xcvr_api, Sff8472Api):
                new_driver = 'optoe2'
            else:
                new_driver = 'optoe1'

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

            if isinstance(self._xcvr_api, Sff8636Api) or \
                isinstance(self._xcvr_api, Sff8436Api):
                self.write_eeprom(93,1,bytes([0x04]))

        return self._xcvr_api

    def get_platform_media_key(self, transceiver_dict, port_speed, lane_count):
        # Per lane speed
        media_key = str(int(port_speed / lane_count))

        api = self.get_xcvr_api()
        if isinstance(api, CmisApi):
            media_compliance_code = transceiver_dict['specification_compliance']
            if 'passive_copper' in media_compliance_code:
                media_len = transceiver_dict['cable_length']
                media_key += '-copper-' + str(media_len) + 'M'
            else:
                media_key += '-optical'
        else:
            if api.is_copper():
                media_len = transceiver_dict['cable_length']
                media_key += '-copper-' + str(media_len) + 'M'
            else:
                media_key += '-optical'

        return {\
            'vendor_key': '',\
            'media_key': media_key,\
            'lane_speed_key': ''\
        }
