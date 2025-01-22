#!/usr/bin/env python
# -*- coding: UTF-8 -*-
'''
generate board air flow according to fan and psu air flow
write resulet to AIRFLOW_RESULT_FILE, file format:
{
    "FAN1": {
        "model":"M1HFAN I-F",
        "airflow":"intake",
    },
    "PSU1": {
        "model":"CSU550AP-3-500",
        "airflow":"intake",
    },
    "board":"intake"
}
'''
import os
import syslog
import json
from platform_config import AIR_FLOW_CONF, AIRFLOW_RESULT_FILE
from platform_util import dev_file_read, byteTostr
from eepromutil.fru import ipmifru
from eepromutil.fantlv import fan_tlv


AIRFLOW_DEBUG_FILE = "/etc/.airflow_debug_flag"

AIRFLOWERROR = 1
AIRFLOWDEBUG = 2

debuglevel = 0


def airflow_info(s):
    syslog.openlog("AIRFLOW", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def airflow_error(s):
    syslog.openlog("AIRFLOW", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def airflow_debug(s):
    if AIRFLOWDEBUG & debuglevel:
        syslog.openlog("AIRFLOW", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def airflow_debug_error(s):
    if AIRFLOWERROR & debuglevel:
        syslog.openlog("AIRFLOW", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def debug_init():
    global debuglevel
    try:
        with open(AIRFLOW_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def get_model_fru(device, eeprom):
    try:
        fru = ipmifru()
        fru.decodeBin(eeprom)
        dev_name = device.get("name")
        area = device.get("area")
        field = device.get("field")
        tmp_area = getattr(fru, area, None)
        if tmp_area is None:
            msg = "%s fru %s area config error" % (dev_name, area)
            return False, msg
        model = getattr(tmp_area, field, None)
        if model is None:
            msg = "%s get model error, area: %s, field: %s" % (dev_name, area, field)
            return False, msg
        airflow_debug("%s get model success, model: %s" % (dev_name, model))
        return True, model
    except Exception as e:
        return False, str(e)


def get_model_fantlv(device, eeprom):
    try:
        dev_name = device.get("name")
        tlv = fan_tlv()
        rets = tlv.decode(eeprom)
        if len(rets) == 0:
            msg = "%s decode fantlv eeprom info error" % dev_name
            return False, msg

        field = device.get("field")
        for fantlv_item in rets:
            if fantlv_item.get("name") == field:
                return True, fantlv_item["value"]
        msg = "%s get model error, field: %s not found" % (dev_name, field)
        return False, msg
    except Exception as e:
        return False, str(e)


def get_device_modele(device):
    e2_type = device.get("e2_type")
    dev_name = device.get("name")
    support_e2_type = ("fru", "fantlv")
    if e2_type not in support_e2_type:
        msg = "%s unsupport e2_type: %s" % (dev_name, e2_type)
        return False, msg

    e2_path = device.get("e2_path")
    e2_size = device.get("e2_size", 256)
    ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
    if ret is False:
        msg = "%s eeprom read error, eeprom path: %s, msg: %s" % (dev_name, e2_path, binval_bytes)
        return False, msg

    binval = byteTostr(binval_bytes)
    if e2_type == "fru":
        return get_model_fru(device, binval)
    return get_model_fantlv(device, binval)


def get_board_air_flow(fan_intake_num, fan_exhaust_num, psu_intake_num, psu_exhaust_num):
    airflow_debug("fan_intake_num: %d, fan_exhaust_num: %d, psu_intake_num: %d, psu_exhaust_num: %d" %
                  (fan_intake_num, fan_exhaust_num, psu_intake_num, psu_exhaust_num))

    if fan_intake_num == 0 and fan_exhaust_num == 0 and psu_intake_num == 0 and psu_exhaust_num == 0:
        airflow_error("get all fans and psus air flow failed")
        return "N/A"

    if fan_intake_num > fan_exhaust_num:
        airflow_debug("fan intake number %d more than fan exhaust number %s, set board air flow: intake")
        return "intake"

    if fan_intake_num < fan_exhaust_num:
        airflow_debug("fan intake number less than fan exhaust number, set board air flow: exhaust")
        return "exhaust"

    airflow_debug("fan intake number equal to exhaust number, check psu air flow")

    if psu_intake_num > psu_exhaust_num:
        airflow_debug("psu intake number more than psu exhaust number, set board air flow: intake")
        return "intake"

    if psu_intake_num < psu_exhaust_num:
        airflow_debug("psu intake number less than psu exhaust number, set board air flow: exhaust")
        return "exhaust"

    airflow_debug("fan and psu intake and exhaust number equal, return intake")
    return "intake"


def generate_airflow():
    fan_intake_list = []
    fan_exhaust_list = []
    psu_intake_list = []
    psu_exhaust_list = []
    ret = {}
    fans = AIR_FLOW_CONF.get("fans", [])
    psus = AIR_FLOW_CONF.get("psus", [])

    for fan in fans:
        dev_name = fan.get("name")
        air_flow = "N/A"
        status, model = get_device_modele(fan)
        if status is False:
            ret[dev_name] = {"model": "N/A", "airflow": "N/A"}
            airflow_error(model)
            continue
        model = model.strip()
        airflowconifg = AIR_FLOW_CONF[fan["decode"]]
        for key, value in airflowconifg.items():
            if model in value:
                air_flow = key
        ret[dev_name] = {"model": model, "airflow": air_flow}
        airflow_debug("%s model: %s, airflow: %s" % (dev_name, model, air_flow))
        if air_flow == "intake":
            fan_intake_list.append(fan.get("name"))
        elif air_flow == "exhaust":
            fan_exhaust_list.append(fan.get("name"))

    airflow_debug("fan_intake_list: %s" % fan_intake_list)
    airflow_debug("fan_exhaust_list: %s" % fan_exhaust_list)

    for psu in psus:
        dev_name = psu.get("name")
        air_flow = "N/A"
        status, model = get_device_modele(psu)
        if status is False:
            ret[dev_name] = {"model": "N/A", "airflow": "N/A"}
            airflow_error(model)
            continue
        model = model.strip()
        airflowconifg = AIR_FLOW_CONF[psu["decode"]]
        for key, value in airflowconifg.items():
            if model in value:
                air_flow = key
        ret[dev_name] = {"model": model, "airflow": air_flow}
        airflow_debug("%s model: %s, airflow: %s" % (dev_name, model, air_flow))
        if air_flow == "intake":
            psu_intake_list.append(psu.get("name"))
        elif air_flow == "exhaust":
            psu_exhaust_list.append(psu.get("name"))

    airflow_debug("psu_intake_list: %s" % psu_intake_list)
    airflow_debug("psu_exhaust_list: %s" % psu_exhaust_list)

    fan_intake_num = len(fan_intake_list)
    fan_exhaust_num = len(fan_exhaust_list)
    psu_intake_num = len(psu_intake_list)
    psu_exhaust_num = len(psu_exhaust_list)

    board_airflow = get_board_air_flow(fan_intake_num, fan_exhaust_num, psu_intake_num, psu_exhaust_num)
    airflow_debug("board_airflow: %s" % board_airflow)
    ret["board"] = board_airflow
    ret_json = json.dumps(ret, ensure_ascii=False, indent=4)

    out_file_dir = os.path.dirname(AIRFLOW_RESULT_FILE)
    if len(out_file_dir) != 0:
        cmd = "mkdir -p %s" % out_file_dir
        os.system(cmd)
        os.system("sync")
    with open(AIRFLOW_RESULT_FILE, "w") as fd:
        fd.write(ret_json)
    os.system("sync")


if __name__ == '__main__':
    debug_init()
    airflow_debug("enter main")
    generate_airflow()
