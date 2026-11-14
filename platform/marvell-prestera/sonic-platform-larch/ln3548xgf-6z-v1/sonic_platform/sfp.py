try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

from enum import Enum
import glob
import os
import time


logger = Logger()

class SfpBoardConfig:
    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"
    MAPPING_VERSION = "v4-straight-panel-order"

    # These are the 1-based physical indices used by port_config.ini.
    # Indices 1-6 are 100G ports in panel order (QSFP1..QSFP6).
    # Indices 7-54 are 10G ports in panel order (SFP1..SFP48).
    PORT_START = 1
    PORT_END = 54
    QSFP_PORT_START = 1
    QSFP_PORT_END = 6
    SFP_PORT_START = 7
    SFP_PORT_END = 54

class GpioFunction(Enum):
    ABS = 'abs'
    LOS = 'los'
    DIS = 'dis'
    PRS = 'prs'
    INTL = 'intl'
    RST = 'rst'
    LP = 'lp'


def get_panel_port_number(port_num, sfp_type):
    """Map a 1-based port_config.ini index to the front-panel cage number."""
    if sfp_type == SfpBoardConfig.SFP_TYPE:
        if not SfpBoardConfig.SFP_PORT_START <= port_num <= SfpBoardConfig.SFP_PORT_END:
            raise ValueError("Invalid SFP physical index: {}".format(port_num))
        return port_num - SfpBoardConfig.SFP_PORT_START + 1

    if sfp_type == SfpBoardConfig.QSFP_TYPE:
        if not SfpBoardConfig.QSFP_PORT_START <= port_num <= SfpBoardConfig.QSFP_PORT_END:
            raise ValueError("Invalid QSFP physical index: {}".format(port_num))
        return port_num

    raise ValueError("Unsupported transceiver type: {}".format(sfp_type))

CHIP_MAP = {
    'abs': {
        'i2c_bus': 2,
        0: 0x20,
        1: 0x21,
        2: 0x22,
    },
    'los': {
        'i2c_bus': 2,
        0: 0x23,
        1: 0x24,
        2: 0x25,
    },
    'dis': {
        'i2c_bus': 1,
        0: 0x20,
        1: 0x21,
        2: 0x22,
    },
    'prs': {
        'i2c_bus': 2,
        0: 0x26,
    },
    'intl': {
        'i2c_bus': 2,
        0: 0x26,
    },
    'rst': {
        'i2c_bus': 2,
        0: 0x27,
    },
    'lp': {
        'i2c_bus': 2,
        0: 0x27,
    },
}

def get_iomux_port_wiring(port_num, function):
    """Return PCA9555 number and line offset for a 1-based physical index."""
    if SfpBoardConfig.SFP_PORT_START <= port_num <= SfpBoardConfig.SFP_PORT_END:
        if function not in (GpioFunction.ABS, GpioFunction.LOS, GpioFunction.DIS):
            raise NotImplementedError

        sfp_index = get_panel_port_number(
            port_num, SfpBoardConfig.SFP_TYPE) - 1
        return (sfp_index // 16, sfp_index % 16)

    if SfpBoardConfig.QSFP_PORT_START <= port_num <= SfpBoardConfig.QSFP_PORT_END:
        qsfp_index = get_panel_port_number(
            port_num, SfpBoardConfig.QSFP_TYPE) - 1
        if function in (GpioFunction.PRS, GpioFunction.RST):
            return (0, qsfp_index)
        if function in (GpioFunction.INTL, GpioFunction.LP):
            return (0, qsfp_index + 8)

    raise ValueError("Unsupported port/function combination: {}/{}".format(
        port_num, function.value))

EEPROM_PATH = '/sys/bus/i2c/devices/i2c-{}/{}-0050/eeprom'
SFP_MUX_PARENT_BUS = 1


def get_mux_channel_bus(mux_addr, channel):
    """Resolve a PCA9548 channel to its dynamic Linux I2C bus number."""
    channel_path = '/sys/bus/i2c/devices/{}-{:04x}/channel-{}'.format(
        SFP_MUX_PARENT_BUS, mux_addr, channel)
    if not os.path.exists(channel_path):
        raise OSError("PCA9548 channel not found: {}".format(channel_path))

    bus_name = os.path.basename(os.path.realpath(channel_path))
    if not bus_name.startswith('i2c-'):
        raise OSError("Unexpected PCA9548 channel target: {}".format(bus_name))
    return int(bus_name.replace('i2c-', '', 1))


def get_eeprom_mux_wiring(port_num, sfp_type):
    """Return module index, PCA9548 address, and channel."""
    if sfp_type == SfpBoardConfig.SFP_TYPE:
        module_index = get_panel_port_number(port_num, sfp_type) - 1
        return module_index, 0x70 + (module_index // 8), module_index % 8

    if sfp_type == SfpBoardConfig.QSFP_TYPE:
        module_index = get_panel_port_number(port_num, sfp_type) - 1
        return module_index, 0x76, module_index

    raise ValueError("Unsupported transceiver type: {}".format(sfp_type))

class Sfp(SfpOptoeBase):
    """
    Platform-specific Sfp class implementation
    """

    def __init__(self, index, type):
        assert index >= SfpBoardConfig.PORT_START
        assert index <= SfpBoardConfig.PORT_END

        SfpOptoeBase.__init__(self)
        self.port_num = index  # 1-based port_config.ini physical index
        self.type = type
        self.module_index, mux_addr, mux_channel = get_eeprom_mux_wiring(
            self.port_num, self.type)

        if self.type ==  SfpBoardConfig.SFP_TYPE:
            self.__init_abs_gpio()
            self.__init_los_gpio()
            self.__init_dis_gpio()
        elif self.type ==  SfpBoardConfig.QSFP_TYPE:
            self.__init_prs_gpio()
            self.__init_intl_gpio()
            self.__init_rst_gpio()
            self.__init_lp_gpio()
        else:
            raise NotImplementedError

        eeprom_bus = get_mux_channel_bus(mux_addr, mux_channel)
        self.eeprom_path = EEPROM_PATH.format(eeprom_bus, eeprom_bus)
    
    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        if self.type == SfpBoardConfig.SFP_TYPE:
            return not self.__get_abs_gpio_value()

        return not self.__get_prs_gpio_value()
    
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
        if self.type == SfpBoardConfig.SFP_TYPE:
            return [self.__get_los_gpio_value()]
        
        return [not self.__get_intl_gpio_value()]
    
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
        
        return [not self.__get_rst_gpio_value()]
    
    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.type == SfpBoardConfig.QSFP_TYPE:
            return self.__get_lp_gpio_value()
    
        return False
    
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
        else:
            self.__set_rst_gpio_value(not tx_disable)
        
        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.type == SfpBoardConfig.QSFP_TYPE:
            self.__set_lp_gpio_value(lpmode)
            return True

        raise NotImplementedError

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
        return "{}{}".format(self.type, self.module_index + 1)

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

    def get_optoe_type(self):
        """
        Returns the optoe driver type for this port:
          1 = optoe1 (QSFP, single I2C address 0x50)
          2 = optoe2 (SFP,  dual  I2C address 0x50/0x51)
        """
        if self.type == SfpBoardConfig.QSFP_TYPE:
            return 1
        return 2

    def get_transceiver_info(self):
        port_info = SfpOptoeBase.get_transceiver_info(self)
        if port_info is not None:
            port_info['hardware_rev'] = 'N/A'
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

    def __init_los_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.LOS)
        self.los_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_dis_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.DIS)
        self.dis_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'output')

    def __init_prs_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.PRS)
        self.prs_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_intl_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.INTL)
        self.intl_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_rst_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.RST)
        self.rst_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'output')
        try:
            self.__set_rst_gpio_value(True)  # De-assert RESET (active-low) so module can initialize
        except OSError:
            pass  # GPIO may be kernel-controlled; assume RST is already de-asserted

    def __init_lp_gpio(self):
        gpio_num = self.__get_gpio_num(GpioFunction.LP)
        self.lp_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'output')

    def __get_abs_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.abs_sysfs_gpio)
    
    def __get_los_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.los_sysfs_gpio)
    
    def __get_dis_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.dis_sysfs_gpio)
    def __set_dis_gpio_value(self, value: bool):
        return self.__set_sysfs_gpio_value(self.dis_sysfs_gpio, value)
    
    def __get_prs_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.prs_sysfs_gpio)
    
    def __get_intl_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.intl_sysfs_gpio)

    def __get_rst_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.rst_sysfs_gpio)
    def __set_rst_gpio_value(self, value: bool):
        return self.__set_sysfs_gpio_value(self.rst_sysfs_gpio, value)

    def __get_lp_gpio_value(self):
        return self.__get_sysfs_gpio_value(self.lp_sysfs_gpio)
    def __set_lp_gpio_value(self, value: bool):
        return self.__set_sysfs_gpio_value(self.lp_sysfs_gpio, value)

    def __get_sysfs_gpio_value(self, sysfs_gpio):
        gpio_value = self.__read_sysfs_file(sysfs_gpio)
        return True if gpio_value == '1' else False
    
    def __set_sysfs_gpio_value(self, sysfs_gpio, value: bool):
        try:
            return self.__write_sysfs_file(sysfs_gpio, int(value))
        except OSError:
            pass
    
    def __get_gpio_num(self, gpio_function: GpioFunction):
        chip, offset = get_iomux_port_wiring(self.port_num, gpio_function)
        config = CHIP_MAP[gpio_function.value]
        i2c_bus = config['i2c_bus']
        i2c_addr = config[chip]
        base = '/sys/bus/i2c/devices/{}-{:04x}/gpio'.format(i2c_bus, i2c_addr)
        gpiochips = glob.glob(os.path.join(base, 'gpiochip*'))
        if len(gpiochips) != 1:
            raise OSError("Expected one gpiochip under {}, found {}".format(
                base, len(gpiochips)))

        gpio_base_num = int(os.path.basename(gpiochips[0]).replace('gpiochip', ''))
        return gpio_base_num + offset
                
    def __add_sysfs_gpio(self, gpio_num, type):
        dir = '/sys/class/gpio/'
        gpio_dir = dir + 'gpio' + str(gpio_num) + '/'

        if not os.path.exists(gpio_dir):
            self.__write_sysfs_file(dir + 'export', gpio_num)
            for _ in range(50):
                if os.path.exists(gpio_dir):
                    break
                time.sleep(0.01)
            if not os.path.exists(gpio_dir):
                raise OSError("GPIO {} did not appear after export".format(gpio_num))
            self.__write_sysfs_file(gpio_dir + 'direction', 'in' if type == 'input' else 'out')
        
        return gpio_dir + 'value'

    def __write_sysfs_file(self, sysfs_file, value):
        with open(sysfs_file, 'w') as fd:
            fd.write(str(value))
        
    def __read_sysfs_file(self, sysfs_file) -> str:      
        with open(sysfs_file, 'r') as fd:
            rv = fd.read()

        rv = rv.rstrip('\r\n')
        return rv.lstrip(" ")
