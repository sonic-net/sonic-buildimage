import binascii
import copy
import math
import re

def lane_bitval_mask_gen(key, bitvalue_map=None):
    output = dict()
    for offset in range(1):
        for bit in range(8):
            output["lane{:02d}{}".format(bit+offset*8, key)] = {
                "offset": offset,
                "bit": bit,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": bitvalue_map
            }
    return output

def laser_mask_gen(key, bitvalue_map=None):
    output = dict()
    for offset in range(1):
        for bit in range(8):
            output["Laser{:02d}{}".format(bit+offset*8, key)] = {
                "offset": offset,
                "bit": bit,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": bitvalue_map
            }
    return output

data_path_init_control_masks = lane_bitval_mask_gen("", ["Initialize", "De-initializ"])
tx_ouput_disable_control_masks = lane_bitval_mask_gen("", ["Tx output enable", "Tx output disabled"])
laser_power_control_masks = laser_mask_gen("", ["half-power", "full-power"])
apply_datapath_init_control_masks = lane_bitval_mask_gen("", ["Not Apply", "Apply"])

default_sku_path="/usr/share/sonic/device/x86_64-micas_m2-w6940-128x1-fr4-r0/default_sku"
with open(default_sku_path, 'r', encoding='utf-8') as file:
    sku_content = file.read().strip()
    sku_match = re.search(r"M2-W6940-\d+X1-FR4", sku_content)
    if sku_match:
        default_sku = sku_match.group(0)
    else:
        default_sku = "M2-W6940-128X1-FR4"

if default_sku == "M2-W6940-64X1-FR4":
    apsel_code_num = {
        '10': 800000,
        '20': 200000,
        '40': 100000
    }
else:
    apsel_code_num = {
        '10': 400000,
        '20': 200000,
        '40': 100000
    }

def apsel_mask_gen():
    output = dict()
    for lane_index in range(8):
        output["lane{:02d}".format(lane_index)] = {
            "offset": lane_index,
            "outtype": 'int',
            "type": 'enum',
            'bitmask': 0xf0,
            'decode': apsel_code_num
        }
    return output
lane_apsel_code_masks = apsel_mask_gen()

def mw_to_dbm(mW):
    if mW == 0:
        return float("-inf")
    elif mW < 0:
        return float("NaN")
    return 10. * math.log10(mW)

def calc_power(eeprom_data, offset, size):
    increments = 0.1 # μW
    PmW = (int(eeprom_data[offset], 16) * 256 + int(eeprom_data[offset + 1], 16)) / (1000./increments)
    return '%.03fdBm' % (mw_to_dbm(PmW))

def calc_bias(eeprom_data, offset, size):
    retval = 'N/A'
    try:
        msb = int(eeprom_data[offset], 16)
        lsb = int(eeprom_data[offset+1], 16)
        result = (msb << 8) | (lsb & 0xff)
        
        result = float(result * 0.002 * 2) # increments 2μA
        retval = "%.4f%s" % (result, "mA")
    except Exception as err:
        print("calc_power err %s" % str(err))
    return retval

def lane_code_mask_gen_func(func):
    output = dict()
    for lane_index in range(8):
        output["lane{:02d}".format(lane_index)] = {
            "offset": lane_index*2,
            "outtype": 'str',
            "type": 'func',
            "size": 2,
            'decode': {
                'func': func
            }
        }
    return output


def lane_code_mask_gen_enum(enum, size=1):
    output = dict()
    for lane_index in range(8):
        output["lane{:02d}".format(lane_index)] = {
            "offset": lane_index*size,
            "outtype": 'str',
            "type": 'enum',
            "size": size,
            'decode': enum
        }
    return output

def lane_code_mask_gen_bitvalue(key, bitvalue_map=None):
    output = dict()
    for offset in range(1):
        for bit in range(8):
            output["lane{:02d}{}".format(bit+offset*8, key)] = {
                "offset": offset,
                "bit": bit,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": bitvalue_map
            }
    return output

# lane_power_code_items = lane_power_code_mask_gen()
lane_dom_code_items = {
    "Rx output status": {
        'offset': 0,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["output signal invalid or muted", "output signal valid"])
    },
    "Tx output status": {
        'offset': 1,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["output signal invalid or muted", "output signal valid"])
    },
    "L-Lane Data Path State Changed Flag": {
        'offset': 2,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Fault": {
        'offset': 3,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx LOS": {
        'offset': 4,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    # "L-Tx CDR LOL Flag.  Not supported"
    # "L-Tx Adaptive input EQ fault flag.  Not supported"
    "Tx Power High Alarm": {
        'offset': 7,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Power Low Alarm": {
        'offset': 8,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Power High Warning": {
        'offset': 9,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Power Low Warning": {
        'offset': 10,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Bias High Alarm": {
        'offset': 11,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Bias Low Alarm": {
        'offset': 12,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Bias High Warning": {
        'offset': 13,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Tx Bias Low Warning": {
        'offset': 14,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Rx LOS": {
        'offset': 15,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    # "CDR LOL flag, Not supported"
    "Rx Power High Alarm": {
        'offset': 17,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Rx Power Low Alarm": {
        'offset': 18,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Rx Power High Warning": {
        'offset': 19,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "Rx Power Low Warning": {
        'offset': 20,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "OutputStatusChangedFlagRx": {
        'offset': 21,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_bitvalue('', ["No flag", "Flag latched"])
    },
    "TX Power": {
        'offset': 22,
        'size': 16,
        "outtype": 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_func(calc_power)
    },
    "TX Bias": {
        'offset': 38,
        'size': 16,
        "outtype": 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_func(calc_bias)
    },
    "RX Power": {
        'offset': 54,
        'size': 16,
        "outtype": 'dict',
        'type': 'nested',
        'decode': lane_code_mask_gen_func(calc_power)
    }
}

def twos_comp(num, bits):
    try:
        if ((num & (1 << (bits - 1))) != 0):
            num = num - (1 << bits)
        return num
    except Exception:
        return 0

def calc_temperature(eeprom_data, offset, size):
    # Internally measured temperature: signed 2’s complement in 1/256 degree Celsius increments
    try:
        cal_type = 1

        msb = int(eeprom_data[offset], 16)
        lsb = int(eeprom_data[offset + 1], 16)

        result = (msb << 8) | (lsb & 0xff)
        result = twos_comp(result, 16)

        if cal_type == 1:
            # Internal calibration
            result = float(result / 256.0)
            retval = '%.4f' %result + 'C'
        else:
            retval = 'Unknown'
    except Exception as err:
        retval = str(err)

    return retval

data_path_state_indicator_enum = {
    '00': "Reserved",
    '01': "DataPathDeactivated",
    '02': "DataPathInit",
    '03': "DataPathDeinit",
    '04': "DataPathActivated",
    '05': "DataPathTxTurnOn",
    '06': "DataPathTxTurnOff",
    '07': "DataPathInitialized",

    '10': "DataPathDeactivated",
    '20': "DataPathInit",
    '30': "DataPathDeinit",
    '40': "DataPathActivated",
    '50': "DataPathTxTurnOn",
    '60': "DataPathTxTurnOff",
    '70': "DataPathInitialized",
}

def data_path_state_indicator_lane_masks_gen(key, decode_enum=None):
    output = dict()
    for offset in range(4):
        for lane in range(2):
            lane_index = (offset*2) + lane
            output["lane{:02d}{}".format(lane_index, key)] = {
                'offset': offset,
                'size': 1,
                'type': 'enum',
                'bitmask': 0x0f<<(lane*4),
                'decode': decode_enum
            }
    return output

data_path_state_indicator_lane_masks = data_path_state_indicator_lane_masks_gen("", data_path_state_indicator_enum)

memory_map = {
    # RW
    'Module Global Controls': {
        'page': 0x00,
        'addr': 0x1a,
        'size': 1,
        "outtype": 'dict',
        'type': 'nested',
        'decode': {
            'BankBoradcastEnable': {
                "offset": 0,
                "bit": 7,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": ["disabled", "enabled"]
            },
            'LowPwrAllowRequestHW': {
                "offset": 0,
                "bit": 6,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": ["disabled", "enabled"]
            },
            'Squelch control': {
                "offset": 0,
                "bit": 5,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": ["Tx squelch reduces OMA", "Tx squelch reduces Pave"]
            },
            'LowPwrRequestSW': {
                "offset": 0,
                "bit": 4,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": ["No request", "Request"]
            },
            'Software reset': {
                "offset": 0,
                "bit": 3,
                "outtype": 'str',
                "type": 'bitvalue',
                "bitvalue_map": ["Not in reset", "Software reset"]
            }
        }
    },
    #RW
    'Data Path Initialization Control': {
        'page': 0x10,
        'addr': 0x80,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': data_path_init_control_masks
    },
    #RW
    'Tx output disable': {
        'page': 0x10,
        'addr': 0x82,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': tx_ouput_disable_control_masks
    },
    #RO
    'Data Path State Indicator': {
        'page': 0x11,
        'addr': 0x80,
        'size': 4,
        'outtype': 'dict',
        'type': 'nested',
        'decode': data_path_state_indicator_lane_masks
    },
    #RO
    'Module State': {
        'page': 0x0,
        'addr': 0x3,
        'bitmask': 0xe,
        'size': 1,
        'type': 'enum',
        'decode': {
            '00': 'Reserved',
            '02': 'ModuleLowPwr',
            '04': 'ModulePwrUp',
            '06': 'ModuleReady',
            '08': 'ModulePwrDn',
            '0a': 'ModuleFault',
            '0c': 'Reserved',
            '0e': 'Reserved'
        }
    },
    #RW
    'Laser power mode control 0': {
        'page': 0xb2,
        'addr': 0x80,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': laser_power_control_masks
    },
    #RW
    'Laser power mode control 1': {
        'page': 0xb6,
        'addr': 0x80,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': laser_power_control_masks
    },
    #RO
    'Active Application Control Set': {
        'page': 0x11,
        'addr': 0xce,
        'size': 8,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_apsel_code_masks
    },
    #RW
    'Application Select Controls': {
        'page': 0x10,
        'addr': 0x91,
        'size': 8,
        'outtype': 'dict',
        'type': 'nested',
        'decode': lane_apsel_code_masks
    },
    #RW
    'Apply DataPathInit Controls': {
        'page': 0x10,
        'addr': 0x8f,
        'size': 1,
        'outtype': 'dict',
        'type': 'nested',
        'decode': apply_datapath_init_control_masks
    },
    #RO
    'Vendor infomation': {
        'page': 0x00,
        'addr': 0x81,
        'size': 61,
        "outtype": 'dict',
        'type': 'nested',
        'decode': {
            'Vendor Name': {
                'offset': 0,
                'size': 16,
                'type': 'str'
            },
            'Vendor OUI': {
                'offset': 16,
                'size': 3,
                'type': 'hex'
            },
            'Vendor PN': {
                'offset': 19,
                'size': 16,
                'type': 'str'
            },
            'Vendor Rev': {
                'offset': 35,
                'size': 2,
                'type': 'str'
            },
            'Vendor SN': {
                'offset': 37,
                'size': 16,
                'type': 'str'
            },
            'CPO Vendor SN': {
                'offset': 37,
                'size': 14,
                'type': 'str'
            },
            'Vendor Date': {
                'offset': 53,
                'size': 8,
                'type': 'date'
            }   
        }
    },
    #RO
    'lanes dom info': {
        'page': 0x11,
        'addr': 0x84,
        'size': 70,
        "outtype": 'dict',
        'type': 'nested',
        'decode': lane_dom_code_items
    },
    #RO
    'dom info': {
        'page': 0x00,
        'addr': 0xe,
        'size': 10,
        "outtype": 'dict',
        'type': 'nested',
        'decode': {
            'temperature': {
                "offset": 0,
                "outtype": 'str',
                "type": 'func',
                "size": 2,
                'decode': {
                    'func': calc_temperature
                }
            }
        }
    }
}

class cmis_memory_helper_base():

    def read(self, bank, page, addr, rd_len):
        raise NotImplementedError

    def write(self, bank, page, addr, data):
        raise NotImplementedError

    def convert_date_to_string(self, eeprom_data, offset, size):
        try:
            year_offset  = 0
            month_offset = 2
            day_offset   = 4
            lot_offset   = 6

            date = self.convert_hex_to_string(eeprom_data, offset, offset + size)
            retval = "20"+ date[year_offset:month_offset] + "-" + \
                    date[month_offset:day_offset] + "-" + \
                    date[day_offset:lot_offset] + " " + \
                    date[lot_offset:size]
        except Exception as err:
            retval = str(err)
        return retval

    # Convert Hex to String
    def convert_hex_to_string(self, arr, start, end):
        try:
            ret_str = ''
            for n in range(start, end):
                ret_str += arr[n]
            return binascii.unhexlify(ret_str).decode("utf-8", "ignore").strip()
        except Exception as err:
            return str(err)

    def test_bit(self, n, bitpos):
        try:
            mask = 1 << bitpos
            if (n & mask) == 0:
                return 0
            else:
                return 1
        except Exception:
            return -1
        
    def modify_bit(self, n, bitpos, val):
        try:
            mask = 1 << bitpos
            return ((~mask) & 0xff & n) | (val << bitpos)
        except Exception:
            return -1

    def parse_map_element(self, eeprom_data, eeprom_ele, start_pos):
        value  = None
        offset = eeprom_ele.get('offset', 0) + start_pos
        size   = eeprom_ele.get('size')
        type   = eeprom_ele.get('type')
        decode = eeprom_ele.get('decode')
        bitmask = eeprom_ele.get('bitmask', None)
        bitvalue_map = eeprom_ele.get('bitvalue_map', None)
        eeprom_data_str = [hex(item)[2:].zfill(2) for item in eeprom_data]

        if type == 'enum':
            # Get the matched value
            if bitmask != None:
                key_value_int = eeprom_data[offset] & bitmask
            else:
                key_value_int = eeprom_data[offset]
            key_value_str = str(hex(key_value_int)[2:].zfill(2))
            value = decode.get(key_value_str, 'Unknown')

        elif type == 'bitmap':
            # Get the 'on' bitname
            bitvalue_dict = {}
            for bitname, bitinfo in sorted(decode.items()):
                bitinfo_offset = bitinfo.get('offset') + start_pos
                bitinfo_pos = bitinfo.get('bit')
                bitinfo_value = bitinfo.get('value')
                data = eeprom_data[bitinfo_offset]
                bit_value = self.test_bit(data, bitinfo_pos)
                if bitinfo_value != None:
                    if bit_value == bitinfo_value:
                        value = bitname
                        break
                elif bit_value == 1:
                    value = bitname
                    break

        elif type == 'bitvalue':
            # Get the value of the bit
            bitpos = eeprom_ele.get('bit')
            data = eeprom_data[offset]
            bitval = self.test_bit(data, bitpos)
            if bitvalue_map != None:
                value = bitvalue_map[bitval]
            else:
                value = ['Off', 'On'][bitval]

        elif type == 'func':
            # Call the decode func to get the value
            value = decode['func'](eeprom_data_str, offset, size)

        elif type == 'str':
            value = self.convert_hex_to_string(eeprom_data_str, offset,
                                offset + size)

        elif type == 'int':
            value = eeprom_data[offset]
            if bitmask != None:
                value = value & bitmask

        elif type == 'date':
            value = self.convert_date_to_string(eeprom_data_str, offset, size)

        elif type == 'hex':
            if size == 1:
                value = "0x{}".format(hex(eeprom_data[offset])[2:])
            elif size > 1:
                value = '-'.join(eeprom_data_str[offset:offset+size])

        return value

    # Recursively parses sff data into dictionary
    def parse_memory_map(self, eeprom_map, eeprom_data, start_pos):
        outdict = {}
        for name, meta_data in sorted(eeprom_map.items()):
            type = meta_data.get('type')
            # Initialize output value
            value_dict = {}
            value_dict['outtype'] = meta_data.get('outtype')
            value_dict['short_name'] = meta_data.get('short_name')
            if type != 'nested':
                data = self.parse_map_element(eeprom_data, meta_data, start_pos)
            else:
                nested_map = meta_data.get('decode')
                data = self.parse_memory_map(nested_map, eeprom_data, start_pos + meta_data.get('offset', 0))
            if data != None:
                value_dict['value'] = data
                outdict[name] = value_dict
        return outdict

    # def parse(self, eeprom_map_key, eeprom_data, start_pos):
    def parse(self, field_name):
        """ Example Return format:
        {'version': '1.0', 'data': {'Length50um(UnitsOf10m)':
        {'outtype': None, 'value': 8, 'short_name': None},
        'TransceiverCodes': {'outtype': None, 'value':
        {'10GEthernetComplianceCode': {'outtype': None, 'value':
        '10G Base-SR', 'short_name': None}}, 'short_name': None},
        'ExtIdentOfTypeOfTransceiver': {'outtype': None, 'value':
        'GBIC/SFP func defined by two-wire interface ID', 'short_name':
            None}, 'Length62.5um(UnitsOfm)': {'outtype': None,"""
        return_data = {}
        eeprom_map = memory_map[field_name]
        page = eeprom_map['page']
        addr = eeprom_map['addr']
        size = eeprom_map['size']
        status, eeprom_data = self.read(page=page, addr=addr, rd_len=size)
        eeprom_map = {
            field_name: eeprom_map
        }
        outdict = self.parse_memory_map(eeprom_map, eeprom_data, 0)
        return_data['data'] = outdict
        return_data['version'] = '0x52' # addr 0x2
        return return_data
    
    def output_process(self, field_keys, field_value):
        if field_value.get('outtype') == 'dict':
            ret = dict()
            for name, value in field_value.get('value').items():
                if field_keys and (name not in field_keys.keys()):
                    continue
                if field_keys:
                    ret[name] = self.output_process(field_keys.get(name, None), value)
                else:
                    ret[name] = self.output_process(None, value)
        else:
            ret = field_value['value']
        return ret

    def get_field(self, field_name):
        if isinstance(field_name, dict):
            ret_dict = dict()
            for field in field_name.keys():
                parse_output = self.parse(field)
                output_data = parse_output.get('data')
                field_data = output_data.get(field)
                ret_dict[field] = self.output_process(field_name[field], field_data)
            return ret_dict
        else:
            parse_output = self.parse(field_name)
            output_data = parse_output.get('data')
            field_data = output_data.get(field_name)
            return self.output_process(None, field_data)
    
    def modify_map_element(self, eeprom_data, eeprom_ele, start_pos, target_value):
        value  = None
        offset = eeprom_ele.get('offset') + start_pos
        size   = eeprom_ele.get('size')
        type   = eeprom_ele.get('type')
        decode = eeprom_ele.get('decode')
        bitmask = eeprom_ele.get('bitmask', None)
        bitvalue_map = eeprom_ele.get('bitvalue_map', None)

        if type == 'bitvalue':
            # Get the value of the bit
            bitpos = eeprom_ele.get('bit')
            data = eeprom_data[offset]
            if bitvalue_map != None:
                target_index = bitvalue_map.index(target_value)
            else:
                target_index = ['Off', 'On'].index(target_value)
            value = self.modify_bit(data, bitpos, target_index)
            eeprom_data[offset] = value & 0xff

        elif type == 'enum':
            # Get the matched value
            enum_value = 'Unknown'
            for key, value in decode.items():
                if value == target_value:
                    enum_value = key
                    break
            enum_value = int(enum_value, 16)
            eeprom_value = eeprom_data[offset]
            if bitmask != None:
                eeprom_value = eeprom_value & (~bitmask)
                eeprom_value = eeprom_value | enum_value
            else:
                eeprom_value = enum_value
            eeprom_data[offset] = eeprom_value & 0xff
        return eeprom_data

    def modify_memory_map(self, eeprom_map, eeprom_data, start_pos, target:dict):
        for target_key, target_value in target.items():
            for name, meta_data in sorted(eeprom_map.items()):
                if name != target_key:
                    continue
                type = meta_data.get('type')
                # Initialize output value
                value_dict = {}
                value_dict['outtype'] = meta_data.get('outtype')
                value_dict['short_name'] = meta_data.get('short_name')

                if type != 'nested':
                    data = self.modify_map_element(eeprom_data,
                                        meta_data, start_pos, target_value)
                else:
                    nested_map = meta_data.get('decode')
                    data = self.modify_memory_map(nested_map,
                                    eeprom_data, start_pos, target_value)
                eeprom_data = data
        return eeprom_data

    def modify(self, field_name, field_value):
        outdict = dict()
        eeprom_map = memory_map[field_name]
        page = eeprom_map['page']
        addr = eeprom_map['addr']
        size = eeprom_map['size']
        status, eeprom_data = self.read(page=page, addr=addr, rd_len=size)
        field_value = {
            field_name: field_value
        }
        eeprom_map = {
            field_name: eeprom_map
        }
        output_eeprom_data = self.modify_memory_map(eeprom_map, copy.deepcopy(eeprom_data), 0, field_value)
        if (eeprom_data != output_eeprom_data):
            outdict[field_name] = self.write(page=page, addr=addr, data=output_eeprom_data)
        else:
            outdict[field_name] = (True, "same as before")
        return outdict

    def set_field(self, field_name, field_value):
        return self.modify(field_name=field_name, field_value=field_value)
