#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/7/24 13:32
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from . import helper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST = helper.APIHelper().get_bmc_status()
FAN_STATUS_INFO_CMD = "i2cget -y -f 107 0x0d 0x26"


class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        self.helper = helper.APIHelper()
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal=is_psu_thermal,
                             psu_index=psu_index)

    def get_name(self):
        thermal_name = None
        if self.is_psu_thermal and "thermal_name" in self.plugin_data['PSU']:
            thermal_name = self.plugin_data['PSU']['thermal_name'][str(self.thermals_psu_index)]
        return super().get_name() if thermal_name is None else thermal_name

    def get_high_critical_threshold(self):
        """
        Rewrite the method of obtaining PSU high critical in pddf_thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if BMC_EXIST:
            from . import sensor_list_config
            if not self.is_psu_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_crit_threshold")
                if not output:
                    return None

                if output['status'].isalpha():
                    attr_value = None
                else:
                    attr_value = float(output['status'])

                if output['mode'] == 'bmc':
                    return attr_value
                else:
                    return float(attr_value / 1000)
            else:
                info = self.helper.read_txt_file(sensor_list_config.Sensor_List_Info)
                for line in info.splitlines():
                    if "PSU%d_Temp1" % self.thermals_psu_index in line:
                        return float(line.split("|")[8])
        else:
            return super().get_high_critical_threshold()

    def get_high_threshold(self):
        if BMC_EXIST:
            return super().get_high_threshold()
        else:
            status, fan_info = self.helper.run_command(FAN_STATUS_INFO_CMD)
            if not status:
                return None
            thermal_name = self.get_name()
            fan_dir = "B2F" if (bin(int(fan_info, 16)))[-2:-1] == "0" else "F2B"
            value = self.plugin_data["THERMAL"]["NONE_BMC"]["temp1_high_threshold"][thermal_name][fan_dir]
            return float(value) if value.isdigit() else None

