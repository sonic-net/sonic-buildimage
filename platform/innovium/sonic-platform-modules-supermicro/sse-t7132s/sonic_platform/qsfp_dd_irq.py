#----------------------------------------------------------------------------
# QSFP-DD 8X Transceiver (QSFP Double Density)
#----------------------------------------------------------------------------

from __future__ import print_function

try:
    from sonic_platform_base.sonic_sfp.sff8024 import type_of_transceiver    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import type_abbrv_name    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import connector_dict    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import ext_type_of_transceiver    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import type_of_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import host_electrical_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import nm_850_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import sm_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import passive_copper_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import active_cable_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sff8024 import base_t_media_interface    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
    from sonic_platform_base.sonic_sfp.sffbase import sffbase    # Dot module supports both Python 2 and Python 3 using explicit relative import methods
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

#------------------------------------------------------------------------------

class qsfp_dd_Interrupt(sffbase):

    def decode_revision_compliance(self, eeprom_data, offset, size):
        # first nibble and second nibble represent the version
        data = int(eeprom_data[offset], 16)
        return '%c.%c' % (str((data >> 4) & 0x0f), str((data & 0x0f)))

    def decode_module_state(self, eeprom_data, offset, size):
        module_state_byte = eeprom_data[offset]
        # bits 1-3
        module_state = (int(module_state_byte, 16) >> 1) & 7
        if module_state == 1:
            return 'ModuleLowPwr state'
        elif module_state == 2:
            return 'ModulePwrUp state'
        elif module_state == 3:
            return 'ModuleReady state'
        elif module_state == 4:
            return 'ModulePwrDn state'
        elif module_state == 5:
            return 'Fault state'
        return 'Unknown State %s' % module_state

    def decode_flag_host_lane(self, eeprom_data, offset, size):
        lanes_byte = eeprom_data[offset]
        lanes_binstr = '{:08b}'.format(int(lanes_byte, 16))
        return lanes_binstr

    data_path_state_encodings = {
        1: 'DataPathDeactivated State',
        2: 'DataPathInit State',
        3: 'DataPathDeinit State',
        4: 'DataPathActivated State',
        5: 'DataPathTxTurnOn State',
        6: 'DataPathTxTurnOff State',
        7: 'DataPathInitialized State'
        }

    def decode_data_path_state_high(self, eeprom_data, offset, size):
        data_path_state_byte = eeprom_data[offset]
        # bits 7-4
        data_path_state = int(data_path_state_byte, 16) >> 4
        return self.data_path_state_encodings.get(
            data_path_state, f'Unknown State {data_path_state}')

    def decode_data_path_state_low(self, eeprom_data, offset, size):
        data_path_state_byte = eeprom_data[offset]
        # bits 3-0
        data_path_state = int(data_path_state_byte, 16) & 0xf
        return self.data_path_state_encodings.get(
            data_path_state, f'Unknown State {data_path_state}')

    version = '1.0'

    rev_comp = {
        'Revision Compliance':
            {'offset': 0,
             'type': 'func',
             'decode': {'func': decode_revision_compliance}}
        }

    module_state = {
        'Module state':
            {'offset': 0,
             'type': 'func',
             'decode': {'func': decode_module_state}},
        'Interrupt':
            {'offset': 0,
             'type': 'bitmap',
             'decode': {
                 'Interrupt not asserted':
                    {'offset': 0,
                     'bit': 0},
                 'Interrupt asserted':
                    {'offset': 0,
                     'bit': 0,
                     'value': 0}}}
        }

    module_flags = {
        'L-CDB block 2 complete':
            {'offset': 0,
             'bit': 7,
             'type': 'bitvalue'},
        'L-CDB block 1 complete':
            {'offset': 0,
             'bit': 6,
             'type': 'bitvalue'},
        'Data Path firmware fault':
            {'offset': 0,
             'bit': 2,
             'type': 'bitvalue'},
        'Module firmware fault':
            {'offset': 0,
             'bit': 1,
             'type': 'bitvalue'},
        'L-Module state changed flag':
            {'offset': 0,
             'bit': 0,
             'type': 'bitvalue'},
        'L-Vcc3.3v Low Warning':
            {'offset': 1,
             'bit': 7,
             'type': 'bitvalue'},
        'L-Vcc3.3v High Warning':
            {'offset': 1,
             'bit': 6,
             'type': 'bitvalue'},
        'L-Vcc3.3v Low Alarm':
            {'offset': 1,
             'bit': 5,
             'type': 'bitvalue'},
        'L-Vcc3.3v High Alarm':
            {'offset': 1,
             'bit': 4,
             'type': 'bitvalue'},
        'L-Temp Low Warning':
            {'offset': 1,
             'bit': 3,
             'type': 'bitvalue'},
        'L-Temp High Warning':
            {'offset': 1,
             'bit': 2,
             'type': 'bitvalue'},
        'L-Temp Low Alarm':
            {'offset': 1,
             'bit': 1,
             'type': 'bitvalue'},
        'L-Temp High Alarm':
            {'offset': 1,
             'bit': 0,
             'type': 'bitvalue'},
        'L-Aux 2 Low Warning':
            {'offset': 2,
             'bit': 7,
             'type': 'bitvalue'},
        'L-Aux 2 High Warning':
            {'offset': 2,
             'bit': 6,
             'type': 'bitvalue'},
        'L-Aux 2 Low Alarm':
            {'offset': 2,
             'bit': 5,
             'type': 'bitvalue'},
        'L-Aux 2 High Alarm':
            {'offset': 2,
             'bit': 4,
             'type': 'bitvalue'},
        'L-Aux 1 Low Warning':
            {'offset': 2,
             'bit': 3,
             'type': 'bitvalue'},
        'L-Aux 1 High Warning':
            {'offset': 2,
             'bit': 2,
             'type': 'bitvalue'},
        'L-Aux 1 Low Alarm':
            {'offset': 2,
             'bit': 1,
             'type': 'bitvalue'},
        'L-Aux 1 High Alarm':
            {'offset': 2,
             'bit': 0,
             'type': 'bitvalue'},
        'L-Vendor Defined Low Warning':
            {'offset': 3,
             'bit': 7,
             'type': 'bitvalue'},
        'L-Vendor Defined High Warning':
            {'offset': 3,
             'bit': 6,
             'type': 'bitvalue'},
        'L-Vendor Defined Low Alarm':
            {'offset': 3,
             'bit': 5,
             'type': 'bitvalue'},
        'L-Vendor Defined High Alarm':
            {'offset': 3,
             'bit': 4,
             'type': 'bitvalue'},
        'L-Aux 3 Low Warning':
            {'offset': 3,
             'bit': 3,
             'type': 'bitvalue'},
        'L-Aux 3 High Warning':
            {'offset': 3,
             'bit': 2,
             'type': 'bitvalue'},
        'L-Aux 3 Low Alarm':
            {'offset': 3,
             'bit': 1,
             'type': 'bitvalue'},
        'L-Aux 3 High Alarm':
            {'offset': 3,
             'bit': 0,
             'type': 'bitvalue'}
        }

    sfp_type_abbrv_name = {
        'type_abbrv_name':
            {'offset': 0,
             'size': 1,
             'type': 'enum',
             'decode': type_abbrv_name}
        }

    qsfp_dd_dom_capability = {
        'Flat_MEM':
            {'offset': 0,
                'bit': 7,
                'type': 'bitvalue'}
        }

    data_path_state = {
        'Data Path State host lane 8':
            {'offset': 3,
             'type': 'func',
             'decode': {'func': decode_data_path_state_high}},
        'Data Path State host lane 7':
            {'offset': 3,
             'type': 'func',
             'decode': {'func': decode_data_path_state_low}},
        'Data Path State host lane 6':
            {'offset': 2,
             'type': 'func',
             'decode': {'func': decode_data_path_state_high}},
        'Data Path State host lane 5':
            {'offset': 2,
             'type': 'func',
             'decode': {'func': decode_data_path_state_low}},
        'Data Path State host lane 4':
            {'offset': 1,
             'type': 'func',
             'decode': {'func': decode_data_path_state_high}},
        'Data Path State host lane 3':
            {'offset': 1,
             'type': 'func',
             'decode': {'func': decode_data_path_state_low}},
        'Data Path State host lane 2':
            {'offset': 0,
             'type': 'func',
             'decode': {'func': decode_data_path_state_high}},
        'Data Path State host lane 1':
            {'offset': 0,
             'type': 'func',
             'decode': {'func': decode_data_path_state_low}},
        }

    lane_specific_flags = {
        'L-Data Path State Changed flag':
            {'offset': 0,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Fault flag':
            {'offset': 1,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx LOS flag':
            {'offset': 2,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx CDR LOL flag':
            {'offset': 3,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Adaptive Input Eq Fault flag':
            {'offset': 4,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Power High alarm':
            {'offset': 5,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Power Low alarm':
            {'offset': 6,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Power High warning':
            {'offset': 7,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Power Low warning':
            {'offset': 8,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Bias High Alarm':
            {'offset': 9,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Bias Low alarm':
            {'offset': 10,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Bias High warning':
            {'offset': 11,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Tx Bias Low warning':
            {'offset': 12,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx LOS':
            {'offset': 13,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx CDR LOL':
            {'offset': 14,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx Power High alarm':
            {'offset': 15,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx Power Low alarm':
            {'offset': 16,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx Power High warning':
            {'offset': 17,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}},
        'L-Rx Power Low warning':
            {'offset': 18,
             'type': 'func',
             'decode': {'func': decode_flag_host_lane}}
        }

    def parse_sfp_rev_comp(self, rev_comp_raw_data, start_pos):
        return sffbase.parse(self, self.rev_comp, rev_comp_raw_data, start_pos)

    def parse_sfp_type_abbrv_name(self, type_raw_data, start_pos):
        return sffbase.parse(self, self.sfp_type_abbrv_name, type_raw_data, start_pos)

    def parse_sfp_module_state(self, module_state_raw_data, start_pos):
        return sffbase.parse(self, self.module_state, module_state_raw_data, start_pos)

    def parse_dom_capability(self, dom_capability_raw_data, start_pos):
        return sffbase.parse(self, self.qsfp_dd_dom_capability, dom_capability_raw_data, start_pos)

    def parse_sfp_module_flags(self, module_flags_raw_data, start_pos):
        return sffbase.parse(self, self.module_flags, module_flags_raw_data, start_pos)

    def parse_sfp_data_path_state(self, data_path_state_raw_data, start_pos):
        return sffbase.parse(self, self.data_path_state, data_path_state_raw_data, start_pos)

    def parse_sfp_lane_specific_flags(self, lane_specific_flags_raw_data, start_pos):
        return sffbase.parse(self, self.lane_specific_flags, lane_specific_flags_raw_data, start_pos)

