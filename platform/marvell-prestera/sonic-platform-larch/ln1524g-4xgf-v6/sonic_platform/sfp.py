try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

from enum import Enum
import os

logger = Logger()


class SfpBoardConfig:
    COPPER_TYPE = "COPPER"
    SFP_TYPE = "SFP"
    SFP_PLUS_TYPE = "SFP+"

    COPPER_PORT_START = 1
    SFP_PORT_START = 17
    SFP_PLUS_PORT_START = 25
    SFP_PLUS_PORT_END = 28

class GpioFunction(Enum):
    ABS = 'abs'
    DIS = 'dis'

class CPLD:
    def __init__(self, port_num):
        self.port_num = port_num
    
    def get_presence(self):
        data = int(read_sysfs_file('/sys/bus/i2c/devices/1-0032/sfp_port_status'), 16) >> 4

        if data & (1 << self.port_num):
            return False
        return True

    def get_rx_los(self):
        data = int(read_sysfs_file('/sys/bus/i2c/devices/1-0032/sfp_port_status'), 16) & 0x0f

        if data & (1 << self.port_num):
            return True
        return False

    def get_tx_disable(self):
        data = int(read_sysfs_file('/sys/bus/i2c/devices/1-0032/sfp_tx_disable'), 16) & 0x0f

        if data & (1 << self.port_num):
            return True
        return False
    
    def set_tx_disable(self, tx_disable):
        data = int(read_sysfs_file('/sys/bus/i2c/devices/1-0032/sfp_tx_disable'), 16) & 0x0f

        if tx_disable:
            data |= (1 << self.port_num)
        else:
            data &= ~(1 << self.port_num)

        write_sysfs_file('/sys/bus/i2c/devices/1-0032/sfp_tx_disable', data)


class Sfp(SfpOptoeBase):
    """
    Platform-specific Sfp class implementation
    """

    def __init__(self, index, type):
        assert index >= SfpBoardConfig.COPPER_PORT_START
        assert index <= SfpBoardConfig.SFP_PLUS_PORT_END

        SfpOptoeBase.__init__(self)

        self.port_num = index # 1-based physical port number
        self.type = type
        self.eeprom_path = self.__get_eeprom_path()

        if self.type == SfpBoardConfig.SFP_TYPE:
            self.__init_abs_gpio()
            self.__init_dis_gpio()
        elif self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            self.cpld = CPLD(self.port_num - SfpBoardConfig.SFP_PLUS_PORT_START)
        
    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        if self.type == SfpBoardConfig.SFP_TYPE:
            return not self.__get_abs_gpio_value()
        elif self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return self.cpld.get_presence()
        else:
            return True

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
            return [self.cpld.get_rx_los()]
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
        if self.type == SfpBoardConfig.SFP_TYPE:
            return [self.__get_dis_gpio_value()] 
        elif self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            return [self.cpld.get_tx_disable()]
        else:
            return [False]

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.type == SfpBoardConfig.SFP_TYPE:
            self.__set_dis_gpio_value(tx_disable)
        elif self.type == SfpBoardConfig.SFP_PLUS_TYPE:
            self.cpld.set_tx_disable(tx_disable)
        
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
        if self.type == SfpBoardConfig.COPPER_TYPE:
            return False

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
                "type": "RJ45" if self.type == SfpBoardConfig.COPPER_TYPE else "N/A",
                "type_abbrv_name": "RJ45" if self.type == SfpBoardConfig.COPPER_TYPE else "N/A",
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


    def __init_abs_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.ABS)
        self.abs_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_dis_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.DIS)
        self.dis_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'output')

    def __get_abs_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.abs_sysfs_gpio)
        
    def __get_dis_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.dis_sysfs_gpio)
    def __set_dis_gpio_value(self, value: bool):
        return self.__set_sysfs_gpio_value(self.dis_sysfs_gpio, value)
    
    def __get_sysfs_gpio_value(self, sysfs_gpio):
        gpio_value = read_sysfs_file(sysfs_gpio)
        return True if gpio_value == '1' else False
    
    def __set_sysfs_gpio_value(self, sysfs_gpio, value: bool):
        return write_sysfs_file(sysfs_gpio, int(value))
    

    def __get_eeprom_path(self):
        if self.type ==  SfpBoardConfig.SFP_TYPE:
            root_bus = 10
            return  '/sys/bus/i2c/devices/' + str(self.port_num - SfpBoardConfig.SFP_PORT_START + root_bus) + '-0050/eeprom'
        elif self.type ==  SfpBoardConfig.SFP_PLUS_TYPE:
            root_bus = 3
            return '/sys/bus/i2c/devices/' + str(self.port_num - SfpBoardConfig.SFP_PLUS_PORT_START + root_bus) + '-0050/eeprom'
        
        return ''

    def __get_gpio_num(self, function: GpioFunction):
        if function == GpioFunction.ABS:
            base = '/sys/bus/i2c/devices/9-0020/gpio/'
        else:
            base = '/sys/bus/i2c/devices/9-0021/gpio/'

        gpio_base_num = int(os.listdir(base)[0].partition('gpiochip')[2])

        return gpio_base_num + (self.port_num - SfpBoardConfig.SFP_PORT_START)
                
    def __add_sysfs_gpio(self, gpio_num, type):
        dir = '/sys/class/gpio/'
        gpio_dir = dir + 'gpio' + str(gpio_num) + '/'

        if not os.path.exists(gpio_dir):
            write_sysfs_file(dir + 'export', gpio_num)
            write_sysfs_file(gpio_dir + 'direction', 'in' if type == 'input' else 'out')
        
        return gpio_dir + 'value'


def write_sysfs_file(sysfs_file, value):
    with open(sysfs_file, 'w') as fd:
        fd.write(str(value))

def read_sysfs_file(sysfs_file) -> str:
    with open(sysfs_file, 'r') as fd:
        rv = fd.read()

    rv = rv.rstrip('\r\n')
    return rv.lstrip(" ")
