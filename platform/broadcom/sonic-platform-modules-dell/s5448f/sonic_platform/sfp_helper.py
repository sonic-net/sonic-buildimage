#!/usr/bin/env python3

"""
# DELLEMC
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
"""

try:
    import os
    import time
    import struct
    import mmap
    import syslog
    import math
    import multiprocessing
    from sonic_platform_base.sonic_sfp.sffbase import sffbase
    from sonic_platform_base.sfp_standard import SfpStandard
    from swsscommon import swsscommon
    from .sfp_phy_helper import \
        aq_set_speed, \
        aq_set_admin_state, \
        aq_set_autoneg, \
        bcm848xx_set_speed, \
        bcm848xx_set_admin_state, \
        bcm848xx_set_autoneg, \
        bcm845xx_set_speed, \
        bcm845xx_set_autoneg, \
        PHY_SPEED_AUTONEG, \
        bcm845xx_set_admin_state, \
        mvl88e1111_set_admin_state
    from .sfp_helper_dom import inf8628PfmDom

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

MEDIA_TYPE_OFFSET = 0
MEDIA_TYPE_WIDTH = 1

QSFP_QSA28_OUI_OFFSET = 64
QSFP_QSA28_OUI_WIDTH = 3

SFP_TYPE_LIST = [
    '03' # SFP/SFP+/SFP28 and later
]
SFP_DD_TYPE_LIST = [
    '1a' #SFP-DD Type
]
QSFP_TYPE_LIST = [
    '0c', # QSFP
    '0d', # QSFP+ or later
    '11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '18' #QSFP-DD Type
]
OSFP_TYPE_LIST = [
    '19' # OSFP 8X Type
]

DF10_ENCODING_OFFSET = 96
DF10_ENCODING_SIZE = 7
DF10_ENCODING_MAGIC_OLD = '0f10' # Force10 for old optics
DF10_ENCODING_MAGIC_NEW = 'df10' # Dell Force10
DF10_30M_MULTIGIG_GEN2_5_PHY = '1111'  # Gen 2.5 optics
DF10_30M_MULTRATE_PHY = 0x21
DF10_30M_MULTIGIG_AQ_PHY = 0x28
DF10_30M_MULTIGIG_BCM_PHY = 0x29

SFF8472_EXT_COMPL_CODE_OFFSET = 36

class SfphelperStandard(SfpStandard):
    """
    DELLEMC Platform-common Sfp class
    """
    def __init__(self, index, sfp_type, eeprom_path):
        SfpStandard.__init__(self)
        #sfp_type is the native port type and media_type is the transceiver type
        self.sffbase_obj = sffbase()
        self.sffbase_obj.version = '1.0'
        self.sfp_type = sfp_type
        self.media_type = self.sfp_type

        self.index = index
        self.sfp_eeprom_path = eeprom_path
        self.initialized = False
        self.delay_init = False
        self.faulty_media = False
        self.error_count = 0
        self.phy_config_supported = False
        self.phy_config_invalid_speed = False
        # Dict {<config>: {'method':<handler>, 'value':<cached_value>, 'param':<list_of_params>}}
        self.phy_config_info = {}
        self.dom_obj = inf8628PfmDom()
        self.phy_remove = None

    @property
    def port_index(self):
        """ Returns SFP port index"""
        return self.index

    @property
    def eeprom_path(self):
        """ Returns SFP EEPROM path"""
        return self.sfp_eeprom_path

    def get_eeprom_sysfs_path(self):
        """ Returns media eeprom path """
        return self.sfp_eeprom_path

    def _initialize_media(self, delay_init):
        if delay_init and self.error_count == 0:
            time.sleep(2) # Media might have just got inserted
        if self.set_media_type() is not None:
            self.reinit_sfp_driver()
            self.initialized = True
            return True
        return False

    @staticmethod
    def _pci_mem_read(_mm, offset):
        _mm.seek(offset)
        read_data_stream = _mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mem_val = str(reg_val)[1:-2]
        # print "reg_val read:%x"%reg_val
        return mem_val

    @staticmethod
    def _pci_mem_write(_mm, offset, data):
        _mm.seek(offset)
        # print "data to write:%x"%data
        _mm.write(struct.pack('I', data))

    def pci_set_value(self, resource, val, offset):
        """ Set Value on FPGA """
        filed = os.open(resource, os.O_RDWR)
        _mm = mmap.mmap(filed, 0)
        val = self._pci_mem_write(_mm, offset, val)
        _mm.close()
        os.close(filed)
        return val

    def pci_get_value(self, resource, offset):
        """ Get Value from FPGA """
        filed = os.open(resource, os.O_RDWR)
        _mm = mmap.mmap(filed, 0)
        val = self._pci_mem_read(_mm, offset)
        _mm.close()
        os.close(filed)
        return val

    @staticmethod
    def _read_eeprom_bytes(eeprom_path, offset, num_bytes):
        eeprom_raw = []
        try:
            eeprom = open(eeprom_path, mode="rb", buffering=0)
            for _ in range(0, num_bytes):
                eeprom_raw.append("0x00")
            eeprom.seek(offset)
            raw = eeprom.read(num_bytes)
        except EnvironmentError as error:
            eeprom.close()
            syslog.syslog(syslog.LOG_ERR, "Error: Can't read EEPROM offset %s" % str(error))
            return None

        try:
            for byte in range(0, num_bytes):
                eeprom_raw[byte] = hex(raw[byte])[2:].zfill(2)
        except ValueError as error:
            eeprom.close()
            syslog.syslog(syslog.LOG_ERR, "Error: Conversion of EEPROM bytes failed %s"%str(error))
            return None

        eeprom.close()
        return eeprom_raw

    def _get_presence(self):
        # Check for invalid port_num
        mask = {'QSFP' : (1 << 4), 'SFP' : (1 << 0), 'QSFP_DD' : (1 << 4)}
        # Port offset starts with 0x4004
        port_offset = 16388 + ((self.index-1) * 16)
        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)
        # ModPrsL is active low
        if (self.sfp_type in mask) and (reg_value & mask[self.sfp_type] == 0):
            return True
        return False

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        Returns : True if sfp is present and false if it is absent
        """
        try:
            # ModPrsL is active low
            if self._get_presence():
                if not self.initialized:
                    if self.faulty_media:
                        return False
                    if not self._initialize_media(self.delay_init):
                        self.error_count += 1
                        if self.error_count > 5:
                            syslog.syslog(syslog.LOG_ERR, \
                                "Faulty Media at Port{}. Masked Port {} presence"\
                                .format(self.index, self.index))
                            self.faulty_media = True
                        return False
                return True

            self.delay_init = True
            if self.initialized or self.faulty_media:
                self.initialized = False
                self.faulty_media = False
                self.error_count = 0

        except ValueError:
            pass

        return False

    def get_reset_status(self):
        """
        Retrives the reset status of SFP
        """
        reset_status = False
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset status
                mask = 1 << 4
                reset_status = not reg_value & mask
        except ValueError:
            pass

        return reset_status

    def get_qsa_adapter_type_fpga(self):
        """
        Reset the SFP and read the EEPROM of adaptor to identify the type of the QSA adapter.
        """
        media_type = 'N/A'
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset
                mask = 1 << 4

                # ResetL is active low
                reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)

                eeprom_raw = self._read_eeprom_bytes(self.sfp_eeprom_path, QSFP_QSA28_OUI_OFFSET, \
                                QSFP_QSA28_OUI_WIDTH)
                if eeprom_raw is not None:
                    qsfp_qsa28_outi_val = ['00', '02', 'c9']
                    if eeprom_raw == qsfp_qsa28_outi_val:
                        media_type = 'QSA28'
                    else:
                        media_type = 'QSA'
                reg_value = reg_value | mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: Get QSA adapter type failed: %s" % str(error))
            return media_type
        return media_type

    def get_qsa_adapter_type_cpld(self):
        """
        Reset the SFP and read the EEPROM of adaptor to identify the type of the QSA adapter.
        """
        media_type = 'N/A'
        QSFP_PORT_START = 53
        try:
            if self.sfp_type == 'QSFP':
                qsfp_rst = int(self._get_cpld_register('qsfp_rst'), 16)
                if qsfp_rst == 'ERR':
                    return media_type
                bit_mask = 1 << (self.index - QSFP_PORT_START)
                qsfp_rst = qsfp_rst & ~bit_mask
                rval = self._set_cpld_register('qsfp_rst', qsfp_rst)
                if rval == 'ERR':
                    return media_type
                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)
                eeprom_raw = self._read_eeprom_bytes(self.sfp_eeprom_path, QSFP_QSA28_OUI_OFFSET, \
                                QSFP_QSA28_OUI_WIDTH)
                if eeprom_raw is not None:
                    qsfp_qsa28_outi_val = ['00', '02', 'c9']
                    if eeprom_raw == qsfp_qsa28_outi_val:
                        media_type = 'QSA28'
                    else:
                        media_type = 'QSA'
                qsfp_rst = qsfp_rst | bit_mask
                rval = self._set_cpld_register('qsfp_rst', qsfp_rst)
                if rval == 'ERR':
                    return media_type
                # Sleep 1 millisecond to allow it to settle
                time.sleep(1/1000)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: Get QSA adapter type failed: %s" % str(error))
            return media_type
        return media_type

    def get_qsa_adapter_type(self):
        try:
            self.BASE_RES_PATH
        except AttributeError:
            return self.get_qsa_adapter_type_cpld()
        else:
            return self.get_qsa_adapter_type_fpga()

    def reset(self):
        """
        Reset the SFP and returns all user settings to their default state
        """
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset
                mask = 1 << 4

                # ResetL is active low
                reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

                # Sleep 1 second to allow it to settle
                time.sleep(1)

                reg_value = reg_value | mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: SFP Reset failed: %s" % str(error))
            return  False
        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode(low power mode) of SFP
        """
        lpmode_state = False
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 6th bit for lpmode
                mask = (1 << 6)

                if reg_value & mask == 0:
                    lpmode_state = False
                else:
                    lpmode_state = True

        except (ValueError, AttributeError):
            pass
        return lpmode_state

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        try:
            if self.sfp_type == 'QSFP' or self.sfp_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 6th bit for lowpower mode
                mask = 1 << 6

                # LPMode is active high; set or clear the bit accordingly
                if lpmode is True:
                    reg_value = reg_value | mask
                else:
                    reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
        except ValueError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: SFP lpmode set failed: %s" % str(error))
            return False
        return True

    @staticmethod
    def tx_disable(_tx_disable):
        """
        Disable SFP TX for all channels
        """
        return False

    def hard_tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels by pulling up hard TX_DISABLE pin

        Args:
        tx_disable : A Boolean, True to enable tx_disable mode, False to disable
        tx_disable mode.

        Returns:
        A boolean, True if tx_disable is set successfully, False if not
        """
        ret = False
        if self.sfp_type == 'SFP':
            try:
                # Port offset starts with 0x4004
                port_offset = 0x4000 + ((self.index-1) * 16)
                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)
                mask = 1
                reg_value = (reg_value | mask) if tx_disable else (reg_value & ~mask)
                self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
                syslog.syslog(syslog.LOG_INFO, \
                                  "Port%d TX Disable :%d" % (self.index, tx_disable))
                ret = True
            except ValueError:
                ret = False
        return ret

    @staticmethod
    def tx_disable_channel(_channel, _disable):
        """
        Sets the tx_disable for specified SFP channels
        """
        return False

    @staticmethod
    def set_power_override(_power_override, _power_set):
        """
        Sets SFP power level using power_override and power_set
        """
        return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()
        return not reset

    def get_port_form_factor(self):
        """
        Retrieves the native port type
        """
        return self.sfp_type

    def set_media_type(self):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._read_eeprom_bytes(self.sfp_eeprom_path, MEDIA_TYPE_OFFSET, \
                        MEDIA_TYPE_WIDTH)
        if eeprom_raw is not None:
            if eeprom_raw[0] in SFP_TYPE_LIST:
                self.media_type = 'SFP'
            elif eeprom_raw[0] in SFP_DD_TYPE_LIST:
                self.media_type = 'SFP_DD'
            elif eeprom_raw[0] in QSFP_TYPE_LIST:
                self.media_type = 'QSFP'
            # OSFP and QSFP_DD uses same drv in SFP standard    
            elif eeprom_raw[0] in QSFP_DD_TYPE_LIST or eeprom_raw[0] in OSFP_TYPE_LIST:
                self.media_type = 'QSFP_DD'
        else:
            self.media_type = None

        return self.media_type

    def _is_cu_media(self):
        cu_media = self._is_1g_cu_media()
        if not cu_media:
            data = self._read_eeprom_bytes(self.sfp_eeprom_path, 36, 1)
            cu_media = data is not None and int(data[0], 16) in [0x1c, 0x1d, 0x1e]
                                                          #10G BASE-T, 5G BASE-T, 2.5G BASE-T
        return cu_media

    def _is_1g_cu_media(self):
        data = self._read_eeprom_bytes(self.sfp_eeprom_path, 6, 1)
        cu_media = data is not None and (int(data[0], 16) & 0x08) != 0  # 1000 BASE-T
        return cu_media

    def phy_config_info_init_aq(self):
        self.phy_config_invalid_speed = False
        self.phy_config_supported = True
        default = { \
            'speed': {'method':aq_set_speed, 'params':None}, \
            'admin_status': {'method':aq_set_admin_state, 'params':None}, \
            'autoneg': {'method':aq_set_autoneg, 'params':None} \
        }

        for config in default:
            if self.phy_config_info.get(config, None) is None:
                self.phy_config_info[config] = default[config]
            else:
                for field, value in list(default[config].items()):
                    self.phy_config_info[config][field] = value

        # Power down during init
        self._phy_config_set('admin_status', 'down')

    def phy_config_info_init_bcm848xx(self):
        self.phy_config_invalid_speed = False
        self.phy_config_supported = True
        self.phy_remove = bcm848xx_set_autoneg
        default = { \
            'speed': {'method':bcm848xx_set_speed, 'params':None}, \
            'admin_status': {'method':bcm848xx_set_admin_state, 'params':None}, \
            'autoneg': {'method':bcm848xx_set_autoneg, 'params':None} \
        }

        for config in default:
            if self.phy_config_info.get(config, None) is None:
                self.phy_config_info[config] = default[config]
            else:
                for field, value in list(default[config].items()):
                    self.phy_config_info[config][field] = value

        # Power down during init
        if not swsscommon.WarmStart().isWarmStart():
            self._phy_config_set('admin_status', 'down')

    def phy_config_info_init_bcm845xx(self):
        self.phy_config_invalid_speed = False
        self.phy_config_supported = True
        default = { \
            'speed': {'method':bcm845xx_set_speed, 'params':None}, \
            'admin_status': {'method':bcm845xx_set_admin_state, 'params':None}, \
            'autoneg': {'method':bcm845xx_set_autoneg, 'params':None} \
        }

        for config in default:
            if self.phy_config_info.get(config, None) is None:
                self.phy_config_info[config] = default[config]
            else:
                for field, value in list(default[config].items()):
                    self.phy_config_info[config][field] = value

        # Power down during init
        if not swsscommon.WarmStart().isWarmStart():
            self._phy_config_set('admin_status', 'down')

    # Marvell phy
    def phy_config_info_init_mvl88e1111(self):
        self.phy_config_invalid_speed = False
        self.phy_config_supported = True
        self.phy_remove = mvl88e1111_set_admin_state
        default = { \
            'admin_status': {'method':mvl88e1111_set_admin_state, 'params':None} \
        }

        for config in default:
            if self.phy_config_info.get(config, None) is None:
                self.phy_config_info[config] = default[config]
            else:
                for field, value in list(default[config].items()):
                    self.phy_config_info[config][field] = value

        # Power down during init
        if not swsscommon.WarmStart().isWarmStart():
            self._phy_config_set('admin_status', 'down')

    def phy_config_info_init_default(self):
        self.phy_config_invalid_speed = False
        self.phy_config_supported = False
        for config in self.phy_config_info:
            self.phy_config_info[config]['method'] = None
            self.phy_config_info[config]['params'] = None

    def _media_phy_init(self):
        try:
            if self.media_type == 'SFP':
                encoding_raw = self._read_eeprom_bytes(self.sfp_eeprom_path, \
                                                       DF10_ENCODING_OFFSET, \
                                                       DF10_ENCODING_SIZE)
                if encoding_raw is None:
                    return
                magic_str = ''.join(encoding_raw[:2])
                sfp_product_id2 = int(encoding_raw[4], 16)

                if magic_str in (DF10_ENCODING_MAGIC_OLD, DF10_ENCODING_MAGIC_NEW):
                    if sfp_product_id2 == DF10_30M_MULTRATE_PHY:
                        self.phy_config_info_init_bcm845xx()
                        # Supported speed for this optics
                        self.phy_config_info['speed']['params'] = ['10000', '1000']
                        return
                    elif sfp_product_id2 == DF10_30M_MULTIGIG_AQ_PHY:
                        self.phy_config_info_init_aq()
                        self.hard_tx_disable(False)
                        # Supported speed for this optics
                        self.phy_config_info['speed']['params'] = ['10000', '1000']
                        return
                    elif (sfp_product_id2 == DF10_30M_MULTIGIG_BCM_PHY) or \
                         ((DF10_30M_MULTIGIG_GEN2_5_PHY == ''.join(encoding_raw[3:5])) and self._is_cu_media()):
                        self.phy_config_info_init_bcm848xx()
                        self.hard_tx_disable(False)
                        # Supported speed for this optics
                        self.phy_config_info['speed']['params'] = ['10000', '1000']
                        return
                if self._is_1g_cu_media():
                    self.phy_config_info_init_mvl88e1111()
                    return
        except (EnvironmentError, ValueError):
            pass

        self.phy_config_info_init_default()

    def media_phy_init(self, default, logical_port):
        # Initialize parameters for SFP PHY configuration
        if default:
            self.phy_config_info_init_default()
            return True

        self._media_phy_init()
        if self.phy_config_supported:
            for config in self.phy_config_info:
                data = self.phy_config_info[config].get('value', None)
                if data is not None:
                    if not self._phy_config_set(config, data, logical_port):
                        return False

        return True

    def media_phy_remove(self, logical_port):
        if self.phy_remove is not None:
        # if media is removed, remove port from autoneg port list
            self.phy_remove(logical_port, self.sfp_eeprom_path, False)
            self.phy_remove = None
    def _phy_config_set(self, config, data, logical_port=None):
        ret = True

        try:
            if config in ('speed', 'autoneg'):
                if config == 'speed':
                    speed_str = data
                    # Consider autoneg as 'off', if the filed is not present
                    if self.phy_config_info.get('autoneg', None) is None or \
                        self.phy_config_info['autoneg'].get('value', None) is None:
                        autoneg_str = 'off'
                    else:
                        autoneg_str = self.phy_config_info['autoneg']['value']
                else:
                    speed_str = self.phy_config_info['speed']['value']
                    autoneg_str = data

                if speed_str not in self.phy_config_info['speed']['params']:
                    syslog.syslog(syslog.LOG_INFO, \
                                  "Port%d SFP PHY unsupported speed(%s) or autoneg(%s) is set" \
                                  % (self.index, speed_str, autoneg_str))
                    if not self.phy_config_invalid_speed:
                        self.phy_config_invalid_speed = True
                        # Disable Tx when unsupported speed is configured
                        if self.phy_config_info['admin_status']['value'] == 'up':
                            ret = self.phy_config_info['admin_status']['method']\
                                    (self.sfp_eeprom_path, False)
                else:
                    autoneg = (autoneg_str == "on")
                    if autoneg:
                        speed = PHY_SPEED_AUTONEG
                    else:
                        speed = int(speed_str)
                    # if autoneg is false , first stop the previous autoneg thread if any
                    if not autoneg:
                        ret = self.phy_config_info['autoneg']['method']\
                                (logical_port, self.sfp_eeprom_path, autoneg)
                    ret = self.phy_config_info['speed']['method'](self.sfp_eeprom_path, speed)
                    if ret and self.phy_config_invalid_speed:
                        self.phy_config_invalid_speed = False
                        #Restore the admin status
                        if self.phy_config_info['admin_status']['value'] == 'up':
                            ret = self.phy_config_info['admin_status']['method']\
                                    (self.sfp_eeprom_path, True)
                    if autoneg:
                        ret = self.phy_config_info['autoneg']['method']\
                                    (logical_port, self.sfp_eeprom_path, autoneg)

            elif config == 'admin_status':
                # Set admin status only when valid speed is configured
                if not self.phy_config_invalid_speed:
                    enable = (data == 'up')
                    ret = self.phy_config_info[config]['method']\
                                (logical_port, self.sfp_eeprom_path, enable)
            else:
                syslog.syslog(syslog.LOG_INFO, \
                              "Port%d SFP PHY unhandled config '%s=%s'" \
                              % (self.index, config, data))
        except (KeyError, TypeError):
            # KeyError can occur when dict is yet to be populated
            # TypeError can occur when no 'method' is defined for any of the config
            pass
        except EnvironmentError as err:
            syslog.syslog(syslog.LOG_ERR, \
                          "Port%d SFP PHY exception '%s' for config %s=%s" \
                          % (self.index, str(err), config, data))

        if not ret:
            syslog.syslog(syslog.LOG_ERR, "Port%d SFP PHY config failed for %s=%s" \
                          % (self.index, config, data))

        return ret

    @staticmethod
    def _process_appldb_update(_field, _value):
        return True

    def appldb_update_notify(self, field, value, logical_port):
        ret = True

        # Supported field list for configuring the PHY in media
        if field in ['speed', 'admin_status', 'autoneg']:
            if self.phy_config_info.get(field, None) is not None:
                val = self.phy_config_info[field].get('value', None)
                # Redundant, so skip
                if val == value:
                    return True
                self.phy_config_info[field]['value'] = value
            else:
                self.phy_config_info[field] = {'value': value}

            if self.phy_config_supported:
                ret = self._phy_config_set(field, value, logical_port)

        # APPL_DB update processing
        self._process_appldb_update(field, value)

        return ret

    def reinit_sfp_driver(self):
        """
        Changes the driver based on media type detected
        """
        i2c_bus = self.sfp_eeprom_path[27:].split('/')[0]
        del_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/name".format(i2c_bus)

        if not os.path.isfile(driver_path):
            print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as filed:
                driver_name = filed.read()
                driver_name = driver_name.rstrip('\r\n')
                driver_name = driver_name.lstrip(" ")


            media_driver = {'SFP-CU' : 'copper', 'SFP' : 'optoe2', 'QSFP' : 'optoe1',
                            'QSFP_DD' : 'optoe3', 'SFP_DD' : 'optoe3'}
            media_type = self.media_type
            if media_type == 'SFP' and self._is_cu_media():
                media_type = 'SFP-CU'
            if driver_name != media_driver[media_type]:
                #Delete the current device if driver type is different
                execute_cmd = "0x50"
                with open(del_sfp_path, 'w') as file_obj:
                    file_obj.write(execute_cmd)

                #Create new device with inserted SFP media driver
                execute_cmd = "{0} 0x50".format(media_driver[media_type])
                with open(new_sfp_path, 'w') as file_obj:
                    file_obj.write(execute_cmd)

            return True
        except EnvironmentError as error:
            syslog.syslog(syslog.LOG_ERR, "Error: reinit sfp driver failed: %s" % str(error))
            return False

    def is_qsa_present(self):
        try:
            media_ff = self.media_type
            port_ff = self.get_port_form_factor()
            # Form factor starts with 'Q'
            if port_ff[0] == 'Q' and media_ff in ['SFP', 'SFP+', 'SFP28', 'SFP56-DD']:
                return True
        except:
            pass

        return False

    def copper_dom_supported(self):
        """
        Check if the copper media inserted supports DOM. For instance, RJ45 might have
        been connected on SFP+ transceiver which supports DOM
        """
        if self.eeprom_cache is not None:
            ext_spec_compliance = self.eeprom_cache[SFF8472_EXT_COMPL_CODE_OFFSET]
            #Byte 36 value 0x1c means 10G SFP Base T
            if ext_spec_compliance == "1c":
                return True

        return False

    def parse_vdm_values(self, eeprom_raw_data, start_pos):
        observables_dict = {}
        c_dict = self.dom_obj.parse_vdm_values(eeprom_raw_data, start_pos)
        observables_dict['preFECBER'] = c_dict['data']['preFECBER']['value']
        observables_dict['postFECerrframeratio'] = c_dict['data']['postFECerrframeratio']['value']
        observables_dict['chromaticDispersionShortlink'] = c_dict['data']['chromaticDispersionShortlink']['value']
        observables_dict['differentialGroupDelay'] = c_dict['data']['differentialGroupDelay']['value']
        observables_dict['polarizationDependentLoss'] = c_dict['data']['polarizationDependentLoss']['value']
        observables_dict['carrierFrequencyOffset'] = c_dict['data']['carrierFrequencyOffset']['value']
        observables_dict['errorVectorMagnitude'] = c_dict['data']['errorVectorMagnitude']['value']
        observables_dict['rxOsnrEstimate'] = c_dict['data']['rxOsnrEstimate']['value']
        observables_dict['rxEsnrEstimate'] = c_dict['data']['rxEsnrEstimate']['value']
        observables_dict['sopRateofChange'] = c_dict['data']['sopRateofChange']['value']
        observables_dict['chromaticDispersionLonglink'] = c_dict['data']['chromaticDispersionLonglink']['value']
        observables_dict['laserTemperature'] = c_dict['data']['laserTemperature']['value']
        observables_dict['txPower'] = c_dict['data']['txPower']['value']
        observables_dict['rxChannelPower'] = c_dict['data']['rxChannelPower']['value']
        observables_dict['rxTotalPower'] = c_dict['data']['rxTotalPower']['value']
        observables_dict['modulationBiasXI'] = c_dict['data']['modulationBiasXI']['value']
        observables_dict['modulationBiasXQ'] = c_dict['data']['modulationBiasXQ']['value']
        observables_dict['modulationBiasXphase'] = c_dict['data']['modulationBiasXphase']['value']
        observables_dict['modulationBiasYI'] = c_dict['data']['modulationBiasYI']['value']
        observables_dict['modulationBiasYQ'] = c_dict['data']['modulationBiasYQ']['value']
        observables_dict['modulationBiasYphase'] = c_dict['data']['modulationBiasYphase']['value']
        return observables_dict

    def get_vdm_parameter_units(self):
        return self.dom_obj.dom_vdm_parameter_units
