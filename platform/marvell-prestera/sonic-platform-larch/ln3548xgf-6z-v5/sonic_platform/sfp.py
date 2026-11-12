try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

from enum import Enum
import os
import math
import glob

logger = Logger()

class SfpBoardConfig:
    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"

    SFP_PORT_START = 1
    QSFP_PORT_START = 49
    QSFP_PORT_END = 54

class GpioFunction(Enum):
    ABS = 'abs'
    LOS = 'los'
    DIS = 'dis'
    PRS = 'prs'
    INTL = 'intl'
    RST = 'rst'


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
        self.eeprom_path = self.__get_eeprom_path()

        if self.type ==  SfpBoardConfig.SFP_TYPE:
            self.__init_abs_gpio()
            self.__init_los_gpio()
            self.__init_dis_gpio()
        elif self.type ==  SfpBoardConfig.QSFP_TYPE:
            self.__init_prs_gpio()
            self.__init_intl_gpio()
            self.__init_rst_gpio()
        else:
            raise NotImplementedError
    
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
        gpio_num = self.__get_gpio_num_qsfp(GpioFunction.PRS)
        self.prs_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_intl_gpio(self):
        gpio_num = self.__get_gpio_num_qsfp(GpioFunction.INTL)
        self.intl_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'input')

    def __init_rst_gpio(self):
        gpio_num = self.__get_gpio_num_qsfp(GpioFunction.RST)
        self.rst_sysfs_gpio = self.__add_sysfs_gpio(gpio_num, 'output')


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

    def __get_sysfs_gpio_value(self, sysfs_gpio):
        gpio_value = self.__read_sysfs_file(sysfs_gpio)
        return True if gpio_value == '1' else False
    
    def __set_sysfs_gpio_value(self, sysfs_gpio, value: bool):
        return self.__write_sysfs_file(sysfs_gpio, int(value))
    

    def __get_eeprom_offset(self, port_idx):
        if self.type ==  SfpBoardConfig.QSFP_TYPE:
            if port_idx == 1: 
                return '0'
            elif port_idx == 2: 
                return '1'
            elif port_idx == 3: 
                return '2'
            elif port_idx == 4: 
                return '3'
            elif port_idx == 5: 
                return '4'
            elif port_idx == 6: 
                return '5'
            
            raise NotImplementedError
        
        if port_idx == 0:
            return '1'
        elif port_idx == 1: 
            return '2'
        elif port_idx == 2: 
            return '3'
        elif port_idx == 3: 
            return '5'
        elif port_idx == 4: 
            return '4'
        elif port_idx == 5: 
            return '7'
        elif port_idx == 6: 
            return '6'
        elif port_idx == 7: 
            return '0'
        
        raise NotImplementedError

    def __get_eeprom_path(self):
        chip_map = {
            1: '75',
            2: '74',
            3: '73',
            4: '72',
            5: '70',
            6: '71',
            7: '76'
        }

        group_id = math.ceil(self.port_num / 8)
        chip_addr = chip_map[group_id]

        port_idx = self.port_num % 8
        offset = self.__get_eeprom_offset(port_idx)

        base = '/sys/class/i2c-dev/i2c-7/device/7-00' + chip_addr + '/channel-' + offset
        path_math = glob.glob(os.path.join(base, "*-0050"))
        if len(path_math) == 0:
            return 'None'

        return glob.glob(os.path.join(base, "*-0050"))[0] + '/eeprom'
    
    def __get_chip_path_components(self):
        chip_addr_map = {
            1: '25',
            2: '26',
            3: '27',
            4: '22',
            5: '23',
            6: '24',
        }

        if SfpBoardConfig.SFP_PORT_START <= self.port_num <= 24: 
            bus_id = '4'
            group_id = math.ceil(self.port_num / 4)
            chip_addr = chip_addr_map[group_id]
            port_idx = self.port_num % 4
        elif 25 <= self.port_num <= SfpBoardConfig.QSFP_PORT_START - 1:
            bus_id = '3'
            group_id = math.ceil((self.port_num - 24) / 4)
            chip_addr = chip_addr_map[group_id]
            port_idx = (self.port_num - 24) % 4
        else:
            raise NotImplementedError
        
        return bus_id, chip_addr, port_idx 

    def __get_chip_offset(self, port_idx, function):
        if port_idx == 0:
            if function == GpioFunction.DIS:
                return 13
            elif function == GpioFunction.LOS:
                return 15
            elif function == GpioFunction.ABS:
                return 14

        if port_idx == 1:
            if function == GpioFunction.DIS:
                return 7
            elif function == GpioFunction.LOS:
                return 6
            elif function == GpioFunction.ABS:
                return 5

        if port_idx == 2:
            if function == GpioFunction.DIS:
                return 2
            elif function == GpioFunction.LOS:
                return 0
            elif function == GpioFunction.ABS:
                return 1

        if port_idx == 3:
            if function == GpioFunction.DIS:
                return 11
            elif function == GpioFunction.LOS:
                return 10
            elif function == GpioFunction.ABS:
                return 9

        raise NotImplementedError

    def __get_gpio_num(self, function: GpioFunction):
        i2c_base = '/sys/devices/platform/cp0/cp0:config-space@f2000000/' 

        bus_id, chip_addr, port_idx = self.__get_chip_path_components()

        base = i2c_base + 'f2701100.i2c/i2c-1/i2c-' + bus_id + '/' + bus_id + '-00' + chip_addr + '/gpio/'
        gpio_base_num = int(os.listdir(base)[0].partition('gpiochip')[2])

        offset = self.__get_chip_offset(port_idx, function)

        return gpio_base_num + offset

    def __get_gpio_num_qsfp(self, function: GpioFunction):
        if self.port_num < SfpBoardConfig.QSFP_PORT_START or self.port_num > SfpBoardConfig.QSFP_PORT_END: 
            raise NotImplementedError

        chip_map = {
            49: {
                GpioFunction.PRS:  ('21', 0),
                GpioFunction.INTL: ('21', 8),
                GpioFunction.RST:  ('20', 6)
            },
            50: {
                GpioFunction.PRS:  ('21', 1),
                GpioFunction.INTL: ('21', 9),
                GpioFunction.RST:  ('20', 7)
            },
            51: {
                GpioFunction.PRS:  ('21', 2),
                GpioFunction.INTL: ('21', 10),
                GpioFunction.RST:  ('21', 14)
            },
            52: {
                GpioFunction.PRS:  ('21', 3),
                GpioFunction.INTL: ('21', 11),
                GpioFunction.RST:  ('21', 15)
            },    
            53: {
                GpioFunction.PRS:  ('21', 4),
                GpioFunction.INTL: ('21', 12),
                GpioFunction.RST:  ('21', 6)    
            },
            54: {
                GpioFunction.PRS:  ('21', 5),
                GpioFunction.INTL: ('21', 13),
                GpioFunction.RST:  ('21', 7)
            }
        }

        i2c_base = '/sys/devices/platform/cp0/cp0:config-space@f2000000/' 

        chip_addr = chip_map[self.port_num][function][0]
        offset = chip_map[self.port_num][function][1]

        base = i2c_base + 'f2701100.i2c/i2c-1/1-00' + chip_addr + '/gpio/'
        gpio_base_num = int(os.listdir(base)[0].partition('gpiochip')[2])

        return gpio_base_num + offset  
                
    def __add_sysfs_gpio(self, gpio_num, gpio_type):
        """
        Exports a GPIO via sysfs and sets its direction.
        Handles race conditions where the GPIO might already be exported.
        """
        base_path = '/sys/class/gpio/'
        export_path = os.path.join(base_path, 'export')
        gpio_dir = os.path.join(base_path, f'gpio{gpio_num}')

        # 1. Export the GPIO if it's not already exported
        if not os.path.exists(gpio_dir):
            try:
                with open(export_path, 'w') as f:
                    f.write(str(gpio_num))
            except OSError as e:
                # If Errno 16 (EBUSY), another process exported it just now. 
                # We can safely ignore this and proceed.
                if e.errno != errno.EBUSY:
                    raise
        
        # 2. Wait for the kernel to populate the directory (it's not always instant)
        # We check for the 'direction' file specifically.
        direction_path = os.path.join(gpio_dir, 'direction')
        retries = 10
        while not os.path.exists(direction_path) and retries > 0:
            time.sleep(0.1)
            retries -= 1

        # 3. Set the direction (Input/Output)
        # Even if already exported, we ensure the direction matches our needs.
        target_direction = 'in' if gpio_type == 'input' else 'out'
        try:
            with open(direction_path, 'w') as f:
                f.write(target_direction)
        except OSError as e:
            # If this fails with EBUSY, the GPIO is likely locked by a kernel driver.
            # Check /sys/kernel/debug/gpio for ownership.
            raise RuntimeError(f"Failed to set GPIO {gpio_num} direction: {e}")

        return os.path.join(gpio_dir, 'value')

    def __write_sysfs_file(self, sysfs_file, value):
        with open(sysfs_file, 'w') as fd:
            fd.write(str(value))
        
    def __read_sysfs_file(self, sysfs_file) -> str:      
        with open(sysfs_file, 'r') as fd:
            rv = fd.read()

        rv = rv.rstrip('\r\n')
        return rv.lstrip(" ")
