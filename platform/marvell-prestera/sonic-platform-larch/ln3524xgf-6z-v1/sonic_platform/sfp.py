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
    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"

    SFP_PORT_START = 1
    QSFP_PORT_START = 25  # QSFP EEPROMs start at I2C bus 35 (port 25 + 10)
    QSFP_PORT_END = 30

class GpioFunction(Enum):
    ABS = 'abs'
    LOS = 'los'
    DIS = 'dis'
    PRS = 'prs'
    INTL = 'intl'
    RST = 'rst'
    LP = 'lp'

CHIP_MAP = {
    'abs': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '20',
        1: '21'
    },
    'los': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '23',
        1: '24'
    },
    'dis': {
        'hw_path': 'f2701100.i2c/i2c-1/1-00',
        0: '20',
        1: '21'
    },
    'prs': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '26'
    },
    'intl': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '26'
    },
    'rst': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '27'
    },
    'lp': {
        'hw_path': 'f2211000.i2c/i2c-2/2-00',
        0: '27'
    },
}

def get_iomux_port_wiring(port_num, function):
    # SFP 1..16: chip0 (zM_ABS0-15 / zRX_LOS0-15 / zTX_DIS0-15), pin = port-1
    # SFP 17..24: chip1 (zM_ABS16-23 / zRX_LOS16-23 / zTX_DIS16-23), pin = port-17
    if port_num < 17:
        return (0, port_num - 1)
    elif port_num < SfpBoardConfig.QSFP_PORT_START:
        return (1, port_num - 17)

    if function == GpioFunction.PRS or function == GpioFunction.RST:
        return (0, port_num - SfpBoardConfig.QSFP_PORT_START)
    elif function == GpioFunction.INTL or function == GpioFunction.LP:
        return (0, port_num - SfpBoardConfig.QSFP_PORT_START + 8)
    else:
        raise NotImplementedError

EEPROM_PATH = '/sys/bus/i2c/devices/i2c-{}/{}-0050/eeprom'

class Sfp(SfpOptoeBase):
    """
    Platform-specific Sfp class implementation
    """

    def __init__(self, index, type):
        assert index >= SfpBoardConfig.SFP_PORT_START
        assert index <= SfpBoardConfig.QSFP_PORT_END

        SfpOptoeBase.__init__(self)
        self.port_num = index # 1-based physical port number
        self.type = type
        self.eeprom_path = EEPROM_PATH.format(self.port_num + 10, self.port_num + 10)

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
    
    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        if self.type == SfpBoardConfig.SFP_TYPE:
            if self.__get_abs_gpio_value():
                return False
            # WORKAROUND: on ln3524xgf-6z-v1 the MOD_ABS inputs on the
            # PCA9555 @0x20/0x21 read 0 ("present") for all 24 ports
            # regardless of the module state (hardware question open).
            # Confirm presence by probing the module EEPROM: optoe returns
            # an I/O error when nothing answers at 0x50 on the port bus.
            return self.__eeprom_responds()

        return not self.__get_prs_gpio_value()

    def __eeprom_responds(self):
        try:
            fd = os.open(self.eeprom_path, os.O_RDONLY)
            try:
                return len(os.read(fd, 1)) == 1
            finally:
                os.close(fd)
        except OSError:
            return False
    
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
        if self.type == SfpBoardConfig.QSFP_TYPE:
            # 100G ports use 4 lanes: QSFP 25..30 -> Ethernet24,28,...,44
            return "Ethernet{}".format(24 + 4 * (self.port_num - SfpBoardConfig.QSFP_PORT_START))
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
                "type": self.get_name(),
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
                "cable_length": "0",
                "nominal_bit_rate": 'N/A',
                "specification_compliance": 'N/A',
                "vendor_date": 'N/A',
                "vendor_oui": 'N/A',
                "application_advertisement": "N/A",
                "hardware_rev": 'N/A'
            }

        if port_info is not None:
            # xcvrd passes most fields to swsscommon.FieldValuePairs unchanged,
            # which only accepts str; a None or non-str value (seen with
            # malformed DAC EEPROMs) kills the SfpStateUpdateTask thread.
            port_info = {k: ('N/A' if v is None else v if isinstance(v, str) else str(v))
                         for k, v in port_info.items()}
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
        i2c_base = '/sys/devices/platform/cp0/cp0:config-space@f2000000/' 
    
        base = i2c_base + CHIP_MAP[gpio_function.value]['hw_path'] + \
               CHIP_MAP[gpio_function.value][chip] + '/gpio/'
        gpio_base_num = int(os.listdir(base)[0].partition('gpiochip')[2])
        
        return gpio_base_num + offset
                
    def __add_sysfs_gpio(self, gpio_num, type):
        dir = '/sys/class/gpio/'
        gpio_dir = dir + 'gpio' + str(gpio_num) + '/'

        if not os.path.exists(gpio_dir):
            self.__write_sysfs_file(dir + 'export', gpio_num)
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


