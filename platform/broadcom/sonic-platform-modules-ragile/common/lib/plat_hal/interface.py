#!/usr/bin/env python3
#######################################################
#
# interface.py
# Python implementation of the Class interface
#
#######################################################
import collections
from plat_hal.chassisbase import chassisbase
from plat_hal.baseutil import baseutil
from plat_hal.osutil import osutil


def Singleton(cls):
    _instance = {}

    def _singleton(*args, **kargs):
        if cls not in _instance:
            _instance[cls] = cls(*args, **kargs)
        return _instance[cls]

    return _singleton


@Singleton
class interface(object):
    __chas = None
    __error_ret = None

    def __init__(self):
        self.chas = chassisbase()
        self.__error_ret = -99999
        self.__na_ret = 'N/A'

    @property
    def na_ret(self):
        return self.__na_ret

    @na_ret.setter
    def na_ret(self, val):
        self.__na_ret = val

    @property
    def error_ret(self):
        return self.__error_ret

    @error_ret.setter
    def error_ret(self, val):
        self.__error_ret = val

    @property
    def chas(self):
        return self.__chas

    @chas.setter
    def chas(self, val):
        self.__chas = val

    # onie_e2
    def get_onie_e2(self):
        onie_e2_list = self.chas.onie_e2_list
        return onie_e2_list

    def get_onie_e2_path(self, name):
        onie_e2 = self.chas.get_onie_e2_byname(name)
        if onie_e2 is None:
            return None
        return onie_e2.e2_path

    def get_device_airflow(self, name):
        onie_e2 = self.chas.get_onie_e2_byname(name)
        if onie_e2 is None:
            return None
        return onie_e2.airflow

    def get_onie_e2_obj(self, name):
        onie_e2 = self.chas.get_onie_e2_byname(name)
        if onie_e2 is None:
            return None
        onie_e2.get_onie_e2_info()
        return onie_e2

    # temp
    def get_temps(self):
        templist = self.chas.temp_list
        return templist

    def get_temp_total_number(self):
        templist = self.chas.temp_list
        return len(templist)

    def check_temp_id_exist(self, temp_id):
        templist = self.chas.temp_list
        for temp in templist:
            if temp.temp_id == temp_id:
                return True
        return False

    def get_temp_id_number(self):
        templist = self.chas.temp_list
        temp_num = 0
        for i in range(len(templist)):
            temp_id = "TEMP" + str(i + 1)
            ret = self.check_temp_id_exist(temp_id)
            if ret is True:
                temp_num = temp_num + 1
            else:
                return temp_num
        return temp_num

    def get_temp_location(self, temp_name):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.get_location()

    def set_temp_location(self, temp_name, location):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.set_location(location)

    def set_temp_name(self, temp_name, name):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.set_name(name)

    def get_appoint_temp(self, temp_name):
        temp = self.chas.get_led_byname(temp_name)
        return temp.get_temp()

    def set_appoint_temp(self, temp_name, val):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.set_temp(val)

    def get_temp_mintemp(self, temp_name):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.get_mintemp()

    def set_temp_mintemp(self, temp_name, val):
        temp = self.chas.get_temp_byname(temp_name)
        return temp.set_mintemp(val)

    # led
    def get_leds(self):
        ledlist = self.chas.led_list
        return ledlist

    def get_led_total_number(self):
        ledlist = self.chas.led_list
        return len(ledlist)

    def get_led_color(self, led_name):
        led = self.chas.get_led_byname(led_name)
        if led is None:
            return -1
        return led.get_color()

    def get_led_color_by_type(self, led_type):
        ledlist = self.chas.led_list
        ledtmp = None
        for temp in ledlist:
            if temp.led_type == led_type:
                ledtmp = temp
                break
        if ledtmp is None:
            return -1
        return ledtmp.get_color()

    def set_led_color(self, led_name, color):
        led = self.chas.get_led_byname(led_name)
        if led is None:
            return -1
        return led.set_color(color)

    # psu
    def get_psu_total_number(self):
        psulist = self.chas.psu_list
        if psulist is None:
            return -1
        return len(psulist)

    def get_psus(self):
        psulist = self.chas.psu_list
        return psulist

    def get_psu_presence(self, psu_name):
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1
        return psu.present

    def get_psu_fru_info(self, psu_name):
        '''
                    {
                    "Name": "PSU1",
                    "SN": "serial_number_example",    # 'N/A'
                    "PN": "part_number_example",      # 'N/A'
                    "AirFlow": "B2F"                  # 'N/A'
                }
        '''
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1
        psu.get_fru_info()
        psu.get_AirFlow()
        psu.get_psu_display_name()

        dic = collections.OrderedDict()
        dic["Name"] = psu.name
        dic["SN"] = psu.productSerialNumber if (psu.productSerialNumber is not None) else self.na_ret
        dic["PN"] = psu.productPartModelName if (psu.productPartModelName is not None) else self.na_ret
        dic["DisplayName"] = psu.psu_display_name if (psu.psu_display_name is not None) else self.na_ret
        dic["VENDOR"] = psu.productManufacturer if (psu.productManufacturer is not None) else self.na_ret
        dic["HW"] = psu.productVersion if (psu.productVersion is not None) else self.na_ret
        dic["AirFlow"] = psu.AirFlow if (psu.AirFlow is not None) else self.na_ret
        return dic

    def get_psu_input_output_status(self, psu_name):
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1
        psu.InputsCurrent.Value  # just for clear faults
        if (psu.InputStatus is True) and (psu.OutputStatus is True):
            return True
        return False

    def get_psu_status(self, psu_name):
        """
        Get status of a specific PSU
        @return dict of the specific PSU's status, None for failure
                Example return value(all keys are mandatory)
                {
                    "Name": "PSU1",
                    "InputType": "DC",    # "AC" or 'N/A'
                    "InputStatus": True,  # H/W status bit
                    "OutputStatus": True  # H/W status bit
                    "FanSpeed": {
                        "Value": 4000,    # -99999
                        "Min": 2000,      # -99999
                        "Max": 10000      # -99999
                    },
                    "Temperature": {
                        "Value": 40.0,    # -99999.0
                        "Min": -30.0,     # -99999.0
                        "Max": 50.0       # -99999.0
                    }
                }
        """
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1

        dic = collections.OrderedDict()
        # psu.get_Temperature()
        temp_dict = collections.OrderedDict()
        temp_dict['Min'] = psu.Temperature.Min
        temp_dict['Max'] = psu.Temperature.Max
        temp_dict['Value'] = psu.Temperature.Value
        temp_dict['Unit'] = psu.Temperature.Unit
        dic["Temperature"] = temp_dict

        # psu.get_FanSpeed()
        fan_speed_dict = collections.OrderedDict()
        fan_speed_dict['Min'] = psu.FanSpeed.Min
        fan_speed_dict['Max'] = psu.FanSpeed.Max
        fan_speed_dict['Tolerance'] = psu.FanSpeedTolerance
        fan_speed_dict['Value'] = psu.FanSpeed.Value
        fan_speed_dict['Unit'] = psu.FanSpeed.Unit
        dic["FanSpeed"] = fan_speed_dict

        dic["Name"] = psu.name
        dic["InputType"] = psu.InputsType
        dic["InputStatus"] = psu.InputStatus
        dic["OutputStatus"] = psu.OutputStatus
        dic["TempStatus"] = psu.TempStatus
        dic["FanStatus"] = psu.FanStatus
        return dic

    def get_psu_power_status(self, psu_name):
        """
        Get power status of a specific PSU
        @return dict of the specific PSU's power status, None for failure
                Example return value
                {
                    "Name": "PSU1",
                    "Inputs": {
                        "Status": True, # H/W status bit
                        "Type": "DC",   # or "AC" or "N/A"
                        "Voltage": {
                            "Value": 220,       # -1
                            "LowAlarm": 200,    # -1
                            "HighAlarm": 240,   # -1
                            "Unit": "V"
                        },
                        "Current": {
                            "Value": 6.0,       # -99999.0
                            "LowAlarm": 0.2,    # -99999.0
                            "HighAlarm": 7.0,   # -99999.0
                            "Unit": "A"
                        },
                        "Power": {
                            "Value": 1000,      # -99999
                            "LowAlarm": -1,     # -99999
                            "HighAlarm": 1400,  # -99999
                            "Unit": "W"
                       }
                    },
                    "Outputs": {
                        "Status": True,
                        "Voltage": {
                            "Value": 220,
                            "LowAlarm": 200,
                            "HighAlarm": 240,
                            "Unit": "V"
                        },
                        "Current": {
                            "Value": 6.0,
                            "LowAlarm": 0.2,
                            "HighAlarm": 7.0,
                            "Unit": "A"
                        },
                        "Power": {
                            "Value": 1000,
                            "LowAlarm": -1,  # Don't care
                            "HighAlarm": 1400,
                            "Unit": "W"
                        }
                    }
                }
        """
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1

        dic = collections.OrderedDict()
        inputdic = collections.OrderedDict()
        Outputsdic = collections.OrderedDict()
        dic["Name"] = psu.name
        inputdic["Status"] = psu.InputStatus
        inputdic["Type"] = psu.InputsType

        # psu.get_InputsVoltage()
        inputdic_voltage = collections.OrderedDict()

        inputdic_voltage["Value"] = psu.InputsVoltage.Value
        inputdic_voltage["LowAlarm"] = psu.InputsVoltage.Min
        inputdic_voltage["HighAlarm"] = psu.InputsVoltage.Max
        inputdic_voltage["Unit"] = psu.InputsVoltage.Unit

        inputdic["Voltage"] = inputdic_voltage
        inputdic_current = collections.OrderedDict()
        inputdic_current["Value"] = psu.InputsCurrent.Value
        inputdic_current["LowAlarm"] = psu.InputsCurrent.Min
        inputdic_current["HighAlarm"] = psu.InputsCurrent.Max
        inputdic_current["Unit"] = psu.InputsCurrent.Unit
        inputdic["Current"] = inputdic_current

        inputdic_power = collections.OrderedDict()
        inputdic_power["Value"] = psu.InputsPower.Value
        inputdic_power["LowAlarm"] = psu.InputsPower.Min
        inputdic_power["HighAlarm"] = psu.InputsPower.Max
        inputdic_power["Unit"] = psu.InputsPower.Unit
        inputdic["Power"] = inputdic_power
        Outputsdic["Status"] = psu.InputStatus

        outputdic_voltage = collections.OrderedDict()
        outputdic_current = collections.OrderedDict()
        outputdic_power = collections.OrderedDict()

        outputdic_voltage["Value"] = psu.OutputsVoltage.Value
        outputdic_voltage["LowAlarm"] = psu.OutputsVoltage.Min
        outputdic_voltage["HighAlarm"] = psu.OutputsVoltage.Max
        outputdic_voltage["Unit"] = psu.OutputsVoltage.Unit

        outputdic_current["Value"] = psu.OutputsCurrent.Value
        outputdic_current["LowAlarm"] = psu.OutputsCurrent.Min
        outputdic_current["HighAlarm"] = psu.OutputsCurrent.Max
        outputdic_current["Unit"] = psu.OutputsCurrent.Unit

        outputdic_power["Value"] = psu.OutputsPower.Value
        outputdic_power["LowAlarm"] = psu.OutputsPower.Min
        outputdic_power["HighAlarm"] = psu.OutputsPower.Max
        outputdic_power["Unit"] = psu.OutputsPower.Unit

        Outputsdic["Voltage"] = outputdic_voltage
        Outputsdic["Current"] = outputdic_current
        Outputsdic["Power"] = outputdic_power

        dic["Inputs"] = inputdic
        dic["Outputs"] = Outputsdic

        return dic

    def set_psu_fan_speed_pwm(self, psu_name, pwm):
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1
        return psu.set_fan_speed_pwm(pwm)

    def get_psu_fan_speed_pwm(self, psu_name):
        psu = self.chas.get_psu_byname(psu_name)
        if psu is None:
            return -1
        return psu.get_fan_speed_pwm()

    def get_psu_info_all(self):
        """
                        {
                    "Number": 2,
                    "PSU1": {
                        "SN": "serial_number_example",  # 'N/A'
                        "PN": "part_number_example",    # 'N/A'
                        "AirFlow": "intake",            # 'N/A'

                        "FanSpeed": {
                            "Value": 4000,
                            "Min": 2000,
                            "Max": 30000
                        },
                        "Temperature": {
                            "Value": 35.0,
                            "Min": -20.0,
                            "Max": 45.0
                        },
                        "Inputs": {
                            "Status": True, # H/W status bit
                            "Type": "DC",   # or "AC"
                            "Voltage": {
                                "Value": 220,
                                "LowAlarm": 200,
                                "HighAlarm": 240,
                                "Unit": "V"
                            },
                            "Current": {
                                "Value": 6.0,
                                "LowAlarm": 0.2,
                                "HighAlarm": 7.0,
                                "Unit": "A"
                            },
                            "Power": {
                                "Value": 1000,
                                "LowAlarm": -1,
                                "HighAlarm": 1400,
                                "Unit": "W"
                           }
                        },
                        "Outputs": {
                            "Status": True,
                            "Voltage": {
                                "Value": 220,
                                "LowAlarm": 200,
                                "HighAlarm": 240,
                                "Unit": "V"
                            },
                            "Current": {
                                "Value": 6.0,
                                "LowAlarm": 0.2,
                                "HighAlarm": 7.0,
                                "Unit": "A"
                            },
                            "Power": {
                                "Value": 1000,
                                "LowAlarm": -1,  # Don't care
                                "HighAlarm": 1400,
                                "Unit": "W"
                            }
                        }
                    }
                }
        """

        psus = self.get_psus()
        psu_dict = collections.OrderedDict()
        psu_dict['Number'] = len(psus)
        for psu in psus:
            dicttmp = self.get_psu_fru_info(psu.name)
            dicttmp.update(self.get_psu_status(psu.name))
            dicttmp.update(self.get_psu_power_status(psu.name))
            if self.get_psu_presence(psu.name) is True:
                dicttmp['Present'] = 'yes'
            else:
                dicttmp['Present'] = 'no'
            psu_dict[psu.name] = dicttmp
        return psu_dict

    def get_fans(self):
        fanlist = self.chas.fan_list
        return fanlist

    # fan
    def get_fan_total_number(self):
        fanlist = self.chas.fan_list
        if fanlist is None:
            return -1
        return len(fanlist)

    def get_fan_rotor_number(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        ret = fan.rotors
        if ret is None:
            return -1
        return ret

    def get_fan_speed(self, fan_name, rotor_index):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        ret = fan.get_speed(rotor_index)
        if ret is None:
            return -1
        return ret

    def fan_speed_set_level(self, fan_name, rotor_index, level):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        ret = fan.set_speed(rotor_index, level)
        if ret is True:
            return 0
        return -1

    def get_fan_speed_pwm(self, fan_name, rotor_index):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        val = fan.get_speed_pwm(rotor_index)
        if val is False:
            return -1
        return val

    def set_fan_speed_pwm(self, fan_name, rotor_index, pwm):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        if isinstance(pwm, str):
            rate = float(pwm.strip('%s'))
            speed = round(rate * 255 / 100)
        elif isinstance(pwm, int):
            speed = round(pwm * 255 / 100)
        elif isinstance(pwm, float):
            speed = round(pwm * 255 / 100)
        else:
            return -1
        ret = self.fan_speed_set_level(fan.name, rotor_index, speed)
        if ret == 0:
            return 0
        return -1

    def get_fan_watchdog_status(self):
        fan = self.chas.fan_list[0]
        dic = fan.get_watchdog_status()
        if dic is None or dic["support"] is False:
            return self.na_ret
        if dic["open"] is False or dic["work_allow_set"] is True:
            return "Normal"
        if dic["work_full"] is True:
            return "Abnormal"
        return "Abnormal"

    def enable_fan_watchdog(self, enable=True):
        fan = self.chas.fan_list[0]
        ret = fan.enable_watchdog(enable)
        if ret is True:
            return 0
        return -1

    def feed_fan_watchdog(self):
        fan_list = self.chas.fan_list
        if fan_list is None:
            return -1
        for fan in fan_list:
            ret = fan.feed_watchdog()
            if ret is False:
                return -1
        return 0

    def set_fan_led(self, fan_name, color):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        ret = fan.set_led(color)
        if ret is True:
            return 0
        return -1

    def get_fan_led(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return False, 'N/A'
        return fan.get_led()

    def get_fan_presence(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        return fan.get_presence()

    def get_fan_fru_info(self, fan_name):
        """
        Get specific fan's information
                    # Properties
                    "Name": "FAN1",
                    "SN": "serial_number_example",  # 'N/A'
                    "PN": "part_number_exampple",   # 'N/A'
                    "Rotors": 2,                    # -1
                    "AirFlow": "intake",            # 'N/A'
                    "SpeedMin": 2000,               # -1
                    "SpeedMax": 30000               # -1
        """
        fan = self.chas.get_fan_byname(fan_name)
        fan.get_fru_info()
        fan.get_AirFlow()
        fan.get_fan_display_name()

        dic = collections.OrderedDict()
        dic["Name"] = fan.name
        dic["SN"] = fan.productSerialNumber
        if dic["SN"] is None:
            dic["SN"] = self.na_ret
        dic["PN"] = fan.productName
        if dic["PN"] is None:
            dic["PN"] = self.na_ret
        dic["DisplayName"] = fan.fan_display_name
        if dic["DisplayName"] is None:
            dic["DisplayName"] = self.na_ret

        dic["Rotors"] = fan.rotors
        dic["AirFlow"] = fan.AirFlow
        if dic["AirFlow"] is None:
            dic["AirFlow"] = self.na_ret
        dic["SpeedMin"] = fan.SpeedMin
        dic["SpeedMax"] = fan.SpeedMax
        return dic

    def get_fan_eeprom_info(self, fan_name):
        """
        Get specific fan's information
                    # Properties
                    "Name": "M6510-FAN-F",          # 'N/A'
                    "SN": "serial_number_example",  # 'N/A'
                    "HW": "hw_version_exampple",    # 'N/A'
        """
        fan = self.chas.get_fan_byname(fan_name)
        fan.decode_eeprom_info()
        dic = collections.OrderedDict()
        dic["NAME"] = fan.productName
        if dic["NAME"] is None:
            dic["NAME"] = self.na_ret
        dic["SN"] = fan.productSerialNumber
        if dic["SN"] is None:
            dic["SN"] = self.na_ret
        dic["HW"] = fan.hw_version
        if dic["HW"] is None:
            dic["HW"] = self.na_ret

        return dic

    def get_product_fullname(self):
        return baseutil.get_product_fullname()

    def get_fan_status(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        rotorlist = fan.rotor_list
        dic = collections.OrderedDict()
        for rotor in rotorlist:
            dic_val = collections.OrderedDict()
            if rotor.rotor_Running is True:
                dic_val['Running'] = 'yes'
            else:
                dic_val['Running'] = 'no'
            if rotor.rotor_HwAlarm is True:
                dic_val['HwAlarm'] = 'yes'
            else:
                dic_val['HwAlarm'] = 'no'
            dic_val['Speed'] = int(rotor.rotor_Speed.Value)
            dic[rotor.name] = dic_val
        return dic

    def get_fan_rotor_status(self, fan_name, rotor_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        rotorlist = fan.rotor_list
        for rotor in rotorlist:
            if rotor_name == rotor.name:
                if rotor.rotor_Running is True:
                    return True
                return False
        return -1

    def get_fan_roll_status(self, fan_name, rotor_index):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        rotor = fan.get_rotor_index(rotor_index)
        if rotor is None:
            return -1
        if rotor.rotor_Running is True:
            return True
        return False

    def get_fan_info_fru(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        fan.get_fru_info()
        fan.get_AirFlow()
        dic = collections.OrderedDict()
        dic["Name"] = fan.name
        dic["SN"] = fan.productSerialNumber
        if dic["SN"] is None:
            dic["SN"] = self.na_ret
        dic["PN"] = fan.productPartModelName
        if dic["PN"] is None:
            dic["PN"] = self.na_ret
        flag = self.get_fan_presence(fan_name)
        if flag is True:
            dic["Present"] = "yes"
        elif flag is False:
            dic["Present"] = "no"
        else:
            dic["Present"] = self.na_ret
        dic["Rotors"] = fan.rotors
        dic["AirFlow"] = fan.AirFlow
        if dic["AirFlow"] is None:
            dic["AirFlow"] = self.na_ret
        return dic

    # support TLV and FRU FAN E2
    def get_fan_info(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return None
        fan.get_AirFlow()
        dic = self.get_fan_eeprom_info(fan_name)
        flag = self.get_fan_presence(fan_name)
        if flag is True:
            dic["Present"] = "yes"
        elif flag is False:
            dic["Present"] = "no"
        else:
            dic["Present"] = self.na_ret
        dic["Rotors"] = fan.rotors
        dic["AirFlow"] = fan.AirFlow
        if dic["AirFlow"] is None:
            dic["AirFlow"] = self.na_ret
        dic["PowerMax"] = fan.PowerMax
        if dic["PowerMax"] is None:
            dic["PowerMax"] = self.na_ret
        return dic

    def get_fan_info_rotor(self, fan_name):
        fan = self.chas.get_fan_byname(fan_name)
        if fan is None:
            return -1
        rotorlist = fan.rotor_list
        dic = collections.OrderedDict()
        for rotor in rotorlist:
            dic_val = collections.OrderedDict()
            if rotor.rotor_Running is True:
                dic_val['Running'] = 'yes'
            else:
                dic_val['Running'] = 'no'
            if rotor.rotor_HwAlarm is True:
                dic_val['HwAlarm'] = 'yes'
            else:
                dic_val['HwAlarm'] = 'no'
            speed_value = rotor.rotor_Speed.Value
            if speed_value is None:
                dic_val['Speed'] = self.error_ret
            else:
                dic_val['Speed'] = int(speed_value)
            if rotor.SpeedMin is None:
                dic_val['SpeedMin'] = self.error_ret
            else:
                dic_val['SpeedMin'] = rotor.SpeedMin
            if rotor.SpeedMax is None:
                dic_val['SpeedMax'] = self.error_ret
            else:
                dic_val['SpeedMax'] = rotor.SpeedMax
            if rotor.Tolerance is None:
                dic_val['Tolerance'] = self.error_ret
            else:
                dic_val['Tolerance'] = rotor.Tolerance

            dic[rotor.name] = dic_val
        return dic

    def get_fan_info_all(self):
        fanlist = self.chas.fan_list
        dic = collections.OrderedDict()
        dic['Number'] = len(fanlist)
        dic['WatchdogStatus'] = self.get_fan_watchdog_status()
        for fan in fanlist:
            dic[fan.name] = self.get_fan_info(fan.name)
            dic[fan.name].update(self.get_fan_info_rotor(fan.name))
        return dic

    def temp_test(self):
        templist = self.chas.temp_list
        dicret = collections.OrderedDict()

        for temp in templist:
            dic = collections.OrderedDict()
            temp_value = temp.Value
            dic["Value"] = temp_value if (temp_value is not None) else self.error_ret
            dic["LowAlarm"] = temp.Min
            dic["HighAlarm"] = temp.Max
            dicret[temp.name] = dic
        return dicret

    # dcdc
    def get_dcdc_total_number(self):
        dcdclist = self.chas.dcdc_list
        if dcdclist is None:
            return -1
        return len(dcdclist)

    def get_dcdc_by_id(self, dcdc_id):
        dcdclist = self.chas.dcdc_list
        dcdctmp = None
        for dcdc in dcdclist:
            if dcdc.dcdc_id == dcdc_id:
                dcdctmp = dcdc
        dic = collections.OrderedDict()
        if dcdctmp is None:
            dic["Name"] = self.error_ret
            dic["Min"] = self.error_ret
            dic["Max"] = self.error_ret
            dic["Low"] = self.error_ret
            dic["High"] = self.error_ret
            dic["Value"] = self.error_ret
            dic["Unit"] = self.error_ret
        else:
            dic["Name"] = dcdctmp.name
            dic["Min"] = dcdctmp.sensor.Min
            dic["Max"] = dcdctmp.sensor.Max
            dic["Low"] = dcdctmp.sensor.Low
            dic["High"] = dcdctmp.sensor.High
            tmp = dcdctmp.sensor.Value
            if tmp is not None:
                dic['Value'] = tmp
            else:
                dic['Value'] = self.error_ret
            dic["Unit"] = dcdctmp.sensor.Unit
        return dic

    def get_dcdc_all_info(self):
        val_list = collections.OrderedDict()
        dcdclist = self.chas.dcdc_list
        for dcdc in dcdclist:
            dicttmp = {}
            sensorname = "%s" % (dcdc.name)
            dicttmp['Min'] = dcdc.sensor.Min
            dicttmp['Max'] = dcdc.sensor.Max
            tmp = dcdc.sensor.Value
            if tmp is not None:
                dicttmp['Value'] = tmp
            else:
                dicttmp['Value'] = self.error_ret
            dicttmp['Unit'] = dcdc.sensor.Unit
            val_list[sensorname] = dicttmp
        return val_list

    # sensors
    def get_monitor_temp(self, name):
        templist = self.chas.temp_list
        temptmp = None
        for temp in templist:
            if temp.name == name:
                temptmp = temp

        dic = collections.OrderedDict()
        if temptmp is None:
            dic["Min"] = self.error_ret
            dic["Max"] = self.error_ret
            dic["Value"] = self.error_ret
            dic["Unit"] = self.error_ret
        else:
            dic["Min"] = temptmp.Min
            dic["Max"] = temptmp.Max
            temp_value = temptmp.Value
            dic["Value"] = temp_value if (temp_value is not None) else self.error_ret
            dic["Unit"] = temptmp.Unit
        return dic

    def get_monitor_temp_by_id(self, temp_id):
        templist = self.chas.temp_list
        temptmp = None
        for temp in templist:
            if temp.temp_id == temp_id:
                temptmp = temp

        dic = collections.OrderedDict()
        if temptmp is None:
            dic["Name"] = self.error_ret
            dic["Api_name"] = self.error_ret
            dic["Min"] = self.error_ret
            dic["Max"] = self.error_ret
            dic["Low"] = self.error_ret
            dic["High"] = self.error_ret
            dic["Value"] = self.error_ret
            dic["Unit"] = self.error_ret
        else:
            dic["Name"] = temptmp.name
            dic["Api_name"] = temptmp.api_name
            dic["Min"] = temptmp.Min
            dic["Max"] = temptmp.Max
            dic["Low"] = temptmp.Low
            dic["High"] = temptmp.High
            temp_value = temptmp.Value
            dic["Value"] = temp_value if (temp_value is not None) else self.error_ret
            dic["Unit"] = temptmp.Unit
        return dic

    def get_temp_info(self):
        val_list = collections.OrderedDict()
        # temp
        templist = self.chas.temp_list
        for temp in templist:
            dic = collections.OrderedDict()
            dic["Min"] = temp.Min
            dic["Max"] = temp.Max
            dic["Low"] = temp.Low
            dic["High"] = temp.High
            temp_value = temp.Value
            dic["Value"] = temp_value if (temp_value is not None) else self.error_ret
            dic["Unit"] = temp.Unit
            val_list[temp.name] = dic
        return val_list

    def get_sensor_info(self):
        val_list = collections.OrderedDict()
        # temp
        templist = self.chas.temp_list
        for temp in templist:
            dic = collections.OrderedDict()
            dic["Min"] = temp.Min
            dic["Max"] = temp.Max
            dic["Low"] = temp.Low
            dic["High"] = temp.High
            temp_value = temp.Value
            dic["Value"] = temp_value if (temp_value is not None) else self.error_ret
            dic["Unit"] = temp.Unit
            val_list[temp.name] = dic
        # fan
        fanlist = self.chas.fan_list
        for fan in fanlist:
            for rotor in fan.rotor_list:
                sensorname = "%s%s" % (fan.name, rotor.name)
                speed = collections.OrderedDict()
                speed['Min'] = rotor.rotor_Speed.Min
                speed['Max'] = rotor.rotor_Speed.Max
                rotor_speed_Value = rotor.rotor_Speed.Value
                speed['Value'] = rotor_speed_Value if (rotor_speed_Value is not None) else self.error_ret
                speed['Unit'] = rotor.rotor_Speed.Unit
                val_list[sensorname] = speed

        val_list.update(self.get_dcdc_all_info())

        # psu
        psulist = self.chas.psu_list
        for psu in psulist:
            inputdic_voltage = collections.OrderedDict()
            inputdic_current = collections.OrderedDict()
            inputdic_power = collections.OrderedDict()
            outputdic_voltage = collections.OrderedDict()
            outputdic_current = collections.OrderedDict()
            outputdic_power = collections.OrderedDict()
            temperature = collections.OrderedDict()
            fanspeed = collections.OrderedDict()

            psu_temp_value = psu.Temperature.Value
            temperature["Value"] = psu_temp_value if (psu_temp_value is not None) else self.error_ret
            temperature["Min"] = psu.Temperature.Min
            temperature["Max"] = psu.Temperature.Max
            temperature["Unit"] = psu.Temperature.Unit

            fanspeed["Value"] = psu.FanSpeed.Value
            fanspeed["Min"] = psu.FanSpeed.Min
            fanspeed["Max"] = psu.FanSpeed.Max
            fanspeed["Unit"] = psu.FanSpeed.Unit

            psu_inputvoltage_value = psu.InputsVoltage.Value
            inputdic_voltage["Value"] = psu_inputvoltage_value if (
                psu_inputvoltage_value is not None) else self.error_ret
            inputdic_voltage["Min"] = psu.InputsVoltage.Min
            inputdic_voltage["Max"] = psu.InputsVoltage.Max
            inputdic_voltage["Unit"] = psu.InputsVoltage.Unit

            psu_inputcurrent_value = psu.InputsCurrent.Value
            inputdic_current["Value"] = psu_inputcurrent_value if (
                psu_inputcurrent_value is not None) else self.error_ret
            inputdic_current["Min"] = psu.InputsCurrent.Min
            inputdic_current["Max"] = psu.InputsCurrent.Max
            inputdic_current["Unit"] = psu.InputsCurrent.Unit

            psu_inputpower_value = psu.InputsPower.Value
            inputdic_power["Value"] = psu_inputpower_value if (psu_inputpower_value is not None) else self.error_ret
            inputdic_power["Min"] = psu.InputsPower.Min
            inputdic_power["Max"] = psu.InputsPower.Max
            inputdic_power["Unit"] = psu.InputsPower.Unit

            psu_outputvoltage_value = psu.OutputsVoltage.Value
            outputdic_voltage["Value"] = psu_outputvoltage_value if (
                psu_outputvoltage_value is not None) else self.error_ret
            outputdic_voltage["Min"] = psu.OutputsVoltage.Min
            outputdic_voltage["Max"] = psu.OutputsVoltage.Max
            outputdic_voltage["Unit"] = psu.OutputsVoltage.Unit

            psu_outputcurrent_value = psu.OutputsCurrent.Value
            outputdic_current["Value"] = psu_outputcurrent_value if (
                psu_outputcurrent_value is not None) else self.error_ret
            outputdic_current["Min"] = psu.OutputsCurrent.Min
            outputdic_current["Max"] = psu.OutputsCurrent.Max
            outputdic_current["Unit"] = psu.OutputsCurrent.Unit

            psu_outputpower_value = psu.OutputsPower.Value
            outputdic_power["Value"] = psu_outputpower_value if (
                psu_outputpower_value is not None) else self.error_ret
            outputdic_power["Min"] = psu.OutputsPower.Min
            outputdic_power["Max"] = psu.OutputsPower.Max
            outputdic_power["Unit"] = psu.OutputsPower.Unit

            val_list["%s%s" % (psu.name, "Vol_I")] = inputdic_voltage
            val_list["%s%s" % (psu.name, "Curr_I")] = inputdic_current
            val_list["%s%s" % (psu.name, "Power_I")] = inputdic_power
            val_list["%s%s" % (psu.name, "Vol_O")] = outputdic_voltage
            val_list["%s%s" % (psu.name, "Curr_O")] = outputdic_current
            val_list["%s%s" % (psu.name, "Power_O")] = outputdic_power
            val_list["%s%s" % (psu.name, "Fan")] = fanspeed
            val_list["%s%s" % (psu.name, "Temp")] = temperature

        return val_list

    # cpld
    def get_cpld_total_number(self):
        cpldlist = self.chas.cpld_list
        return len(cpldlist)

    def get_cpld_user_reg(self):
        cpld = self.chas.get_cpld_byname("BASE_CPLD")
        if cpld is None:
            return None
        return cpld.get_user_reg()

    def set_cpld_user_reg(self, value):
        if isinstance(value, int) is False:
            baseutil.logger_debug("value must int %s" % type(value))
            return -1
        if (int(value) < 0 or int(value) > 255):
            baseutil.logger_debug("value must [0 - 255]")
            return -1
        cpld = self.chas.get_cpld_byname("BASE_CPLD")
        if cpld is None:
            baseutil.logger_debug("name BASE_CPLD not find")
            return -1
        if cpld.set_user_reg(value) is True:
            return 0
        return -1

    def set_cpld_console_owner(self, owner):
        """
        Set console I/O owner

        @param owner I/O owner of the console, either "cpu" or "bmc"

        @return 0 for success, -1 for failure
        """
        if owner is None:
            baseutil.logger_debug("owner is None")
            return -1
        owner_tuple = ("cpu", "bmc")
        if owner not in owner_tuple:
            baseutil.logger_debug("owner is %s, must cpu or bmc" % owner)
            return -1
        cpld = self.chas.get_cpld_byname("BASE_CPLD")
        if cpld is None:
            baseutil.logger_debug("name BASE_CPLD not find")
            return -1
        if cpld.set_console_owner(owner) is True:
            return 0
        return -1

    def get_cpld_version_by_id(self, cpld_id):
        cpldlist = self.chas.cpld_list
        cpldtmp = None
        for cpld in cpldlist:
            if cpld.cpld_id == cpld_id:
                cpldtmp = cpld

        dic = collections.OrderedDict()
        if cpldtmp is None:
            dic["Name"] = self.na_ret
            dic["Version"] = self.na_ret
            dic["Desc"] = self.na_ret
            dic["Slot"] = None
            dic["Warm"] = None
        else:
            dic["Name"] = cpldtmp.name
            dic["Version"] = cpldtmp.get_version()
            dic["Desc"] = cpldtmp.desc
            dic["Slot"] = cpldtmp.slot
            dic["Warm"] = cpldtmp.warm
        return dic

    def get_cpld_all_version(self):
        """
        Get version of all CPLDs' that can be read from BMC

        @return dict of CPLDs' version or None for failure.
                example outputs:
                {
                    "BASE_CPLD": "0.1",     # or "N/A" for read failure
                    "FAN_CPLD": "0.2"
                }
        """
        cpld_version = {
            "BASE_CPLD": "N/A",
            "FAN_CPLD": "N/A"
        }
        for cpld_name in cpld_version:
            cpld = self.chas.get_cpld_byname(cpld_name)
            if cpld is None:
                baseutil.logger_debug("name %s not find" % cpld_name)
                continue
            cpld_version[cpld_name] = cpld.get_version()
        return cpld_version

    # comp
    def get_comp_total_number(self):
        complist = self.chas.comp_list
        return len(complist)

    def get_comp_list(self):
        return self.chas.comp_list

    def get_comp_id(self, comp):
        return comp.comp_id

    def get_comp_version_by_id(self, comp_id):
        comp_list = self.chas.comp_list
        comptmp = None
        for comp in comp_list:
            if comp.comp_id == comp_id:
                comptmp = comp
                break

        dic = collections.OrderedDict()
        if comptmp is None:
            dic["Name"] = self.na_ret
            dic["Version"] = self.na_ret
            dic["Desc"] = self.na_ret
            dic["Slot"] = None
        else:
            dic["Name"] = comptmp.name
            dic["Version"] = comptmp.get_version()
            dic["Desc"] = comptmp.desc
            dic["Slot"] = comptmp.slot
        return dic

    def get_bmc_productname(self):
        """
        Get product name

        @return product name string, e.g. $(device name)-F-$(VENDOR_NAME), if error return "N/A"
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name bmc(master) not find")
            return self.na_ret
        return bmc.get_productname()

    def call_bmc_diagcmd(self, cmdstr):
        """
        Call BMC diag comman func

        @return ret: 0 sucess , -1 fail
                outmsg: if success is out msg, or fail is err msg
        """
        if (cmdstr is None or cmdstr == ""):
            outmsg = "cmdstr is empty"
            baseutil.logger_debug(outmsg)
            return -1, outmsg
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            outmsg = "name bmc(master) not find"
            baseutil.logger_debug(outmsg)
            return -1, outmsg
        baseutil.logger_debug("call cmdstr %s" % cmdstr)
        return bmc.call_diagcmd(cmdstr)

    def write_bios_version(self, flash, version):
        bios = self.chas.get_bios_byname("master")
        if bios is None:
            baseutil.logger_debug("name bios(master) not find")
            return -1
        return bios.set_bios_version(flash, version)

    def get_bios_version(self):
        bios = self.chas.get_bios_byname("master")
        if bios is None:
            baseutil.logger_debug("name bios(master) not find")
            return -1
        return bios.get_bios_version()

    def get_bios_status(self):
        bios = self.chas.get_bios_byname("master")
        if bios is None:
            baseutil.logger_debug("name bios(master) not find")
            return -1
        return bios.get_bios_boot_status()

    def get_bmc_mac_rov(self):
        """
        Get BMC mac rov

        @return ret: 0 sucess , -1 fail
                outmsg: if success is out msg, or fail is err msg
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            msg = "name master not find"
            baseutil.logger_debug(msg)
            return -1, msg
        return bmc.get_mac_rov()

    def get_bmc_next_boot(self):
        """
        Get next booting flash of BMC

        @return 'master'/'slave' on success, "N/A" for failure
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name master not find")
            return self.na_ret
        return bmc.get_next_boot()

    def set_bmc_next_boot(self, flash):
        """
        Set flash from which next BMC boot

        @param flash Booting flash of BMC, "master" or "slave"

        @return 0 on success, -1 for failure
        """
        flash_status = ("master", "slave")
        if flash is None or flash not in flash_status:
            baseutil.logger_debug("parameter flash illegal, should be [master|slave]")
            return -1
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name master not find")
            return -1
        return bmc.set_next_boot(flash)

    def reboot_bmc(self):
        """
        Reboot running BMC
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name master not find")
            return -1
        return bmc.reboot()

    def get_bmc_info(self):
        """
        Get BMC info

        @return dict of BMC info or None for failure
                    "Version": "1.1.1", # "N/A"
                    "Flash": "master",  # "N/A"
                    "Next": "master"    # "N/A"
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name master not find")
            return self.na_ret
        return bmc.get_info()

    def get_bmc_version_all(self):
        """
        @return dict of BMCs
                {
                    "MasterVersion": "1.1.1",   # "N/A"
                    "SlaveVersion": "1.1.1"     # "N/A"
                }
        """
        bmc = self.chas.get_bmc_byname("master")
        if bmc is None:
            baseutil.logger_debug("name master not find")
            return self.na_ret
        return bmc.get_version_all()

    def bmc_execute_command(self, cmd_str):
        ret, output = osutil.command(cmd_str)
        if ret:
            baseutil.logger_debug("execute %s command failed" % (cmd_str))
        return ret, output

    def get_cpu_reset_num(self):
        """
        Get CPU reset num
        @return CPU reset num on success, -1 for failure
        """
        cpu = self.chas.get_cpu_byname("cpu")
        if cpu is None:
            msg = "name cpu not find"
            baseutil.logger_debug(msg)
            return -1
        return cpu.get_cpu_reset_num()

    def get_cpu_reboot_cause(self):
        """
        Get CPU reboot cause
        @return string of cpu reboot reason
        """
        cpu = self.chas.get_cpu_byname("cpu")
        if cpu is None:
            msg = "name cpu not find"
            baseutil.logger_debug(msg)
            return "Unknown reboot cause"
        return cpu.get_cpu_reboot_cause()

