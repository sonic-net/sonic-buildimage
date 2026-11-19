#!/usr/bin/env python

try:
    import os
    import datetime
    import fcntl
    import subprocess
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform_base.sonic_xcvr.api.public.cmis import CmisApi
    from sonic_platform_base.sonic_xcvr.api.public.sff8636 import Sff8636Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8436 import Sff8436Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8472 import Sff8472Api
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_RESET_FILE_PATH = "/var/run/pmon/SFP{}_reset_ts.txt"
SFP_LOCK_FILE = "/var/run/pmon/SFP_lock.txt"


class ClsPddfSfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

        self.__docker = None

    def write_ts(self, file_path):
        ts = datetime.datetime.now().isoformat()
    def read_ts(self, file_path):
        ts_str = None
        return ts_str

    def remove_ts(self, file_path):
        pass

    def reset(self):
        # Reset call just resets the transceiver to the factory state
        # of the EEPROM so it is required to reinitialze it again
        # Write the timestamp of the reset based on the port index so
        # that it is used by get_presence API
        # Timestamp is created before calling reset to avoid reading the
        # presence as True during reset
        file_path = SFP_RESET_FILE_PATH.format(self.port_index)

        if self.read_ts(file_path) != None:
            # Wait until the reset completes
            return False

        self.write_ts(file_path)

        return super(ClsPddfSfp, self).reset()

    def get_presence(self):
        # If file exists then the transceiver is triggered for a reset,
        # so until 5 secs return the presence as False and then return
        # the actual presence after 5 secs. This enables xcvrd to do the
        # reinitialization of the module correctly
        file_path = SFP_RESET_FILE_PATH.format(self.port_index)

        ts_str = self.read_ts(file_path)
        if ts_str:
            ts = datetime.datetime.fromisoformat(ts_str)
            now = datetime.datetime.now()
            if (now - ts).total_seconds() <= 5:
                modpres = False
            else:
                self.remove_ts(file_path)
                modpres = super(ClsPddfSfp, self).get_presence()
        else:
            modpres = super(ClsPddfSfp, self).get_presence()

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
        speed_str = str(int(port_speed / lane_count))
        vendor_key = speed_str
        media_key = ''
        lane_speed_key = ''

        api = self.get_xcvr_api()
        if isinstance(api, CmisApi):
            media_compliance_code = transceiver_dict['specification_compliance']
            if 'passive_copper' in media_compliance_code:
                media_len = transceiver_dict['cable_length']
                vendor_key += '-copper-' + str(media_len) + 'M'
                media_key = speed_str + '-copper-default'
            else:
                vendor_key += '-optical'
        else:
            if api.is_copper():
                media_len = transceiver_dict['cable_length']
                vendor_key += '-copper-' + str(media_len) + 'M'
                media_key = speed_str + '-copper-default'
            else:
                vendor_key += '-optical'

        return {\
            'vendor_key': vendor_key,\
            'media_key': media_key,\
            'lane_speed_key': lane_speed_key\
        }

    def get_transceiver_status(self):
        # Override to get the port cage type
        status_dict = super(ClsPddfSfp, self).get_transceiver_status()
        if status_dict:
            status_dict['cage_type'] = self.get_port_or_cage_type()

        return status_dict
