#!/usr/bin/env python


import logging
import struct

try:
    from sonic_platform_pddf_base.pddf_current_sensor import PddfCurrentSensor
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)

def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_current_dev_attr(pddf_obj, current_index, attr_name):
    """Get CURRENTx dev_attr value from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    current_key = "CURRENT{}".format(current_index + 1)
    dev_attr = pddf_json.get(current_key, {}).get("dev_attr", {})
    if isinstance(dev_attr, dict):
        return dev_attr.get(attr_name)
    return None


def _get_current_dev_attr_int(pddf_obj, current_index, attr_name):
    """Get integer CURRENTx dev_attr value."""
    value = _get_current_dev_attr(pddf_obj, current_index, attr_name)
    if value is None or value == "":
        return None
    return int(str(value), 0)


def _extract_adc_word(raw32, channel):
    """Extract 12-bit ADC value for one channel from packed 32-bit register."""
    # [31:16] is even channel, [15:0] is odd channel.
    if channel % 2 == 0:
        word16 = (raw32 >> 16) & 0xFFFF
    else:
        word16 = raw32 & 0xFFFF
    return word16 & 0x0FFF


class CurrentSensor(PddfCurrentSensor):
    """
    Ciena current sensor backed by MAX11127 ADC via FPGA registers.

    Reads a raw 12-bit ADC count, converts it to millivolts, then
    applies the per-channel scale factor to produce milliamps (mA).
    Appears in 'show platform current'.
    """

    def _read_fpga_reg(self, offset):
        """Read one 32-bit FPGA register from this sensor's regmap path."""
        try:
            with open(self._regmap_read, "rb") as f:
                f.seek(offset)
                data = f.read(4)
                if len(data) == 4:
                    return struct.unpack('<I', data)[0]
        except OSError as e:
            logger.debug("FPGA regmap read at offset 0x%04X failed: %s", offset, e)
        return None

    def _read_adc_channel(self):
        """Read this sensor's MAX11127 ADC channel via FPGA registers.

        Returns:
            int: 12-bit raw ADC count (0-4095), or None on failure.
        """
        if self._adc_reg_base is None or self._adc_reg_stride is None or not self._regmap_read:
            return None
        channel = self._index
        reg_index = channel // 2
        offset = self._adc_reg_base + reg_index * self._adc_reg_stride

        raw32 = self._read_fpga_reg(offset)
        if raw32 is None:
            return None

        return _extract_adc_word(raw32, channel)

    def __init__(self, index, pddf_data, pddf_plugin_data):
        """
        Args:
            index          (int):   Sensor index
            pddf_data      (dict):  PDDF data
            pddf_plugin_data (dict): PDDF plugin data
        """
        PddfCurrentSensor.__init__(self, index, pddf_data, pddf_plugin_data)
        self._index = index
        self._min_recorded = None
        self._max_recorded = None
        pddf_obj = getattr(self, "pddf_obj", pddf_data)

        self._adc_reg_base = _get_current_dev_attr_int(
            pddf_obj, self._index, "ADC_REG_BASE"
        )
        self._adc_reg_stride = _get_current_dev_attr_int(
            pddf_obj, self._index, "ADC_REG_STRIDE"
        )
        self._regmap_read = _get_current_dev_attr(
            pddf_obj, self._index, "REGMAP_READ"
        )

        mv_per_lsb_cfg = _get_current_dev_attr(
            pddf_obj, self._index, "MV_PER_LSB"
        )
        self._mv_per_lsb = float(mv_per_lsb_cfg) if mv_per_lsb_cfg is not None else None

        scale_cfg = _get_current_dev_attr(
            pddf_obj, self._index, "scale"
        )
        self._scale_factor = float(scale_cfg) if scale_cfg is not None else None

        self._model = _get_current_dev_attr(pddf_obj, self._index, "model")

    # ------------------------------------------------------------------
    # DeviceBase methods
    # ------------------------------------------------------------------

    def get_name(self):
        try:
            return PddfCurrentSensor.get_name(self)
        except Exception:
            return f"CURRENT{self._index + 1}"

    def get_presence(self):
        return True

    def get_model(self):
        return self._model or "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    # ------------------------------------------------------------------
    # CurrentSensorBase methods
    # ------------------------------------------------------------------

    def get_value(self):
        """Read current in milliamps.

        For MAX11127 ADC sensors (scale/MV_PER_LSB configured):
            adc_mV = raw_count × MV_PER_LSB
            current_mA = adc_mV × scale_factor

        For PMBUS sensors (no ADC config): falls back to PDDF base class
        which reads current1_input via bmc_cmd.

        Returns:
            A float: current in mA.
        """
        # If ADC parameters are configured, use direct FPGA register read
        if self._adc_reg_base is not None and self._mv_per_lsb is not None and self._scale_factor is not None:
            raw = self._read_adc_channel()
            if raw is None:
                return 0.0
            adc_mv = raw * self._mv_per_lsb
            current_ma = adc_mv * self._scale_factor

            # Track min/max
            if self._min_recorded is None or current_ma < self._min_recorded:
                self._min_recorded = current_ma
            if self._max_recorded is None or current_ma > self._max_recorded:
                self._max_recorded = current_ma

            return float("{:.1f}".format(current_ma))

        # Fall back to PDDF base class (reads current1_input from bmc_cmd)
        value = PddfCurrentSensor.get_value(self)
        if value is not None:
            return float(value)
        return 0.0

    def get_high_threshold(self):
        try: 
            return PddfCurrentSensor.get_high_threshold(self)
        except Exception:
            return None

    def get_low_threshold(self):
        try:
            return PddfCurrentSensor.get_low_threshold(self)
        except Exception:
            return None

    def get_high_critical_threshold(self):
        try:
            return PddfCurrentSensor.get_high_critical_threshold(self)
        except Exception:
            return None

    def get_low_critical_threshold(self):
        try:
            return PddfCurrentSensor.get_low_critical_threshold(self)
        except Exception:
            return None

    def set_high_threshold(self, value):
        return False

    def set_low_threshold(self, value):
        return False

    def get_minimum_recorded(self):
        if self._min_recorded is None:
            self.get_value()
        return self._min_recorded if self._min_recorded is not None else 0.0

    def get_maximum_recorded(self):
        if self._max_recorded is None:
            self.get_value()
        return self._max_recorded if self._max_recorded is not None else 0.0
