#!/usr/bin/python3

import os
from collections import OrderedDict
from platform_util import *
from platform_config import *
import glob

TEMP_FIELD = {
    "name": "alias",
    "value":"value",
    "max": "max",
    "min": "min",
}

FAN_FIELD = {
    "name": "auto_name_by_type_index",
    "fan_type":"model_name",
    "status": {
        "key" : "status",
        "decode_type" : "decode",
        "decode_info" : {"0": "ABSENT", "1": "OK", "2": "NOT OK"}
    },
    "sn": "serial_number",
    "hw_version": "hardware_version",
}

S3IP_FAN_SPEED_FIELD = {
    "speed_front": "motor1/speed",
    "speed_rear": "motor2/speed",
}

RESTFUL_FAN_SPEED_FIELD = {
    "speed_front": {"field_name": "speed", "type":"fan_motor", "gettype":"restful", "motor_index": 1},
    "speed_rear": {"field_name": "speed", "type":"fan_motor", "gettype":"restful", "motor_index": 2},
}

PSU_FIELD = {
    "name": "auto_name_by_type_index",
    "type":"model_name",
    "status": {
        "key" : "hw_status",
        "decode_type" : "decode",
        "decode_info" : {"0": "ABSENT", "1": "OK", "2": "OK", "3": "NOT OK", "4": "NOT OK"}
    },
    "sn": "serial_number",
    "fan_speed": "fan_speed",
    "in_voltage": "in_vol",
    "in_power": "in_power",
    "in_current": "in_curr",
    "out_voltage": "out_vol",
    "out_power": "out_power",
    "out_current": "out_curr",
}

S3IP_PSU_OTHER_FIELD = {
    "temp": "temp1/value",
}

RESTFUL_PSU_OTHER_FIELD = {
    "temp": "temp1",
}

SLOT_FIELD = {
    "name":"auto_name_by_type_index",
    "status":{
        "key" : "status",
        "decode_type" : "decode",
        "decode_info" : {"0": "ABSENT", "1": "OK", "2": "NOT OK"}
    },
    "slot_type": "model_name",
    "sn": "serial_number",
    "hw_version": "hardware_version",
}

DCDC_FIELD = {
    "name": "alias",
    "value": "value",
    "max": "max",
    "min": "min",
}

def deal_config_field(config):
    if config.get("field"):
        return

    dev_type =  config.get("type")
    if dev_type == DEV_TYPE_TEMP:
        config["field"] = TEMP_FIELD
    elif dev_type == DEV_TYPE_PSU:
        if config.get("source") == GET_BY_RESTFUL:
            config["field"] = { **PSU_FIELD, **RESTFUL_PSU_OTHER_FIELD}
        elif config.get("source") == GET_BY_S3IP:
            config["field"] = { **PSU_FIELD, **S3IP_PSU_OTHER_FIELD}
    elif dev_type == DEV_TYPE_FAN:
        if config.get("source") == GET_BY_RESTFUL:
            config["field"] = { **FAN_FIELD, **RESTFUL_FAN_SPEED_FIELD}
        elif config.get("source") == GET_BY_S3IP:
            config["field"] = { **FAN_FIELD, **S3IP_FAN_SPEED_FIELD}
    elif dev_type == DEV_TYPE_SLOT:
        config["field"] = SLOT_FIELD
    elif dev_type == DEV_TYPE_VOL:
        config["field"] = DCDC_FIELD
    elif dev_type == DEV_TYPE_CURR:
        config["field"] = DCDC_FIELD
    elif dev_type == DEV_TYPE_POWER:
        config["field"] = DCDC_FIELD

def deal_class_result(config, ret, val, result):
    include_name = config.get("include_name")
    exclude_name = config.get("exclude_name")
    dev_type = config.get("type")
    index = config.get("index")

    if ret is False:
        err_name = "%s%s" % (dev_type, index)
        err_msg = "get %s fail, reason: %s" % (err_name, val)
        err_dict = {"errmsg" : err_msg, "name": err_name}
        result.append(err_dict)
        return

    if dev_type == DEV_TYPE_VOL:
        val["unit"] = "V"
    if dev_type == DEV_TYPE_CURR:
        val["unit"] = "A" 

    if include_name:
        if val.get("name") in include_name:
            result.append(val)
    elif exclude_name:
        if val.get("name") not in exclude_name:
            result.append(val)
    else:
        result.append(val)

def get_class_info(config):
    result = []
    source = config.get("source", GET_BY_COMMON)
    include_name = config.get("include_name")
    exclude_name = config.get("exclude_name")
    
    if include_name and exclude_name:
        return False, "can not both config include_name and exclude_name"

    if include_name and not isinstance(include_name, list):
        return False, "include_name config err, not list"

    if exclude_name and not isinstance(exclude_name, list):
        return False, "exclude_name config err, not list"

    if source == GET_BY_RESTFUL:
        num_config = {}
        num_config["type"] = config.get("type")
        num_config["field_name"] = "num"
        ret, num = get_single_info_from_restful(num_config)
        if ret is False:
            return False, ("get %s num fail, reason: %s" % (num_config["type"], num))
        deal_config_field(config)

        for i in range(1, num + 1):
            config["index"] = i
            ret, val = get_multiple_info_from_restful(config)
            deal_class_result(config, ret, val, result)
    elif source == GET_BY_S3IP:
        dev_type = config.get("type")
        num_sysfs = "/sys/s3ip/%s/number" % S3IP_PREFIX_DIR_NAME.get(dev_type, dev_type)
        ret, num = read_sysfs(num_sysfs)
        if ret is False:
            return False, ("get %s num fail, reason: %s" % (dev_type, num))
        deal_config_field(config)

        for i in range(1, int(num) + 1):
            config["index"] = i
            ret, val = get_multiple_info_from_s3ip(config)
            deal_class_result(config, ret, val, result)
    else:
        return False, "not support source : %s" % source

    return True, result

def format_temp_val(val, format_type = 1):
    if format_type == 1:
        return float('%.1f' % (float(val)/1000))
    elif format_type == 2:
        return float('%.3f' % (float(val)/1000))
    elif format_type == 3:
        return float('%.1f' % (float(val)/1000/1000))
    else:
        return val

class Platoform_sensor_v2(object):

    # status showed
    __STATUS_OK = "OK"
    __STATUS_ABSENT = "ABSENT"
    __STATUS_NOT_OK = "NOT OK"
    __STATUS_FAILED = "GET FAILED"
    __STATUS_PRESENT = "PRESENT"

    def __init__(self):
        self.config = PLATFORM_SENSORS_CFG

    def print_console(self, msg):
        print(msg)

    def print_platform(self):
        platform_info = getplatform_name()
        self.print_console(platform_info)
        self.print_console("")

    def print_cputemp_sensors(self):
        print_info_str = ""
        toptile = "Onboard coretemp Sensors:"
        errformat = "    {name:<25} : {errmsg}"
        formatstr = "    {name:<20} : {temp} C (high = {max} C , crit = {crit} C )"

        cpu_temp_location = self.config.get("cpu_temp")
        if cpu_temp_location is None:
            return

        locations = glob.glob(cpu_temp_location)
        if len(locations) == 0:
            print(toptile)
            print("%s not found\n" % cpu_temp_location)
            return

        temp_input_list = []
        temp_info = []
        for dirpath, dirnames, filenames in os.walk(locations[0]):
            for file in filenames:
                if file.endswith("input"):
                    temp_input_list.append(os.path.join(dirpath, file))
            temp_input_list = sorted(temp_input_list,reverse=False)

        for i in range(len(temp_input_list)):
            prob_t = {}
            try:
                prob_t["status"] = self.__STATUS_OK
                _, prob_t["name"] = read_sysfs("%s/temp%d_label"%(locations[0], i+1))
                _, prob_t["temp"] = read_sysfs("%s/temp%d_input"%(locations[0], i+1))
                prob_t["temp"] = format_temp_val(prob_t["temp"])
                _, prob_t["alarm"] = read_sysfs("%s/temp%d_crit_alarm"%(locations[0], i+1))
                prob_t["alarm"] = format_temp_val(prob_t["alarm"])
                _, prob_t["crit"] = read_sysfs("%s/temp%d_crit"%(locations[0], i+1))
                prob_t["crit"] = format_temp_val(prob_t["crit"])
                _, prob_t["max"] = read_sysfs("%s/temp%d_max"%(locations[0], i+1))
                prob_t["max"] = format_temp_val(prob_t["max"])
            except Exception as e:
                prob_t["status"] = self.__STATUS_FAILED
                prob_t["errmsg"] = str(e)
            temp_info.append(prob_t)

        if len(temp_info) != 0:
            print_info_str += toptile + '\n'
            for item in temp_info:
                realformat = formatstr if item.get('status') == self.__STATUS_OK else errformat
                print_info_str += realformat.format(**item) + '\n'
        print(print_info_str)

    def print_other_temp(self, other_temp_config, toptile_str=None):
        try:
            if other_temp_config is None:
                return

            print_info_str = ""
            toptile = toptile_str or "Onboard Temperature Sensors:"
            errformat = "    {id:<25} : {errmsg}"
            formatstr = "    {id:<20} : {temp1_input} C (high = {temp1_max} C, low = {temp1_min} C)"

            source = other_temp_config.get("source", GET_BY_COMMON)
            if other_temp_config.get("class"):
                ret, info_list = get_class_info(other_temp_config)
                if not ret:
                    print(toptile)
                    print(info_list  + '\n')
                    return
            else:
                print(toptile)
                print("config: %s err, not support\n" % other_temp_config)
                return

            monitor_sensor = []
            for sensor_info in info_list:
                monitor_one_sensor_dict = OrderedDict()
                monitor_one_sensor_dict['id'] = sensor_info.get("name")
                try:
                    errmsg = sensor_info.get("errmsg")
                    if errmsg is not None:
                        monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                        monitor_one_sensor_dict["errmsg"] = errmsg
                    else:
                        monitor_one_sensor_dict["status"] = self.__STATUS_OK
                        monitor_one_sensor_dict['temp1_input'] = format_temp_val(sensor_info.get("value"))
                        monitor_one_sensor_dict['temp1_max'] = format_temp_val(sensor_info.get("max"))
                        monitor_one_sensor_dict['temp1_min'] = format_temp_val(sensor_info.get("min"))
                except Exception as e:
                    monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                    monitor_one_sensor_dict["errmsg"] = str(e)
                monitor_sensor.append(monitor_one_sensor_dict)

            if len(monitor_sensor) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_sensor:
                    realformat = formatstr if item.get('status') == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item) + '\n'
                self.print_console(print_info_str)
        except Exception as e:
            print(e)
            pass

    def print_fan_sensor(self):
        try:
            fan_config = self.config.get("fan")
            if fan_config is None:
                return

            print_info_str = ""
            toptile = "Onboard fan Sensors:"
            errformat = "    {id} : {status}\n"
            fan_signle_rotor_format = "    {id} : \n"  \
                "        fan_type  : {fan_type}\n"  \
                "        sn        : {sn}\n"  \
                "        hw_version: {hw_version}\n"  \
                "        Speed     : {Speed}\n"     \
                "        status    : {status} \n"
            fan_double_rotor_format = "    {id} : \n"  \
                "        fan_type  : {fan_type}\n"  \
                "        sn        : {sn}\n"  \
                "        hw_version: {hw_version}\n"  \
                "        Speed     :\n"     \
                "            speed_front :{speed_front:<5} RPM\n"     \
                "            speed_rear  :{speed_rear:<5} RPM\n"     \
                "        status    : {status} \n"

            source = fan_config.get("source", GET_BY_COMMON)
            if fan_config.get("class"):
                ret, fan_info_list = get_class_info(fan_config)
                if not ret:
                    print(toptile)
                    print(fan_info_list  + '\n')
                    return
            else:
                print(toptile)
                print("config: %s err, not support\n" % fan_config)
                return

            monitor_fans = []
            for fan in fan_info_list:
                try:
                    monitor_one_fan_dict = OrderedDict()
                    monitor_one_fan_dict["id"] = fan["name"].upper()
                    status = fan.get("status")
                    errmsg = fan.get("errmsg")
                    if errmsg is not None:
                        monitor_one_fan_dict["status"] = errmsg
                    elif status == self.__STATUS_ABSENT:
                        monitor_one_fan_dict["status"] = self.__STATUS_ABSENT
                    elif status == self.__STATUS_NOT_OK:
                        monitor_one_fan_dict["status"] = self.__STATUS_NOT_OK
                    else:
                        monitor_one_fan_dict["status"] = self.__STATUS_OK
                        monitor_one_fan_dict["fan_type"] = fan.get("fan_type")
                        monitor_one_fan_dict["sn"] = fan.get("sn")
                        monitor_one_fan_dict["Speed"] = fan.get("speed")
                        monitor_one_fan_dict["hw_version"] = fan.get("hw_version")
                        monitor_one_fan_dict["speed_rear"] = fan.get("speed_rear")
                        monitor_one_fan_dict["speed_front"] = fan.get("speed_front")
                except Exception as e:
                    monitor_one_fan_dict["status"] = str(e)
                monitor_fans.append(monitor_one_fan_dict)

            if len(monitor_fans) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_fans:
                    if item.get('Speed') is None:
                        realformat = fan_double_rotor_format if item.get('status') == self.__STATUS_OK else errformat
                    else:
                        realformat = fan_signle_rotor_format if item.get('status') == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item)
                self.print_console(print_info_str)
        except Exception as e:
            print(e)
            pass

    def print_psu_sensor(self):
        try:
            psu_config = self.config.get("psu")
            if psu_config is None:
                return

            print_info_str = ""
            toptile = "Onboard Power Supply Unit Sensors:"
            errformat = "    {id} : {status}\n"  # "    {id:<20} : {status}"
            psuformat = "    {id} : \n"  \
                        "        type       :{type}\n"  \
                        "        sn         :{sn}\n"  \
                        "        in_current :{in_current} A\n"  \
                        "        in_voltage :{in_voltage} V\n"     \
                        "        out_current:{out_current} A\n"     \
                        "        out_voltage:{out_voltage} V\n"     \
                        "        temp       :{temp} C        \n"     \
                        "        fan_speed  :{fan_speed} RPM\n"     \
                        "        in_power   :{in_power} W\n"        \
                        "        out_power  :{out_power} W\n"

            source = psu_config.get("source", GET_BY_COMMON)
            if psu_config.get("class"):
                ret, psu_info_list = get_class_info(psu_config)
                if not ret:
                    print(toptile)
                    print(psu_info_list  + '\n')
                    return
            else:
                print(toptile)
                print("config: %s err, not support\n" % psu_config)
                return
            
            monitor_psus = []
            for psu in psu_info_list:
                try:
                    monitor_one_psu_dict = OrderedDict()
                    monitor_one_psu_dict["id"] = psu.get("name").upper()
                    status = psu.get("status")
                    errmsg = psu.get("errmsg")
                    if errmsg is not None:
                        monitor_one_psu_dict["status"] = errmsg
                    elif status == self.__STATUS_ABSENT:
                        monitor_one_psu_dict["status"] = self.__STATUS_ABSENT
                    elif status == self.__STATUS_NOT_OK:
                        monitor_one_psu_dict["status"] = self.__STATUS_NOT_OK
                    else:
                        monitor_one_psu_dict["status"] = self.__STATUS_OK
                        monitor_one_psu_dict["type"] = psu.get("type")
                        monitor_one_psu_dict["sn"] = psu.get("sn")
                        monitor_one_psu_dict["in_current"] = format_temp_val(psu.get("in_current"))
                        monitor_one_psu_dict["in_voltage"] = format_temp_val(psu.get("in_voltage"))
                        monitor_one_psu_dict["out_current"] = format_temp_val(psu.get("out_current"))
                        monitor_one_psu_dict["out_voltage"] = format_temp_val(psu.get("out_voltage"))
                        monitor_one_psu_dict["temp"] = format_temp_val(psu.get("temp"))
                        monitor_one_psu_dict["fan_speed"] = psu.get("fan_speed")
                        monitor_one_psu_dict["in_power"] = format_temp_val(psu.get("in_power"), 3)
                        monitor_one_psu_dict["out_power"] = format_temp_val(psu.get("out_power"), 3)
                except Exception as e:
                    monitor_one_psu_dict["status"] = str(e)
                monitor_psus.append(monitor_one_psu_dict)

            if len(monitor_psus) != 0:
                print_info_str += toptile + '\r\n'
                for item in monitor_psus:
                    realformat = psuformat if item.get('status') == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item)
                self.print_console(print_info_str)
        except Exception as e:
            print(e)
            pass

    def print_slot_sensor(self):
        try:
            slot_config = self.config.get("slot")
            if slot_config is None:
                return

            print_info_str = ""
            toptile = "Onboard slot Sensors:"
            errformat = "    {id} : {status}\n"  # "    {id:<20} : {errmsg}"
            slotformat = "    {id} : \n"  \
                        "        slot_type  :{slot_type}\n"  \
                        "        sn         :{sn}\n"  \
                        "        hw_version :{hw_version} \n"  \
                        "        status     :{status}\n"

            source = slot_config.get("source", GET_BY_COMMON)
            if slot_config.get("class"):
                ret, slot_info_list = get_class_info(slot_config)
                if not ret:
                    print(toptile)
                    print(slot_info_list  + '\n')
                    return
            else:
                print(toptile)
                print("config: %s err, not support\n" % slot_config)
                return
            
            monitor_slots = []
            for slot in slot_info_list:
                try:
                    monitor_one_slot_dict = OrderedDict()
                    monitor_one_slot_dict["id"] = slot.get("name").upper()
                    errmsg = slot.get("errmsg")
                    status = slot.get("status")
                    if errmsg is not None:
                        monitor_one_slot_dict["status"] = errmsg
                    elif status == self.__STATUS_ABSENT:
                        monitor_one_slot_dict["status"] = self.__STATUS_ABSENT
                    elif status == self.__STATUS_NOT_OK:
                        monitor_one_slot_dict["status"] = self.__STATUS_NOT_OK
                    else:
                        monitor_one_slot_dict["status"] = self.__STATUS_OK
                        monitor_one_slot_dict["slot_type"] = slot.get("slot_type")
                        monitor_one_slot_dict["sn"] = slot.get("sn")
                        monitor_one_slot_dict["hw_version"] = slot.get("hw_version")
                except Exception as e:
                    monitor_one_slot_dict["status"] = str(e)
                monitor_slots.append(monitor_one_slot_dict)

            if len(monitor_slots) != 0:
                print_info_str += toptile + '\r\n'
                for item in monitor_slots:
                    realformat = slotformat if item.get('status') == self.__STATUS_OK else errformat
                    print_info_str += realformat.format(**item)
                self.print_console(print_info_str)
        except Exception as e:
            print(e)
            pass


    def print_boarddcdc(self):
        try:
            dcdc_config = self.config.get("dcdc")
            if dcdc_config is None:
                return

            dcdc_config_list = []
            dcdc_info_list_all = []
            print_info_str = ""
            toptile = "Onboard DCDC Sensors:"
            errformat = "    {id:<32} : {errmsg}"
            formatstr = "    {id:<32} : {dcdc_input:<6} {unit:<1} (Min = {dcdc_min:<6} {unit:<1}, Max = {dcdc_max:<6} {unit:<1})"
            nok_formatstr = "    {id:<32} : {dcdc_input:<6} {unit:<1}  (Min = {dcdc_min:<6} {unit:<1} , Max = {dcdc_max:<6} {unit:<1}) (NOT OK)"

            if isinstance(dcdc_config, dict):
                dcdc_config_list.append(dcdc_config)
            elif isinstance(dcdc_config, list):
                dcdc_config_list = dcdc_config
            else:
                print(toptile)
                print("config: %s err, not support\n" % dcdc_config)
                return

            for dcdc_item in dcdc_config_list:
                source = dcdc_item.get("source", GET_BY_COMMON)
                if dcdc_item.get("class"):
                    ret, dcdc_info_list = get_class_info(dcdc_item)
                    if not ret:
                        print(toptile)
                        print(dcdc_info_list  + '\n')
                        return
                    dcdc_info_list_all += dcdc_info_list
                else:
                    print(toptile)
                    print("config: %s err, not support\n" % dcdc_item)
                    return

            monitor_sensor = []
            for sensor_info in dcdc_info_list_all:
                monitor_one_sensor_dict = OrderedDict()
                monitor_one_sensor_dict['id'] = sensor_info.get("name")
                monitor_one_sensor_dict['unit'] = sensor_info.get("unit")
                try:
                    errmsg = sensor_info.get("errmsg")
                    if errmsg is not None:
                        monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                        monitor_one_sensor_dict["errmsg"] = errmsg
                    else:
                        dcdc_min = format_temp_val(sensor_info["min"], 2)
                        dcdc_max = format_temp_val(sensor_info["max"], 2)
                        dcdc_input = format_temp_val(sensor_info["value"], 2)
                        monitor_one_sensor_dict['dcdc_input'] = dcdc_input
                        monitor_one_sensor_dict['dcdc_min'] = dcdc_min
                        monitor_one_sensor_dict['dcdc_max'] = dcdc_max
                        if dcdc_input > dcdc_max or dcdc_input < dcdc_min:
                            monitor_one_sensor_dict["status"] = self.__STATUS_NOT_OK
                        else:
                            monitor_one_sensor_dict["status"] = self.__STATUS_OK
                except Exception as e:
                    monitor_one_sensor_dict["status"] = self.__STATUS_FAILED
                    monitor_one_sensor_dict["errmsg"] = str(e)
                monitor_sensor.append(monitor_one_sensor_dict)

            if len(monitor_sensor) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_sensor:
                    if item.get("status") == self.__STATUS_OK:
                        realformat = formatstr
                    elif item.get("status") == self.__STATUS_NOT_OK:
                        realformat = nok_formatstr
                    else:
                        realformat = errformat
                    print_info_str += realformat.format(**item) + '\n'
                self.print_console(print_info_str)

        except Exception as e:
            print(e)
            pass

    def print_power_sensors(self, power_config, toptile_str=None):
        try:
            if power_config is None:
                return

            print_info_str = ""
            toptile = toptile_str or "Onboard MAC Power Sensors:"
            errformat = "    {id:<20} : {errmsg}"
            formatstr = "    {id:<20} : {power_input} W"

            source = power_config.get("source", GET_BY_COMMON)
            if power_config.get("class"):
                ret, power_info_list = get_class_info(power_config)
                if not ret:
                    print(toptile)
                    print(power_info_list  + '\n')
                    return
            else:
                print(toptile)
                print("config: %s err, not support\n" % power_config)
                return

            monitor_power = []
            for sensor_info in power_info_list:
                monitor_one_power_dict = OrderedDict()
                monitor_one_power_dict['id'] = sensor_info.get("name")
                try:
                    errmsg = sensor_info.get("errmsg")
                    if errmsg is not None:
                        monitor_one_power_dict["status"] = self.__STATUS_FAILED
                        monitor_one_power_dict["errmsg"] = errmsg
                    else:
                        monitor_one_power_dict["status"] = self.__STATUS_OK
                        monitor_one_power_dict['power_input'] = format_temp_val(sensor_info["value"], 3)
                except Exception as e:
                    monitor_one_power_dict["status"] = self.__STATUS_FAILED
                    monitor_one_power_dict["errmsg"] = str(e)
                monitor_power.append(monitor_one_power_dict)

            if len(monitor_power) != 0:
                print_info_str += toptile + '\n'
                for item in monitor_power:
                    if item.get("status") == self.__STATUS_OK:
                        realformat = formatstr
                    else:
                        realformat = errformat
                    print_info_str += realformat.format(**item) + '\n'
                self.print_console(print_info_str)

        except Exception as e:
            print(e)
            pass

    def getsensors(self):
        self.print_platform()
        self.print_cputemp_sensors()
        self.print_other_temp(self.config.get("board_temp"))
        self.print_other_temp(self.config.get("mac_temp"), "Onboard MAC Temperature Sensors:")
        self.print_power_sensors(self.config.get("power"), "Onboard Power Sensors:")
        self.print_power_sensors(self.config.get("mac_power"))
        self.print_fan_sensor()
        self.print_psu_sensor()
        self.print_slot_sensor()
        self.print_boarddcdc()

if __name__ == "__main__":
    Platoform_sensor_v2 = Platoform_sensor_v2()
    Platoform_sensor_v2.getsensors()
