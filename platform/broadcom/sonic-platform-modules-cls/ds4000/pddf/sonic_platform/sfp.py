#!/usr/bin/env python

try:
    import ast
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform_base.sonic_xcvr.api.public.cmis import CmisApi
    from sonic_platform_base.sonic_xcvr.api.public.sff8636 import Sff8636Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8436 import Sff8436Api
    from sonic_platform_base.sonic_xcvr.api.public.sff8472 import Sff8472Api
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

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
