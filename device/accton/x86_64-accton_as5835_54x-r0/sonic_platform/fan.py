"""AS5835 fan monitoring through PDDF's native-driver mappings."""
from sonic_platform_pddf_base.pddf_fan import PddfFan


class Fan(PddfFan):
    def get_name(self):
        if self.is_psu_fan:
            return "PSU-{} FAN-{}".format(self.fans_psu_index, self.fan_index)
        return "FAN-{}{}".format(self.fantray_index, "FR"[self.fan_index - 1])

    def get_presence(self):
        if self.is_psu_fan:
            value = self.pddf_obj.get_attr_name_output(
                "PSU{}".format(self.fans_psu_index), "psu_present")
            return bool(value and value['status'].strip() == '1')
        return super().get_presence()

    def get_fault(self):
        if self.is_psu_fan:
            return self.get_speed_rpm() <= 0
        index = (self.fantray_index - 1) * 2 + self.fan_index
        value = self.pddf_obj.get_attr_name_output('FAN-CTRL', 'fan{}_fault'.format(index))
        # Missing fault data must never be interpreted as healthy.
        return not value or value['status'].strip() != '0'

    def get_status(self):
        return self.get_presence() and self.get_speed_rpm() > 0 and not self.get_fault()

    def get_speed_tolerance(self):
        return 15
