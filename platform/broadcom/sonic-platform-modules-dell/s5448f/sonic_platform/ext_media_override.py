#!/usr/bin/env python3
"""
#############################################################################
#
# Module contains override methods on ext-media library
#
#############################################################################
"""

try:
    import re
    import sonic_platform_base.sonic_sfp.ext_media_api as ext_media
    from sonic_platform_base.sonic_sfp.ext_media_utils import read_eeprom_byte,\
           media_eeprom_address, read_eeprom_multi_byte, DEFAULT_NO_DATA_VALUE
    from sonic_platform_base.sonic_sfp.ext_media_handler_qsfp28 import qsfp28
    from sonic_platform_base.sonic_sfp.ext_media_handler_sfp28 import sfp28
    from sonic_platform_base.sonic_sfp.ext_media_handler_qsfp_dd import qsfp_dd
    from sonic_platform_base.sonic_sfp.ext_media_handler_qsfp28_dd import qsfp28_dd
    from sonic_platform_base.sonic_sfp.ext_media_handler_qsfp_plus import qsfp_plus
    from sonic_platform_base.sonic_sfp.ext_media_handler_sfp_plus import sfp_plus
    from sonic_platform_base.sonic_sfp.ext_media_common import get_form_factor_info

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


LINK_LEN_SUPPORTED_ADDR = media_eeprom_address(offset=142)
QSFP28_EXT_SPEC_COMPL_ADDR = media_eeprom_address(offset=192)
QSFP28_ENCODE_STRING_ADDR = media_eeprom_address(offset=244)
QSFP_PRODUCT_ID_ADDR = media_eeprom_address(offset=244)
QSFP28_LANE_IN_USE_ADDR = media_eeprom_address(offset=113)
QSFP_PLUS_PRODUCT_ID_ADDR = media_eeprom_address(offset=243)
QSFP28_DD_NOF_ADDR = media_eeprom_address(offset=87)
QSFP28_DD_MEDIA_LANE_COUNT = media_eeprom_address(offset=88)
QSFP28_DD_MEDIA_LANE_ASSIGNMENT = media_eeprom_address(offset=89)
SFP_PRODUCT_ID_ADDR = media_eeprom_address(offset=100)
QSFP_DD_NOF_ADDR = media_eeprom_address(offset=87)
QSFP_ETH_SPEC_COMPL_ADDR = media_eeprom_address(offset=131)

QSFP28_GET_DISPLAY_NAME = qsfp28.get_display_name
QSFP28_GET_CABLE_LENGTH_DETAILED = qsfp28.get_cable_length_detailed
SFP28_GET_DISPLAY_NAME = sfp28.get_display_name
SFP28_GET_CABLE_LENGTH_DETAILED = sfp28.get_cable_length_detailed
SFP_PLUS_GET_DISPLAY_NAME = sfp_plus.get_display_name
SFP_PLUS_GET_CABLE_LENGTH_DETAILED = sfp_plus.get_cable_length_detailed
QSFP_DD_GET_DISPLAY_NAME = qsfp_dd.get_display_name
QSFP28_DD_GET_DISPLAY_NAME = qsfp28_dd.get_display_name
QSFP_PLUS_GET_DISPLAY_NAME = qsfp_plus.get_display_name
QSFP_PLUS_GET_CABLE_LENGTH_DETAILED = qsfp_plus.get_cable_length_detailed
QSFP_PLUS_GET_CABLE_BREAKOUT = qsfp_plus.get_cable_breakout

SFP_KEYCODE_ADDR = media_eeprom_address(offset=96)
QSFP_KEYCODE_ADDR = media_eeprom_address(offset=240)

def ext_media_override():
    """
    Override methods in ext-media module
    """
    qsfp28.get_display_name = qsfp28_display_name_override
    qsfp28.get_cable_length_detailed = qsfp28_cable_length_override
    sfp28.get_display_name = sfp28_display_name_override
    sfp28.get_cable_length_detailed = sfp28_cable_length_override
    sfp_plus.get_display_name = sfp_plus_display_name_override
    sfp_plus.get_cable_length_detailed = sfp_plus_cable_length_override
    qsfp_dd.get_display_name = qsfp_dd_display_name_override
    qsfp28_dd.get_display_name = qsfp28_dd_display_name_override
    qsfp_plus.get_display_name = qsfp_plus_display_name_override
    qsfp_plus.get_cable_length_detailed = qsfp_plus_cable_length_override
    qsfp_plus.get_cable_breakout = qsfp_plus_cable_breakout_override

    ext_media.is_qualified = is_qualified_override

def qsfp28_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of qsfp28
    """
    name = QSFP28_GET_DISPLAY_NAME(obj, eeprom)
    if 'ER4' in name and read_eeprom_byte(eeprom, LINK_LEN_SUPPORTED_ADDR) == 30:
        name += '-LITE'
    if name.find('BIDI4') >= 0:
        if ((read_eeprom_byte(eeprom, QSFP28_EXT_SPEC_COMPL_ADDR) == 0x21) and \
            (read_eeprom_byte(eeprom, QSFP28_ENCODE_STRING_ADDR) == 0x0d)):
            name = name.replace('BIDI4', 'SR1.2')
        else:
            name = name.replace('BIDI4', 'BIDI')
    if name.find('AOC') >= 0:
        if read_eeprom_byte(eeprom, QSFP28_EXT_SPEC_COMPL_ADDR) == 0x18:
            name = name.replace('AOC', 'AOC-NOF')
    if name.find('100GBASE-SR4') >= 0:
        if (read_eeprom_byte(eeprom, QSFP28_ENCODE_STRING_ADDR) >> 4) == 0x5:
            name += '-NOF'
    if name.find('100GBASE-CR4') >= 0:
        if (read_eeprom_byte(eeprom, QSFP28_EXT_SPEC_COMPL_ADDR) == 0x40) and \
                (read_eeprom_byte(eeprom, QSFP28_LANE_IN_USE_ADDR) == 0x0c):
            name = name.replace('QSFP28', 'QSFP56-DEPOP')
            name = name.replace('100GBASE-CR4', '100GBASE-CR2')
        if (read_eeprom_byte(eeprom, QSFP28_EXT_SPEC_COMPL_ADDR) == 0x40) and \
                (read_eeprom_byte(eeprom, QSFP28_LANE_IN_USE_ADDR) == 0x00):
            name = name.replace('QSFP28', 'QSFP56')
            name = name.replace('100GBASE-CR4', '200GBASE-CR4')

    cable_length = read_eeprom_byte(eeprom, QSFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 6:
        name = name.replace('1.0', '0.5')
    elif(cable_length >> 4) == 8:
        name = name.replace('2.0', '1.5')
    elif(cable_length >> 4) == 9:
        name = name.replace('3.0', '2.5')
    return name

def qsfp28_cable_length_override(obj, eeprom):
    """
    Override function for get_cable_length_detailed of qsfp28
    """
    cable_length = read_eeprom_byte(eeprom, QSFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 6:
        return 0.5
    if(cable_length >> 4) == 8:
        return 1.5
    if(cable_length >> 4) == 9:
        return 2.5
    return QSFP28_GET_CABLE_LENGTH_DETAILED(obj, eeprom)

def sfp28_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of sfp28
    """
    name = SFP28_GET_DISPLAY_NAME(obj, eeprom)
    cable_length = read_eeprom_byte(eeprom, SFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 0x5:
        name += '-NOF'
    elif(cable_length >> 4) == 11:
        name = name.replace('1.0', '0.5')
    elif(cable_length >> 4) == 12:
        name = name.replace('2.0', '1.5')
    elif(cable_length >> 4) == 13:
        name = name.replace('3.0', '2.5')
    return name

def sfp28_cable_length_override(obj, eeprom):
    """
    Override function for get_cable_length_detailed of sfp28
    """
    cable_length = read_eeprom_byte(eeprom, SFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 11:
        return 0.5
    if(cable_length >> 4) == 12:
        return 1.5
    if(cable_length >> 4) == 13:
        return 2.5
    return SFP28_GET_CABLE_LENGTH_DETAILED(obj, eeprom)

def sfp_plus_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of sfp_plus
    """
    name = SFP_PLUS_GET_DISPLAY_NAME(obj, eeprom)
    cable_length = read_eeprom_byte(eeprom, SFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 11:
        name = name.replace('1.0', '0.5')
    elif(cable_length >> 4) == 14:
        name = name.replace('2.0', '1.5')
    elif(cable_length>> 4) == 15:
        name = name.replace('3.0', '2.5')
    return name

def sfp_plus_cable_length_override(obj, eeprom):
    """
    Override function for get_cable_length_detailed of sfp_plus
    """
    cable_length = read_eeprom_byte(eeprom, SFP_PRODUCT_ID_ADDR)
    if(cable_length >> 4) == 11:
        return 0.5
    if(cable_length >> 4) == 14:
        return 1.5
    if(cable_length >> 4) == 15:
        return 2.5
    return SFP_PLUS_GET_CABLE_LENGTH_DETAILED(obj, eeprom)

def qsfp28_dd_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of qsfp28_dd
    """
    name = QSFP28_DD_GET_DISPLAY_NAME(obj, eeprom)
    # For 2x100G QSFP28-DD, there is no SR8 available.  SR8 requires one MAC from the host.
    # For the 2x100G QSFP28-DD port, we don't have 200G NRZ MAC or one single MAC to cover this
    # 200G NRZ (or 2x100G NRZ).  Because of lack of a single MAC, we have to use 2SR4 or 2xSR4.
    # Byte 88,89 values will tell us if this is a breakout cable or point to point cable.
    # Based on byte 88,89 values, update the display name
    cable_type_keycode = [0x44, 0x11]
    eeprom_keycode = read_eeprom_multi_byte(eeprom, QSFP28_DD_MEDIA_LANE_COUNT,\
                media_eeprom_address(offset=QSFP28_DD_MEDIA_LANE_COUNT.offset+2))
    if name.find('200GBASE-SR8') != -1:
        if eeprom_keycode == cable_type_keycode:
            name = name.replace('200GBASE-SR8', '2x(100GBASE-SR4)')
        else:
            name = name.replace('200GBASE-SR8', '200GBASE-2SR4')
    elif name.find('200GBASE-CR8') != -1:
        if eeprom_keycode == cable_type_keycode:
            name = name.replace('200GBASE-CR8', '2x(100GBASE-CR4)')
        else:
            name = name.replace('200GBASE-CR8', '200GBASE-2CR4')
    # for optical cable
    if name.find('AOC') != -1:
        if (read_eeprom_byte(eeprom, QSFP28_DD_NOF_ADDR)) == 0x1:
            name = name.replace('AOC', 'AOC-NOF')
    # Appending "DUALRATE" in the display name for dualrate optics
    dualrate_keycode = [0xDF, 0x10, 0x02, 0x11, 0x18]
    eeprom_keycode = read_eeprom_multi_byte(eeprom, \
        QSFP_KEYCODE_ADDR, \
        media_eeprom_address(offset=QSFP_KEYCODE_ADDR.offset+5))
    if eeprom_keycode == dualrate_keycode:
        name += '-DUALRATE'
    return name

def qsfp_dd_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of qsfp_dd
    """
    name = QSFP_DD_GET_DISPLAY_NAME(obj, eeprom)
    # for optical cable
    # do not add NOF for 8x10G
    if name.find('AOC') != -1 and name.find('8x(10GBASE') < 0:
        if (read_eeprom_byte(eeprom, QSFP_DD_NOF_ADDR)) == 0x1:
            name = name.replace('AOC', 'AOC-NOF')
    return name

def qsfp_plus_display_name_override(obj, eeprom):
    """
    Override function for get_display_name of qsfp_plus
    """
    name = QSFP_PLUS_GET_DISPLAY_NAME(obj, eeprom)
    vendor_name = qsfp_plus.get_vendor_name(obj, eeprom)
    if vendor_name in ("DELL", "DELL EMC"):
        #byte 243 upper nibble is the wavelength. wave length 7 is BIDI
        if (read_eeprom_byte(eeprom, QSFP_PLUS_PRODUCT_ID_ADDR) >> 4) == 0x7:
            name = re.sub(r'(?<=40GBASE-).*', "BIDI", name)
        # As per Spec, check for byte 131 and 192 to confirm it is a SM4 media.
        # Ensure AOC cable (eg:40GBASE-SR4-AOC-10.0M) is not wrongly matched.
        if (read_eeprom_byte(eeprom, QSFP28_EXT_SPEC_COMPL_ADDR) == 0x0) and \
           (read_eeprom_byte(eeprom, QSFP_ETH_SPEC_COMPL_ADDR) == 0x0) and \
           (name == 'QSFP+ 40GBASE-SR4'):
            keycode = read_eeprom_multi_byte(eeprom, QSFP_KEYCODE_ADDR,\
                media_eeprom_address(offset=QSFP_KEYCODE_ADDR.offset+2))
            # Check for Magic Keycode byte pattern
            if ((keycode[0] == 0xdf) or (keycode[0] == 0x0f)) and \
                ((keycode[1] == 0x10)):
                name = name.replace('SR4', 'SM4')
    cable_length = read_eeprom_byte(eeprom, QSFP_PRODUCT_ID_ADDR)
    if (cable_length >> 4) == 6:
        name = name.replace('1.0', '0.5')
    elif (cable_length >> 4) == 11:
        name = name.replace('2.0', '1.5')
    elif (cable_length>> 4) == 12:
        name = name.replace('3.0', '2.5')
    return name

def qsfp_plus_cable_length_override(obj, eeprom):
    """
    Override function for get_cable_length_detailed of qsfp_plus
    """
    cable_length = read_eeprom_byte(eeprom, QSFP_PRODUCT_ID_ADDR)
    if (cable_length >> 4) == 6:
        return 0.5
    if (cable_length >> 4) == 11:
        return 1.5
    if (cable_length>> 4) == 12:
        return 2.5
    return QSFP_PLUS_GET_CABLE_LENGTH_DETAILED(obj, eeprom)

def qsfp_plus_cable_breakout_override(obj, eeprom):
    """
    Override function for get_cable_breakout_detailed of qsfp_plus
    """
    f10_copper_keycode = [0x0f, 0x10, 0x00, 0x93]
    eeprom_keycode = read_eeprom_multi_byte(eeprom, QSFP_KEYCODE_ADDR,\
                media_eeprom_address(offset=QSFP_KEYCODE_ADDR.offset+5))
    if eeprom_keycode[:4] == f10_copper_keycode:
        if eeprom_keycode[4]&0xF == 2:
            return '1x4'
    return QSFP_PLUS_GET_CABLE_BREAKOUT(obj, eeprom)

def is_qualified_override(info_dict, _, sfp_obj):
    """
    We check the media part info against the set of supported parts from the given platform
    """
    try:
        # First read 256 bytes for caching
        eeprom_bytes = sfp_obj.get_eeprom_cache_raw()
        media_ff, _ = get_form_factor_info(eeprom_bytes)
        vendor_name = info_dict.get('vendor_name', DEFAULT_NO_DATA_VALUE)

        if vendor_name in ("DELL", "DELL EMC"):
            return True
        if media_ff == 'SFP':
            # Consider 1G medias as Dell Qualified
            return True
        #Read key code value from QSFP/OSFP/SFP to determine if its a qualified media
        if media_ff.startswith(('Q', 'O')):
            # Check if form factor starts with 'Q' or 'O' and Keycode is 0xDF10
            keycode = read_eeprom_multi_byte(eeprom_bytes, QSFP_KEYCODE_ADDR,\
                media_eeprom_address(offset=QSFP_KEYCODE_ADDR.offset+6))
            if ((keycode[0] == 0xdf) or (keycode[0] == 0x0f)) and \
                ((keycode[1] == 0x10)):
                return True
            else:
                # Convert the keycode to string and Check for "DSPLPO" 2DR4 optic.
                keycode_ascii = bytes(keycode).decode('ascii')
                if keycode_ascii == "DSPLPO":
                    return True
        elif media_ff.startswith('S'):
            # Form factor starts with 'S' and Keycode is 0xDF10
            keycode = read_eeprom_multi_byte(eeprom_bytes, SFP_KEYCODE_ADDR,\
                media_eeprom_address(offset=SFP_KEYCODE_ADDR.offset+2))
            if ((keycode[0] == 0xdf) or (keycode[0] == 0x0f)) and \
                ((keycode[1] == 0x10)):
                return True
    except:
        pass

    return False
