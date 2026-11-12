try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    import time
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


logger = Logger()


class SfpBoardConfig:
    SFP_PLUS_TYPE = "SFP+"
    QSFP28_TYPE = "QSFP28"

    SFP_PLUS_PORT_START = 1
    QSFP28_PORT_START = 49
    QSFP28_PORT_END = 54


class SFPManagement():
    def __init__(self, port_num, ctrl_cpld, port_cpld):
        self.port_num = port_num
        self.ctrl_cpld = ctrl_cpld
        self.port_cpld = port_cpld

    def is_module_present(self) -> bool:
        if self.port_num >=1 and self.port_num <= 8:
            value = self.port_cpld.read_register(0x10)
        elif self.port_num >= 9 and self.port_num <= 16:
            value = self.port_cpld.read_register(0x11)
        elif self.port_num >= 17 and self.port_num <= 24:
            value = self.port_cpld.read_register(0x12)
        elif self.port_num >= 25 and self.port_num <= 32:
            value = self.ctrl_cpld.read_register(0x25)
        elif self.port_num >= 33 and self.port_num <= 40:
            value = self.ctrl_cpld.read_register(0x26)
        else:
            value = self.ctrl_cpld.read_register(0x27)

        return not (value & (1 << ((self.port_num - 1) % 8)))

    def get_rx_los(self):
        if self.port_num >=1 and self.port_num <= 8:
            value = self.port_cpld.read_register(0x13)
        elif self.port_num >= 9 and self.port_num <= 16:
            value = self.port_cpld.read_register(0x14)
        elif self.port_num >= 17 and self.port_num <= 24:
            value = self.port_cpld.read_register(0x15)
        elif self.port_num >= 25 and self.port_num <= 32:
            value = self.ctrl_cpld.read_register(0x28)
        elif self.port_num >= 33 and self.port_num <= 40:
            value = self.ctrl_cpld.read_register(0x29)
        else:
            value = self.ctrl_cpld.read_register(0x30)

        return [value & (1 << ((self.port_num - 1) % 8))]

    def get_tx_fault(self):
        if self.port_num >=1 and self.port_num <= 8:
            value = self.port_cpld.read_register(0x23)
        elif self.port_num >= 9 and self.port_num <= 16:
            value = self.port_cpld.read_register(0x24)
        elif self.port_num >= 17 and self.port_num <= 24:
            value = self.port_cpld.read_register(0x25)
        elif self.port_num >= 25 and self.port_num <= 32:
            value = self.ctrl_cpld.read_register(0x34)
        elif self.port_num >= 33 and self.port_num <= 40:
            value = self.ctrl_cpld.read_register(0x35)
        else:
            value = self.ctrl_cpld.read_register(0x36)

        return [value & (1 << ((self.port_num - 1) % 8))]

    def get_tx_disable(self) -> bool:
        if self.port_num >=1 and self.port_num <= 8:
            value = self.port_cpld.read_register(0x20)
        elif self.port_num >= 9 and self.port_num <= 16:
            value = self.port_cpld.read_register(0x21)
        elif self.port_num >= 17 and self.port_num <= 24:
            value = self.port_cpld.read_register(0x22)
        elif self.port_num >= 25 and self.port_num <= 32:
            value = self.ctrl_cpld.read_register(0x31)
        elif self.port_num >= 33 and self.port_num <= 40:
            value = self.ctrl_cpld.read_register(0x32)
        else:
            value = self.ctrl_cpld.read_register(0x33)

        return [value & (1 << ((self.port_num - 1) % 8))]

    def set_tx_disable(self, disable: bool) -> None:
        reg = 0
        mask = 0
        cpld = None
    
        if self.port_num >=1 and self.port_num <= 8:
            reg = 0x20
            cpld = self.port_cpld
            mask = 1 << (self.port_num - 1)
        elif self.port_num >= 9 and self.port_num <= 16:
            reg = 0x21
            cpld = self.port_cpld
            mask = 1 << (self.port_num - 9)
        elif self.port_num >= 17 and self.port_num <= 24:
            reg = 0x22
            cpld = self.port_cpld
            mask = 1 << (self.port_num - 17)
        elif self.port_num >= 25 and self.port_num <= 32:
            reg = 0x31
            cpld = self.ctrl_cpld
            mask = 1 << (self.port_num - 25)
        elif self.port_num >= 33 and self.port_num <= 40:
            reg = 0x32
            cpld = self.ctrl_cpld
            mask = 1 << (self.port_num - 33)
        else:
            reg = 0x33
            cpld = self.ctrl_cpld
            mask = 1 << (self.port_num - 41)

        val = cpld.read_register(reg)

        if disable:
            val |= mask
        else:
            val &= ~mask

        cpld.write_register(reg, val)

class QSFPManagement():
    def __init__(self, port_num, ctrl_cpld):
        self.port_num = port_num
        self.ctrl_cpld = ctrl_cpld

    def is_module_present(self) -> bool:
        value = self.ctrl_cpld.read_register(0x22)

        return not (value & (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START)))

    def reset(self) -> None:
        value = self.ctrl_cpld.read_register(0x24)
        value &= ~(1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))
        self.ctrl_cpld.write_register(0x24, value)

        time.sleep(0.1)

        value = self.ctrl_cpld.read_register(0x24)
        value |= (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))
        self.ctrl_cpld.write_register(0x24, value)

        time.sleep(0.5) 

    def get_low_power_mode(self) -> bool:
        value = self.ctrl_cpld.read_register(0x21)

        return value & (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))

    def set_low_power_mode(self, enable: bool) -> None:
        value = self.ctrl_cpld.read_register(0x21)
        if enable:
            value |= (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))
        else:
            value &= ~(1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))

        self.ctrl_cpld.write_register(0x21, value)
    
    def get_tx_disable(self) -> bool:
        value = self.ctrl_cpld.read_register(0x23)

        return [value & (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))]

    def set_tx_disable(self, disable: bool) -> None:
        value = self.ctrl_cpld.read_register(0x23)

        if disable:
            value |= (1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))
        else:
            value &= ~(1 << (self.port_num - SfpBoardConfig.QSFP28_PORT_START))

        self.ctrl_cpld.write_register(0x23, value)


class Sfp(SfpOptoeBase):
    """
    Platform-specific Sfp class implementation
    """

    def __init__(self, index, type, main_cpld, aux_cpld):
        assert index >= SfpBoardConfig.SFP_PLUS_PORT_START
        assert index <= SfpBoardConfig.QSFP28_PORT_END
        assert type == SfpBoardConfig.SFP_PLUS_TYPE or type == SfpBoardConfig.QSFP28_TYPE

        SfpOptoeBase.__init__(self)

        self.port_num = index # 1-based physical port number
        self.type = type
        self.eeprom_path = self.__get_eeprom_path()

        if self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            self.management = SFPManagement(self.port_num, main_cpld, aux_cpld)
        else:
            self.management = QSFPManagement(self.port_num, main_cpld)
        
    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        return self.management.is_module_present()

    def get_rx_los(self):
        """
        Retrieves the RX LOS (loss-of-signal) status of SFP

        Returns:
            A list of boolean values, representing the RX LOS status
            of each available channel, value is True if SFP channel
            has RX LOS, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        if self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return [self.management.get_rx_los()]
        else:
            return [False]

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A list of boolean values, representing the TX disable status
            of each available channel, value is True if SFP channel
            is TX disabled, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
        """  
        return [self.management.get_tx_disable()] 

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        self.management.set_tx_disable(tx_disable)

        return True
    
    def get_tx_fault(self):
        if self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return [self.management.get_tx_fault()]
        else:
            return [False]
        
    def reset(self):
        """
        Reset SFP and return all user module settings to their default state.

        Returns:
            A boolean, True if successful, False if not
        """
        if self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return False
        else:
            return self.management.reset()

    def get_lpmode(self):
        if self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return False
        else:
            return self.management.get_low_power_mode()  

    def set_lpmode(self, lpmode):
        if self.type == SfpBoardConfig.QSFP28_PORT_START:
            self.management.set_low_power_mode(lpmode)

        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        return self.SFP_STATUS_OK

    def get_transceiver_status(self):
        """
        Below part is added to avoid failing xcvrd
        """
        raise NotImplementedError

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return "Ethernet{}".format(self.port_num - 1)

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return self.port_num

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_transceiver_info(self):
        port_info = None
        try:
            port_info = SfpOptoeBase.get_transceiver_info(self)
        except NotImplementedError:
            pass
        if port_info is not None:
            port_info['hardware_rev'] = 'N/A'
        # Temporarily use eeprom stub info
        if port_info is None and self.get_presence():
            port_info = {
                "type": "N/A",
                "type_abbrv_name": 'N/A',
                "vendor_rev": 'N/A',
                "serial": 'N/A',
                "manufacturer": 'N/A',
                "model": 'N/A',
                "connector": 'N/A',
                "encoding": 'N/A',
                "ext_identifier": 'N/A',
                "ext_rateselect_compliance": 'N/A',
                "cable_type": 'N/A',
                "cable_length": 0,
                "nominal_bit_rate": 'N/A',
                "specification_compliance": 'N/A',
                "vendor_date": 'N/A',
                "vendor_oui": 'N/A',
                "application_advertisement": "N/A",
                "hardware_rev": 'N/A'
            }

        return port_info

    def get_transceiver_threshold_info(self):
        return None

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        return self.get_presence()
  

    def __get_eeprom_path(self):
        if self.type ==  SfpBoardConfig.SFP_PLUS_TYPE:
            if self.port_num >= 1 and self.port_num <= 8:
                channel = 100 + (self.port_num - 1) * 5
            elif self.port_num >= 9 and self.port_num <= 16:
                channel = 101 + (self.port_num - 8 - 1) * 5
            elif self.port_num >= 17 and self.port_num <= 24:
                channel = 102 + (self.port_num - 16 - 1) * 5
            elif self.port_num >= 25 and self.port_num <= 32:
                channel = 50 + (self.port_num - 24 - 1) * 5
            elif self.port_num >= 33 and self.port_num <= 40:
                channel = 51 + (self.port_num - 32 - 1) * 5
            else:
                channel = 52 + (self.port_num - 40 - 1) * 5

            return  '/sys/bus/i2c/devices/' + str(channel) + '-0050/eeprom'
        else:
            channel = 53 + (self.port_num - SfpBoardConfig.QSFP28_PORT_START) * 5

            return  '/sys/bus/i2c/devices/' + str(channel) + '-0050/eeprom'
