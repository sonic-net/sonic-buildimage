"""AS5835 YM-1401A PSUs through PDDF and the native PMBus driver."""
import math
from sonic_platform_pddf_base.pddf_psu import PddfPsu


class Psu(PddfPsu):
    def get_name(self):
        return 'PSU-{}'.format(self.psu_index)

    def _read_measurement(self, attribute):
        value = self.pddf_obj.get_attr_name_output(
            'PSU{}'.format(self.psu_index), attribute)
        if not value or isinstance(value['status'], bool):
            return None
        try:
            number = float(value['status'])
        except (ValueError, TypeError):
            return None
        return number / 1000 if math.isfinite(number) and number >= 0 else None

    def get_voltage(self):
        return self._read_measurement('psu_v_out')

    def get_current(self):
        return self._read_measurement('psu_i_out')

    def get_voltage_low_threshold(self):
        return self._read_measurement('psu_v_out_min')

    def get_voltage_high_threshold(self):
        return self._read_measurement('psu_v_out_max')

    def get_power(self):
        # The native ym2651 driver exports mW, not the PDDF default uW.
        return self._read_measurement('psu_p_out')

    def get_capacity(self):
        return int(self.get_maximum_supplied_power())

    def get_type(self):
        return 'AC'

    def get_input_voltage(self):
        raise NotImplementedError('YM-1401A input telemetry is unavailable')

    def get_input_current(self):
        raise NotImplementedError('YM-1401A input telemetry is unavailable')

    def get_input_power(self):
        raise NotImplementedError('YM-1401A input telemetry is unavailable')
