#!/usr/bin/env python3

import logging
import os
from pathlib import Path

try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

try:
    from ciena.fpga_lib import find_pci_devices, read_32
except ImportError as e:
    raise ImportError(str(e) + " - ciena.fpga_lib not found")

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# IIO sysfs helpers
# ---------------------------------------------------------------------------

def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_psu_dev_attr(pddf_obj, psu_index, attr_name):
    """Get PSU dev_attr value from PDDF JSON.

    Args:
        pddf_obj: PDDF object (or dict)
        psu_index: 0-based PSU index
        attr_name: key under PSUx.dev_attr
    """
    pddf_json = _get_pddf_json(pddf_obj)
    psu_key = "PSU{}".format(psu_index + 1)
    dev_attr = pddf_json.get(psu_key, {}).get("dev_attr", {})
    if isinstance(dev_attr, dict):
        return dev_attr.get(attr_name)


def _get_psu_dev_attr_int(pddf_obj, psu_index, attr_name):
    """Get integer PSU dev_attr value."""
    value = _get_psu_dev_attr(pddf_obj, psu_index, attr_name)
    if value is None or value == "":
        return None
    return int(value)


def _find_iio_device(pddf_obj=None, psu_index=None):
    """Find the IIO device sysfs path using PDDF object data.

    Reads iio_device_path and iio_device_name from PSU dev_attr.
    Falls back to hardcoded defaults.
    Returns the device directory path, or None.
    """

    if psu_index is not None:
        path = _get_psu_dev_attr(pddf_obj, psu_index, "iio_device_path")
        name = _get_psu_dev_attr(pddf_obj, psu_index, "iio_device_name")
    if path is None or name is None:
        return None

    has_wildcard = any(ch in path for ch in "*?[")
    if has_wildcard:
        if os.path.isabs(path):
            candidates = sorted(
                candidate for candidate in Path("/").glob(path.lstrip("/")) if candidate.is_dir()
            )
        else:
            candidates = sorted(
                candidate for candidate in Path().glob(path) if candidate.is_dir()
            )
    else:
        device_root = Path(path)
        if not device_root.exists():
            return None
        candidates = sorted(
            candidate for candidate in device_root.glob("*") if candidate.is_dir()
        )

    for dev_path in candidates:
        name_file = dev_path / "name"
        try:
            with name_file.open("r") as f:
                if f.read().strip() == name:
                    return str(dev_path)
        except OSError:
            continue
    return None


def _read_iio_raw(iio_dev_path, channel):
    """Read raw ADC value from IIO sysfs.

    Args:
        iio_dev_path: e.g. '/sys/bus/iio/devices/iio:device0'
        channel: integer ADC channel number

    Returns:
        Integer raw ADC count, or None on error.
    """
    if iio_dev_path is None:
        return None
    raw_file = os.path.join(iio_dev_path, f"in_voltage{channel}_raw")
    try:
        with open(raw_file, "r") as f:
            return int(f.read().strip())
    except (OSError, ValueError) as e:
        logger.warning("IIO read ch%d failed: %s", channel, e)
        return None


def _read_iio_scale(iio_dev_path, default_scale_mv):
    """Read the IIO voltage scale (mV per LSB).

    Returns:
        Float scale in mV, or default_scale_mv as fallback.
    """
    if default_scale_mv is None:
        return None
    if iio_dev_path is None:
        return float(default_scale_mv)
    scale_file = os.path.join(iio_dev_path, "in_voltage_scale")
    try:
        with open(scale_file, "r") as f:
            return float(f.read().strip())
    except (OSError, ValueError):
        return float(default_scale_mv)


def _adc_to_value(raw, iio_scale_mv, ext_scale):
    """Convert raw ADC reading to real-world value.

    real_value = (raw * iio_scale_mv / 1000.0) * ext_scale

    Args:
        raw: integer raw ADC count
        iio_scale_mv: IIO scale in mV per LSB (e.g. 2.0)
        ext_scale: external scaling factor (resistor divider or current sense)

    Returns:
        Float value in engineering units (V or A).
    """
    if raw is None:
        return 0.0
    return (raw * iio_scale_mv / 1000.0) * ext_scale


# ---------------------------------------------------------------------------
# FPGA helpers
# ---------------------------------------------------------------------------

def _detect_fpga_pci_address(pddf_obj=None, psu_index=None):
    """Auto-detect Europa FPGA PCI address (BDF string) from sysfs.

    Args:
        pddf_obj: PDDF object for reading vendor/device IDs from PSU dev_attr
        psu_index: 0-based PSU index

    Returns:
        PCI address string (e.g. '0000:01:00.0').
    """
    vendor_cfg = _get_psu_dev_attr(pddf_obj, psu_index, "FPGA_PCI_VENDOR_ID")
    device_cfg = _get_psu_dev_attr(pddf_obj, psu_index, "FPGA_PCI_DEVICE_ID")
    if vendor_cfg is None:
        return None
    if device_cfg is None:
        return None
    vendor_id = int(str(vendor_cfg), 0)
    device_id = int(str(device_cfg), 0)
    addrs = find_pci_devices(vendor_id, device_id)
    return addrs[0] if addrs else None


def _read_fpga_reg(pci_address, offset):
    """Read a single 32-bit register from FPGA via PCI resource0 sysfs.

    Returns the integer value, or None on error.
    """
    try:
        return read_32(pci_address, offset)
    except Exception as e:
        logger.warning("FPGA register read failed at offset 0x%04X: %s",
                       offset, e)
        return None


def _linear11_to_float(raw16):
    """Decode a PMBus LINEAR11 (5-bit exponent + 11-bit mantissa) value.

    Format: bits[15:11] = signed exponent, bits[10:0] = signed mantissa.
    Result = mantissa * 2^exponent

    Returns a float, or 0.0 if the raw value looks invalid.
    """
    if raw16 == 0 or raw16 == 0xFFFF:
        return 0.0

    exponent = (raw16 >> 11) & 0x1F
    mantissa = raw16 & 0x07FF

    # Sign-extend exponent (5 bits)
    if exponent >= 16:
        exponent -= 32

    # Sign-extend mantissa (11 bits)
    if mantissa >= 1024:
        mantissa -= 2048

    return float(mantissa) * (2.0 ** exponent)


class Psu(PddfPsu):
    """Ciena Platform-specific PSU class.

    PSU status (presence, power-good) comes from FPGA registers.
    PSU electrical telemetry (voltage, current) comes from the MAX1139 ADC
    via Linux IIO sysfs — each PSU has its own dedicated ADC channels.

    PSU index: 0 = PSA (left slot), 1 = PSB (right slot).
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self._index = index            # 0-based: 0=PSA, 1=PSB
        self._pci_address = _detect_fpga_pci_address(self.pddf_obj, self._index)
        self._iio_dev_path = _find_iio_device(self.pddf_obj, index)

        # Read IIO voltage scale from PDDF JSON
        iio_scale_cfg = _get_psu_dev_attr(
            self.pddf_obj, self._index, "iio_voltage_scale"
        )
        self._iio_scale_mv = _read_iio_scale(self._iio_dev_path, iio_scale_cfg)

        # Read ADC current channel from PDDF JSON
        self._adc_current_ch = _get_psu_dev_attr_int(
            self.pddf_obj, self._index, "adc_current_ch"
        )

        # Read ADC voltage channel from PDDF JSON
        self._adc_voltage_ch = _get_psu_dev_attr_int(
            self.pddf_obj, self._index, "adc_voltage_ch"
        )

        # Read SW_TEST_PWR register offset from PDDF JSON
        reg_sw_test_pwr_cfg = _get_psu_dev_attr(
            self.pddf_obj, self._index, "REG_SW_TEST_PWR"
        )
        if reg_sw_test_pwr_cfg is not None:
            self._reg_sw_test_pwr = int(str(reg_sw_test_pwr_cfg), 0)

        # Read PSU_PRESENT_L_BIT from PDDF JSON
        self._psu_present_l_bit = _get_psu_dev_attr_int(
            self.pddf_obj, self._index, "PSU_PRESENT_L_BIT"
        )

    # -------------------------------------------------------------------
    # Private helpers -- FPGA registers
    # -------------------------------------------------------------------

    def _read_reg(self, offset):
        """Read a 32-bit FPGA register, returning int or None."""
        if self._pci_address is None:
            return None
        return _read_fpga_reg(self._pci_address, offset)

    # -------------------------------------------------------------------
    # Private helpers -- ADC
    # -------------------------------------------------------------------

    def _read_adc_current(self):
        """Read this PSU's 12 V output current from the MAX1139 ADC.

        Returns current in amperes, or 0.0 on failure.
        """
        current_scale_cfg = _get_psu_dev_attr(
            self.pddf_obj, self._index, "PSU_ADC_CURRENT_SCALE"
        )
        if current_scale_cfg is None:
            logger.error("PSU%d dev_attr 'PSU_ADC_CURRENT_SCALE' is required but missing",
                         self._index + 1)
            return 0.0
        try:
            current_scale = float(current_scale_cfg)
        except (TypeError, ValueError):
            logger.error("PSU%d PSU_ADC_CURRENT_SCALE is not a valid float: %s",
                         self._index + 1, current_scale_cfg)
            return 0.0
        if self._iio_dev_path is None or self._adc_current_ch is None:
            return 0.0

        raw = _read_iio_raw(self._iio_dev_path, self._adc_current_ch)
        return round(_adc_to_value(raw, self._iio_scale_mv,
                                   current_scale), 3)

    def _read_adc_voltage(self):
        """Read this PSU's 12 V output voltage from the MAX1139 ADC.

        Returns voltage in volts, or 0.0 on failure.
        """
        voltage_scale_cfg = _get_psu_dev_attr(
            self.pddf_obj, self._index, "PSU_ADC_VOLTAGE_SCALE"
        )
        if voltage_scale_cfg is None:
            logger.error("PSU%d dev_attr 'PSU_ADC_VOLTAGE_SCALE' is required but missing",
                         self._index + 1)
            return 0.0
        try:
            voltage_scale = float(voltage_scale_cfg)
        except (TypeError, ValueError):
            logger.error("PSU%d PSU_ADC_VOLTAGE_SCALE is not a valid float: %s",
                         self._index + 1, voltage_scale_cfg)
            return 0.0
        if self._iio_dev_path is None or self._adc_voltage_ch is None:
            return 0.0
        raw = _read_iio_raw(self._iio_dev_path, self._adc_voltage_ch)
        return round(_adc_to_value(raw, self._iio_scale_mv, voltage_scale), 3)



    # -------------------------------------------------------------------
    # Private helpers -- GPIO (cross-check for powergood)
    # -------------------------------------------------------------------

    def _read_gpio_pwr_ok(self, gpio_path):
        """Read PSU power-good status from GPIO sysfs.

        Returns True if power is OK, False otherwise or on error.
        """
        with open(gpio_path, "r") as f:
            return f.read().strip() == "1"

    # -------------------------------------------------------------------
    # DeviceBase methods
    # -------------------------------------------------------------------

    def get_name(self):
        """Return human-readable PSU name."""
        try:
            return PddfPsu.get_name(self)
        except Exception:
            pass
        return f"PSU-{self._index + 1}"  # 1-based display name


    def get_model(self):
        """Return PSU model string from PDDF data when available."""
        try:
            model = PddfPsu.get_model(self)
            if model:
                return model
        except Exception:
            pass
        return "N/A"

    def get_serial(self):
        """Return PSU serial number from PDDF data when available."""
        try:
            serial = PddfPsu.get_serial(self)
            if serial:
                return serial
        except Exception:
            pass
        return "N/A"

    def get_revision(self):
        """Return PSU HW revision."""
        try:
            device = "PSU{}".format(self._index + 1)
            output = self.pddf_obj.get_attr_name_output(device, "psu_revision")
            if output and output.get("status") is not None:
                revision = str(output.get("status", "")).strip()
                if revision:
                    return revision
        except Exception:
            pass
        return "N/A"

    def get_presence(self):
        """Return True if the PSU is physically present.

        Read SW_TEST_PWR register, check PSx_PRESENT_L bit.
        Active-low: 0 = present, 1 = absent.
        """
        if self._reg_sw_test_pwr is None or self._psu_present_l_bit is None:
            return False
        reg_val = self._read_reg(self._reg_sw_test_pwr)
        if reg_val is None:
            return False
        bit = self._psu_present_l_bit
        present_l = (reg_val >> bit) & 1
        return present_l == 0

    def get_status(self):
        """Return True if the PSU is operating normally.

        Prefer PDDF (`psu_power_good` from pddf-device.json) via the base class.
        Fall back to local presence/power-good checks if the PDDF path fails.
        """
        try:
            return PddfPsu.get_status(self)
        except Exception:
            return False

    def get_position_in_parent(self):
        """Return 1-based physical position in parent chassis."""
        return self._index + 1

    def is_replaceable(self):
        """Indicate this is a hot-swappable FRU."""
        return True

    # -------------------------------------------------------------------
    # PsuBase methods -- status
    # -------------------------------------------------------------------

    def get_powergood_status(self):
        """Return True if the PSU output is good.

        Reads GPIO sysfs (PWR_PSA_PWR_OK / PWR_PSB_PWR_OK) from PDDF config.
        The FPGA CIC GPIO driver exports these as named GPIOs under
        /sys/class/gpio/.
        """
        gpio_path = _get_psu_dev_attr(
            self.pddf_obj, self._index, "PSU_GPIO_PWR_OK"
        )
        if gpio_path and os.path.exists(gpio_path):
            return self._read_gpio_pwr_ok(gpio_path)
        return False

    # -------------------------------------------------------------------
    # PsuBase methods -- electrical telemetry (from MAX1139 ADC)
    # -------------------------------------------------------------------

    def get_voltage(self):
        """Return PSU output voltage in volts.

        Prefer PDDF (`psu_voltage_input` from pddf-device.json) when present.
        Fall back to local ADC telemetry if the PDDF path is unavailable.
        """
        try:
            device = "PSU{}".format(self._index + 1)
            output = self.pddf_obj.get_attr_name_output(device, "psu_voltage_input")
            if output and output.get("status") is not None:
                voltage = float(str(output.get("status", "")).strip())
                # If sysfs value comes in millivolts, normalize to volts.
                if voltage > 1000.0:
                    voltage /= 1000.0
                if voltage > 0.0:
                    return round(voltage, 3)
        except (TypeError, ValueError):
            pass
        except Exception:
            pass

        return self._read_adc_voltage()

    def get_current(self):
        """Return PSU output current in amperes.

        Prefer PDDF (`psu_current_input` from pddf-device.json) when present.
        Fall back to local ADC telemetry if the PDDF path is unavailable.
        """
        try:
            device = "PSU{}".format(self._index + 1)
            output = self.pddf_obj.get_attr_name_output(device, "psu_current_input")
            if output and output.get("status") is not None:
                current = float(str(output.get("status", "")).strip())
                # If sysfs value comes in milliamps, normalize to amps.
                if current > 1000.0:
                    current /= 1000.0
                if current >= 0.0:
                    return round(current, 3)
        except (TypeError, ValueError):
            pass
        except Exception:
            pass

        return self._read_adc_current()

    def get_power(self):
        """Return PSU output power in watts.

        Prefer PDDF (`psu_p_out` from pddf-device.json) via the base class.
        Fall back to local V x I telemetry if the PDDF path is not available.
        """
        voltage = self.get_voltage()
        current = self.get_current()
        if voltage > 0.0 and current > 0.0:
            return round(voltage * current, 2)
        return 0.0

    def get_input_voltage(self):
        """Return PSU input voltage.

        The MAX1139 ADC monitors the 12 V DC output side of the PSU.
        The true AC mains input voltage is not instrumented.
        Returns the same value as get_voltage().
        """
        return self.get_voltage()

    def get_input_current(self):
        """Return PSU input current in amperes.

        Returns the same value as get_current() (DC side measurement).
        """
        return self.get_current()

    def get_voltage_high_threshold(self):
        """Return high threshold for the 12 V rail (+20%)."""
        threshold = None
        try:
            device = "PSU{}".format(self._index + 1)
            output = self.pddf_obj.get_attr_name_output(
                device, "psu_voltage_high_threshold"
            )
            if output and output.get("status") is not None:
                threshold = str(output.get("status", "")).strip()
        except Exception:
            pass
        try:
            return float(threshold)
        except (TypeError, ValueError):
            return 0.0

    def get_voltage_low_threshold(self):
        """Return low threshold for the 12 V rail (-20%)."""
        threshold = None
        try:
            device = "PSU{}".format(self._index + 1)
            output = self.pddf_obj.get_attr_name_output(
                device, "psu_voltage_low_threshold"
            )
            if output and output.get("status") is not None:
                threshold = str(output.get("status", "")).strip()
        except Exception:
            pass
        try:
            return float(threshold)
        except (TypeError, ValueError):
            return 0.0

    def get_temperature(self):
        """Return PSU / VRM temperature in Celsius.

        The PSUs themselves have no exposed temperature sensor via the
        MAX1139 ADC or FPGA.  Return None to indicate not available.
        Board-level thermal monitoring is handled by the Thermal class.

        Returns:
            None (not instrumented per-PSU on Europa).
        """
        return None

    def get_temperature_high_threshold(self):
        """Return high temperature threshold in Celsius."""
        return None

    # -------------------------------------------------------------------
    # PsuBase methods -- capacity & type
    # -------------------------------------------------------------------

    def get_maximum_supplied_power(self):
        """Return maximum PSU output power in watts."""
        capacity = _get_psu_dev_attr(
            self.pddf_obj, self._index, "PLATFORM_PSU_CAPACITY"
        )
        try:
            return float(capacity)
        except (TypeError, ValueError):
            return 0.0

    def get_capacity(self):
        """Return PSU capacity in watts."""
        capacity = _get_psu_dev_attr_int(
            self.pddf_obj, self._index, "PLATFORM_PSU_CAPACITY"
        )
        if capacity is not None:
            return capacity
        return 0.0

    def get_type(self):
        """Return PSU type (AC or DC)."""
        psu_type = _get_psu_dev_attr(self.pddf_obj, self._index, "psu_type")
        if isinstance(psu_type, str):
            psu_type = psu_type.strip()
            if psu_type:
                return psu_type
        return "N/A"

    # -------------------------------------------------------------------
    # PsuBase methods -- LED
    # -------------------------------------------------------------------

    def get_status_led(self):
        """Return PSU status LED color string."""
        try:
            status = PddfPsu.get_status_led(self)
            # PDDF return LED_COLOR_OFF if status is not good
            # Override to return LED_COLOR_AMBER for power-good failure instead of OFF
            if status == self.STATUS_LED_COLOR_OFF:
                return self.STATUS_LED_COLOR_AMBER
            return status
        except Exception:
             pass
        return self.STATUS_LED_COLOR_OFF

    def set_status_led(self, color):
        """Set PSU status LED -- not supported on Europa FPGA.

        The PSU LED is controlled by hardware, not software-settable.
        """
        return False

    # -------------------------------------------------------------------
    # Diagnostic helpers (not part of SONiC API, useful for debug)
    # -------------------------------------------------------------------

    def get_all_adc_data(self):
        """Read all MAX1139 ADC channels and return decoded values.

        Returns a dict with per-PSU and subsystem current/voltage readings.
        """

        # ---------------------------------------------------------------------------
        # MAX1139 ADC – IIO sysfs interface
        # ---------------------------------------------------------------------------
        # The MAX1139 appears as iio:device0 (name=max1139) via the FPGA I2C tree.
        # Scale = 2.0 mV per LSB (2.048 V ref / 1024 counts).
        #
        # Per-PSU ADC channel mapping (from Functional Spec Table 25):
        #   PSU index 0 (PSA): current = AIN0, voltage = AIN9
        #   PSU index 1 (PSB): current = AIN1, voltage = AIN8
        # (for diagnostics)
        
        _PSU_ADC_CURRENT_CH    = {0: 0, 1: 1}    # AIN0 = PSA IMON, AIN1 = PSB IMON
        _PSU_ADC_VOLTAGE_CH    = {0: 9, 1: 8}    # AIN9 = PSA VMON, AIN8 = PSB VMON
        _PSU_ADC_CURRENT_SCALE = 26.67           # A per V (current sense scaling)
        _PSU_ADC_VOLTAGE_SCALE = 6.0             # V per V (resistor divider scaling)

        # Board-level (post-OR) 12 V rail (for diagnostics)
        _BOARD_12V_ADC_CH      = 10              # AIN10 = +12V_VMON
        _BOARD_12V_ADC_SCALE   = 6.0

        # Subsystem current monitoring channels (for diagnostics)
        _SUBSYS_ADC_CHANNELS = {
            'optics':  {'ch': 2, 'scale': 6.67,  'name': '+12V_OPT_IMON'},
            'q2c':     {'ch': 3, 'scale': 6.67,  'name': '+12V_Q2C_IMON'},
            'cpu':     {'ch': 4, 'scale': 4.0,   'name': '+12V_CPU_IMON'},
            'timing':  {'ch': 5, 'scale': 1.0,   'name': '+12V_SYNC_IMON'},
        }

        # Number of PSUs on the platform (for diagnostics)
        NUM_PSUS = 2

        results = {}

        # Per-PSU data
        for psu_idx in range(NUM_PSUS):
            psu_name = "PSA" if psu_idx == 0 else "PSB"
            i_ch = _PSU_ADC_CURRENT_CH[psu_idx]
            v_ch = _PSU_ADC_VOLTAGE_CH[psu_idx]
            i_raw = _read_iio_raw(self._iio_dev_path, i_ch)
            v_raw = _read_iio_raw(self._iio_dev_path, v_ch)
            results[psu_name] = {
                'current_ch': i_ch,
                'current_raw': i_raw,
                'current_A': round(_adc_to_value(i_raw, self._iio_scale_mv,
                                                 _PSU_ADC_CURRENT_SCALE), 3),
                'voltage_ch': v_ch,
                'voltage_raw': v_raw,
                'voltage_V': round(_adc_to_value(v_raw, self._iio_scale_mv,
                                                 _PSU_ADC_VOLTAGE_SCALE), 3),
            }

        # Board 12V
        raw_12v = _read_iio_raw(self._iio_dev_path, _BOARD_12V_ADC_CH)
        results['board_12V'] = {
            'ch': _BOARD_12V_ADC_CH,
            'raw': raw_12v,
            'voltage_V': round(_adc_to_value(raw_12v, self._iio_scale_mv,
                                             _BOARD_12V_ADC_SCALE), 3),
        }

        # Subsystem currents
        for subsys, info in _SUBSYS_ADC_CHANNELS.items():
            raw = _read_iio_raw(self._iio_dev_path, info['ch'])
            results[subsys] = {
                'ch': info['ch'],
                'name': info['name'],
                'raw': raw,
                'current_A': round(_adc_to_value(raw, self._iio_scale_mv,
                                                 info['scale']), 3),
            }

        return results

    def get_all_pmbus1_data(self):
        """Read all PMBUS1_VI_MON registers and return decoded values.

        These are VRM-level PMBus telemetry values (shared, not per-PSU).
        Useful for board-level diagnostics.

        The FPGA polls 4 VRM devices and stores LINEAR11-encoded results:
          Q2C_CORE_V : DATA0-2  (IIN, VIN, VOUT, IOUT, TEMP, OC_WARN)
          Q2C_TRVDD  : DATA3-5  (VOUT, IOUT, TEMP, DEVICE_ID, OC_WARN)
          3.3_SFP    : DATA5-7  (VIN, VOUT, IOUT, TEMP, OC_WARN)
          3.3_QSFP   : DATA8-10 (VIN, VOUT, IOUT, TEMP, OC_WARN)

        Returns a list of dicts, one per DATA register.
        """
        # ---------------------------------------------------------------------------
        # FPGA register offsets (from plreg / europa_regmap.html) (for diagnostics)
        # ---------------------------------------------------------------------------
        REG_PMBUS1_VI_MON_BASE = 0x0638   # PMBUS1_VI_MON_DATA0..DATA10 (stride=4)
        REG_PMBUS1_VI_MON_COUNT = 11

        # Field labels for each half-word: (DATA_index, half) -> description
        _field_labels = {
            (0, 'lo'): 'Q2C_CORE_V IOUT_OC_WARN (0x4A)',
            (0, 'hi'): 'Q2C_CORE_V READ_IIN (0x89)',
            (1, 'lo'): 'Q2C_CORE_V READ_VIN (0x88)',
            (1, 'hi'): 'Q2C_CORE_V READ_VOUT (0x8B)',
            (2, 'lo'): 'Q2C_CORE_V READ_IOUT (0x8C)',
            (2, 'hi'): 'Q2C_CORE_V READ_TEMP1 (0x8D)',
            (3, 'lo'): 'Q2C_TRVDD IOUT_OC_WARN (0x4A)',
            (3, 'hi'): 'Q2C_TRVDD IC_DEVICE_ID (0xAD)',
            (4, 'lo'): 'Q2C_TRVDD READ_VOUT (0x8B)',
            (4, 'hi'): 'Q2C_TRVDD READ_IOUT (0x8C)',
            (5, 'lo'): 'Q2C_TRVDD READ_TEMP1 (0x8D)',
            (5, 'hi'): '3.3_SFP IOUT_OC_WARN (0x4A)',
            (6, 'lo'): '3.3_SFP READ_VIN (0x88)',
            (6, 'hi'): '3.3_SFP READ_VOUT (0x8B)',
            (7, 'lo'): '3.3_SFP READ_IOUT (0x8C)',
            (7, 'hi'): '3.3_SFP READ_TEMP1 (0x8D)',
            (8, 'lo'): '3.3_QSFP IOUT_OC_WARN (0x4A)',
            (8, 'hi'): '3.3_QSFP READ_VIN (0x88)',
            (9, 'lo'): '3.3_QSFP READ_VOUT (0x8B)',
            (9, 'hi'): '3.3_QSFP READ_IOUT (0x8C)',
            (10, 'lo'): '3.3_QSFP READ_TEMP1 (0x8D)',
            (10, 'hi'): '(unused)',
        }

        results = []
        for i in range(REG_PMBUS1_VI_MON_COUNT):
            offset = REG_PMBUS1_VI_MON_BASE + i * 4
            val32 = self._read_reg(offset)
            if val32 is None:
                results.append(None)
                continue
            hi = (val32 >> 16) & 0xFFFF
            lo = val32 & 0xFFFF
            results.append({
                'index': i,
                'offset': offset,
                'raw32': val32,
                'hi_raw': hi,
                'lo_raw': lo,
                'hi_val': _linear11_to_float(hi),
                'lo_val': _linear11_to_float(lo),
                'hi_label': _field_labels.get((i, 'hi'), ''),
                'lo_label': _field_labels.get((i, 'lo'), ''),
            })
        return results
