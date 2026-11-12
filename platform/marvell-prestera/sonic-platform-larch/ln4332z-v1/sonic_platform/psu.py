#!/usr/bin/env python3

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os
import logging

try:
    import smbus as smbus_module
    _smbus_available = True
except ImportError:
    _smbus_available = False

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.bridges import bridge_bus, mux_child
    from sonic_platform.led import (I2CLed, I2CRegisterBitField,
                                    pld_bus, LED_I2CADDR,
                                    PWRLED_MASK, PWRLED_STATES)
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TLV_ATTR_TYPE_MODEL = 2
TLV_ATTR_TYPE_SERIAL = 5

PSU_HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
# PSU A / PSU B hang off channels 0 / 1 of the PCA9548 0x70 on the CP2112
# system bridge (see bridges.py); both are PMBus devices at 0x5b (no separate
# FRU EEPROM was found, model/serial come from PMBus MFR_MODEL/MFR_SERIAL).
PSU_MUX_ADDR = 0x70
PSU_I2C_MAPPING = {
    0: {"channel": 0, "psu_addr": "5b"},
    1: {"channel": 1, "psu_addr": "5b"},
}
PMBUS_STATUS_WORD = 0x79
PMBUS_MFR_MODEL = 0x9a
PMBUS_MFR_SERIAL = 0x9e
# model/serial strings read by the platform init script (pmon has no
# /dev/i2c-* nodes for adapters created after the container started);
# /var/run/platform_cache is bind-mounted into pmon
PSU_INFO_DIR = "/var/run/platform_cache/ln4332z-v1"


def _psu_bus(index):
    return mux_child(bridge_bus("sys"), PSU_MUX_ADDR, PSU_I2C_MAPPING[index]["channel"])

class Psu(PsuBase):
    """Platform-specific Psu class"""

    PSU_NAME_LIST = ["PSU-1", "PSU-2"]

    # Both PSUs share a single front-panel PWR LED in the control PLD; it is
    # built on first use because the PLD bus is only known once sysfs has
    # been walked. Every instance is kept so the LED follows all PSUs.
    _pwr_led = None
    _psus = []

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        Psu._psus.append(self)
        self.index = psu_index
        self._bus = None
        self.hwmon_path = None
        self._resolve()

    def _resolve(self):
        bus = _psu_bus(self.index)
        if bus is not None:
            self._bus = bus
            self.hwmon_path = PSU_HWMON_PATH.format(bus, PSU_I2C_MAPPING[self.index]['psu_addr'])
        return self._bus

    def _smbus(self):
        if not _smbus_available or self._resolve() is None:
            return None
        try:
            return smbus_module.SMBus(self._bus)
        except (OSError, IOError):
            return None

    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open %s", attr_path)

        retval = retval.rstrip('\r\n')

        fd.close()
        return retval

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        # The pmbus device stays bound when the PSU is pulled out, so a bound
        # hwmon is not presence. The PMBus controller answers as long as the
        # PSU is inserted - even without AC it runs from the other PSU's
        # standby rail and reports VIN 0 - and stops answering once it is
        # removed, which makes the driver's read fail.
        if self._resolve() is None or not os.path.isdir(self.hwmon_path):
            return False
        for dirpath, _, files in os.walk(self.hwmon_path):
            if "in1_input" in files:
                try:
                    with open(os.path.join(dirpath, "in1_input")) as f:
                        f.read()
                    return True
                except OSError:
                    return False
        return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False
        # output power good: STATUS_WORD OFF/VOUT_OV/IOUT_OC/VIN_UV/PGOOD# bits
        # (direct PMBus access is only possible on the host); otherwise the
        # PSU is considered good when it delivers output power
        status = self._pmbus_read_word(PMBUS_STATUS_WORD)
        if status is not None:
            return (status & 0x0878) == 0
        return self.get_power() > 0.0 or self.get_current() > 0.0

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Note: the platform has a single front-panel PWR LED shared by
        both PSUs.
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # psud calls this per PSU with that PSU's colour; with a single LED
        # the last call would win. Drive it from the aggregate instead: red
        # when any PSU is absent or not delivering power, green when all are
        # fine. Other colours (off/amber) are honoured as given, for tests.
        color = color.lower()
        if color in ("green", "red"):
            color = self._aggregate_color()
        led = self._get_pwr_led()
        if led is None:
            return False
        try:
            led.set_state("STATUS_LED_COLOR_" + color.upper())
        except Exception:          # LED register unreachable
            return False
        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        try:
            return self._get_pwr_led().get_state()
        except Exception:
            return self.STATUS_LED_COLOR_OFF

    @classmethod
    def _get_pwr_led(cls):
        if cls._pwr_led is None:
            bus = pld_bus()
            if bus is None:
                return None
            cls._pwr_led = I2CLed(I2CRegisterBitField(bus, LED_I2CADDR, None,
                                                      PWRLED_MASK, PWRLED_STATES))
        return cls._pwr_led

    @classmethod
    def _aggregate_color(cls):
        for psu in cls._psus:
            try:
                if not psu.get_presence() or not psu.get_status():
                    return "red"
            except Exception:
                return "red"
        return "green"

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        # Try hwmon first; fall back to direct PMBus read (reg 0x8B = READ_VOUT, LINEAR16)
        psu_voltage = self._get_hwmon_value("in{}_input", "vout1", "in")
        if psu_voltage != 0.0:
            return float(psu_voltage) / 1000
        return self._pmbus_read_vout()

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        # Try hwmon first; fall back to direct PMBus read (reg 0x8C = READ_IOUT, LINEAR11)
        psu_current = self._get_hwmon_value("curr{}_input", "iout1", "cur")
        if psu_current != 0.0:
            return float(psu_current) / 1000
        return self._pmbus_read_linear11(0x8C)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        # Try hwmon first; fall back to direct PMBus read (reg 0x96 = READ_POUT, LINEAR11)
        psu_power = self._get_hwmon_value("power{}_input", "pout1", "power")
        if psu_power != 0.0:
            return float(psu_power) / 1000000
        return self._pmbus_read_linear11(0x96)

    def _temp_sensor(self):
        """Return (temperature, rated max) in Celsius of the sensor closest to
        its own limit, or None when no sensor can be read.

        The PSU has three sensors, each with its own MFR_MAX_TEMP
        (temp{n}_rated_max: 65/112/115 C on the W-CRPS800N2). A single
        max-of-all threshold would let the 65 C sensor run 50 C over its
        rating, so the sensor with the smallest margin is reported together
        with its own limit.
        """
        best = None
        if not self.hwmon_path or not os.path.isdir(self.hwmon_path):
            return None
        for dirpath, _, files in os.walk(self.hwmon_path):
            for name in files:
                if not (name.startswith("temp") and name.endswith("_input")):
                    continue
                prefix = name[:-len("_input")]
                temp = self.get_attr_value(os.path.join(dirpath, name))
                limit = self.get_attr_value(os.path.join(dirpath, prefix + "_rated_max"))
                try:
                    temp = float(temp) / 1000
                    limit = float(limit) / 1000
                except ValueError:
                    continue
                if limit <= 0:          # -500 = limit not provided
                    continue
                if best is None or limit - temp < best[1] - best[0]:
                    best = (temp, limit)
        return best

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        sensor = self._temp_sensor()
        if sensor is None:
            raise NotImplementedError
        return sensor[0]

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        sensor = self._temp_sensor()
        if sensor is None:
            raise NotImplementedError
        return sensor[1]

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        # MFR_VOUT_MAX (in*_rated_max). The VOUT_OV/UV limits (in*_max,
        # in*_crit, ...) read 0xFFFF on the W-CRPS800N2 (127998 mV), so they
        # are not used. Fall back to the 12 V rail +5% if it cannot be read.
        value = self._get_hwmon_value("in{}_rated_max", "vout1", "in")
        return value / 1000 if value > 0 else 12.6

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        # MFR_VOUT_MIN (in*_rated_min), see get_voltage_high_threshold()
        value = self._get_hwmon_value("in{}_rated_min", "vout1", "in")
        return value / 1000 if value > 0 else 11.4

    def _pmbus_read_vout(self):
        """Read output voltage directly via SMBus (LINEAR16 with VOUT_MODE exponent)."""
        bus = self._smbus()
        if bus is None:
            return 0.0
        try:
            addr = int(PSU_I2C_MAPPING[self.index]['psu_addr'], 16)
            vout_mode = bus.read_byte_data(addr, 0x20)
            exp = vout_mode & 0x1F
            if exp & 0x10:  # 5-bit two's complement
                exp -= 32
            raw = bus.read_word_data(addr, 0x8B)  # READ_VOUT
            return round(raw * (2 ** exp), 2)
        except Exception:
            return 0.0

    def _pmbus_read_linear11(self, reg):
        """Read a PMBus LINEAR11-encoded register directly via SMBus."""
        bus = self._smbus()
        if bus is None:
            return 0.0
        try:
            addr = int(PSU_I2C_MAPPING[self.index]['psu_addr'], 16)
            raw = bus.read_word_data(addr, reg)
            exp = (raw >> 11) & 0x1F
            if exp & 0x10:
                exp -= 32
            mantissa = raw & 0x7FF
            if mantissa & 0x400:  # 11-bit two's complement
                mantissa -= 2048
            return round(mantissa * (2 ** exp), 2)
        except Exception:
            return 0.0

    def _pmbus_read_word(self, reg):
        bus = self._smbus()
        if bus is None:
            return None
        try:
            return bus.read_word_data(int(PSU_I2C_MAPPING[self.index]['psu_addr'], 16), reg)
        except Exception:
            return None

    def _pmbus_read_string(self, reg):
        bus = self._smbus()
        if bus is None:
            return None
        try:
            data = bus.read_block_data(int(PSU_I2C_MAPPING[self.index]['psu_addr'], 16), reg)
            text = bytes(b for b in data if 32 <= b < 127).decode("ascii").strip()
            return text or None
        except Exception:
            return None

    def _info_file(self, what):
        try:
            with open(os.path.join(PSU_INFO_DIR, "psu%d_%s" % (self.index + 1, what))) as f:
                return f.read().strip() or None
        except OSError:
            return None

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if not self.get_presence():
            return "N/A"
        return self._pmbus_read_string(PMBUS_MFR_MODEL) or self._info_file("model") or "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if not self.get_presence():
            return "N/A"
        return self._pmbus_read_string(PMBUS_MFR_SERIAL) or self._info_file("serial") or "N/A"

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def _get_hwmon_value(self, name, label, start):
        value = 0.0

        label_path = self._search_file_by_contain(
            self.hwmon_path, label, start)
        if label_path:
            dir_name = os.path.dirname(label_path)
            basename = os.path.basename(label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            file_path = os.path.join(dir_name, name.format(in_num))
            val = self.get_attr_value(file_path)
            if val != 'ERR':
                value = float(val)
        return value

    def _search_file_by_contain(self, directory, search_str, file_start):
        if not directory or not os.path.isdir(directory):
            return None
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name.startswith(file_start) and search_str in self.get_attr_value(file_path):
                    return file_path
        return None
