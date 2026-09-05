#!/usr/bin/env python3
import syslog
import os
import glob

from cpoutil import get_all_oe_vendor_info

OE_AVDD_VOLTAGE_STATIC_PARAMETERS = [
    # OE0_AVDD_RX_1.8_V
    {"loc": "67-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "67-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "67-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE0_AVDD_RX_1.2_V
    {"loc": "67-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "67-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "67-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE0_AVDD_TX_1.8_V
    {"loc": "67-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "67-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "67-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE0_AVDD_TX_1.2_V
    {"loc": "67-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "67-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "67-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE0_AVDD_TRX_0.75_V
    {"loc": "67-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "67-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "67-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE1_AVDD_RX_1.8_V
    {"loc": "68-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "68-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "68-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE1_AVDD_RX_1.2_V
    {"loc": "68-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "68-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "68-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE1_AVDD_TX_1.8_V
    {"loc": "68-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "68-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "68-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE1_AVDD_TX_1.2_V
    {"loc": "68-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "68-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "68-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE1_AVDD_TRX_0.75_V
    {"loc": "68-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "68-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "68-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE2_AVDD_RX_1.8_V
    {"loc": "68-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "68-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "68-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE2_AVDD_RX_1.2_V
    {"loc": "68-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "68-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "68-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE2_AVDD_TX_1.8_V
    {"loc": "68-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "68-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "68-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE2_AVDD_TX_1.2_V
    {"loc": "68-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "68-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "68-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE2_AVDD_TRX_0.75_V
    {"loc": "68-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "68-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "68-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE3_AVDD_RX_1.8_V
    {"loc": "69-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "69-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "69-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE3_AVDD_RX_1.2_V
    {"loc": "69-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "69-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "69-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE3_AVDD_TX_1.8_V
    {"loc": "69-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "69-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "69-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE3_AVDD_TX_1.2_V
    {"loc": "69-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "69-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "69-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE3_AVDD_TRX_0.75_V
    {"loc": "69-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "69-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "69-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE4_AVDD_RX_1.8_V
    {"loc": "69-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "69-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "69-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE4_AVDD_RX_1.2_V
    {"loc": "69-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "69-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "69-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE4_AVDD_TX_1.8_V
    {"loc": "69-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "69-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "69-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE4_AVDD_TX_1.2_V
    {"loc": "69-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "69-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "69-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE4_AVDD_TRX_0.75_V
    {"loc": "69-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "69-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "69-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE5_AVDD_RX_1.8_V
    {"loc": "70-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "70-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "70-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE5_AVDD_RX_1.2_V
    {"loc": "70-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "70-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "70-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE5_AVDD_TX_1.8_V
    {"loc": "70-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "70-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "70-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE5_AVDD_TX_1.2_V
    {"loc": "70-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "70-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "70-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE5_AVDD_TRX_0.75_V
    {"loc": "70-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "70-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "70-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE6_AVDD_RX_1.8_V
    {"loc": "70-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "70-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "70-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE6_AVDD_RX_1.2_V
    {"loc": "70-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "70-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "70-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE6_AVDD_TX_1.8_V
    {"loc": "70-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "70-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "70-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE6_AVDD_TX_1.2_V
    {"loc": "70-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "70-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "70-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE6_AVDD_TRX_0.75_V
    {"loc": "70-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "70-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "70-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # OE7_AVDD_RX_1.8_V
    {"loc": "67-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    {"loc": "67-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    {"loc": "67-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # OE7_AVDD_RX_1.2_V
    {"loc": "67-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "67-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "67-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE7_AVDD_TX_1.8_V
    {"loc": "67-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    {"loc": "67-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "67-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # OE7_AVDD_TX_1.2_V
    {"loc": "67-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    {"loc": "67-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    {"loc": "67-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # OE7_AVDD_TRX_0.75_V
    {"loc": "67-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    {"loc": "67-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    {"loc": "67-005e/hwmon/hwmon*/avs0_vout", "value": "750000"}
]

OE_AVDD_VOLTAGE_CUSTOM_PARAMETERS = {
    # Strict ordering is required for this parameters
    "OE_AVDD_TX_1_8":[
        # OE0_AVDD_TX
        "67-0068/hwmon/hwmon*/avs0_vout",
        # OE1_AVDD_TX
        "68-0068/hwmon/hwmon*/avs0_vout",
        # OE2_AVDD_TX
        "68-0066/hwmon/hwmon*/avs0_vout",
        # OE3_AVDD_TX
        "69-0068/hwmon/hwmon*/avs0_vout",
        # OE4_AVDD_TX
        "69-0066/hwmon/hwmon*/avs0_vout",
        # OE5_AVDD_TX
        "70-0068/hwmon/hwmon*/avs0_vout",
        # OE6_AVDD_TX
        "70-0066/hwmon/hwmon*/avs0_vout",
        # OE7_AVDD_TX
        "67-0066/hwmon/hwmon*/avs0_vout"
    ],
}

CUSTOM_PARA_1_85 = "1850000"
CUSTOM_PARA_1_80 = "1810000"
SPECIAL_SN_CASES = ["SB244800G",
                    "SB2439006",
                    "SB2446006",
                    "SB2448004",
                    "SB244800A",
                    "SB244800F",
                    "SB244800J",
                    "SB244800L",
                    "SB244800R",
                    "SB244800V",]

OE_VOLTAGE_SET_DEBUG_FILE = "/etc/.oe_voltage_set_debug_flag"

debuglevel = 0


def oe_voltage_set_debug(s):
    if debuglevel == 1:
        syslog.openlog("OE_VOLTAGE_SET", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)

def oe_voltage_set_error(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("OE_VOLTAGE_SET", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def oe_voltage_set_info(s):
    syslog.openlog("OE_VOLTAGE_SET", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)

def debug_init():
    global debuglevel
    if os.path.exists(OE_VOLTAGE_SET_DEBUG_FILE):
        debuglevel = 1
    else:
        debuglevel = 0

def write_sysfs_value(reg_name, value):
    mb_reg_file = "/sys/bus/i2c/devices/" + reg_name
    locations = glob.glob(mb_reg_file)
    if len(locations) == 0:
        print("%s not found" % mb_reg_file)
        return False, "{} not found".format(mb_reg_file)
    sysfs_loc = locations[0]
    try:
        with open(sysfs_loc, 'w') as fd:
            fd.write(value)
    except Exception as e:
        return False, e
    return True,None

def set_static_oe_avdd_parameters():
    for para_dic in OE_AVDD_VOLTAGE_STATIC_PARAMETERS:
        oe_voltage_set_debug("Set {} to {}".format(para_dic["loc"], para_dic["value"]))
        ret, log = write_sysfs_value(para_dic["loc"], para_dic["value"])
        if ret is False:
            oe_voltage_set_error("Set {} failed, msg:{}".format(para_dic["loc"] ,log))

def get_OE_AVDD_TX_1_8_value(sn):
    """
    rules:
        format of SN : SByywwZZZ, yy: year, ww: work week, ZZZ: serial number

        the day after 2449(include 2449), return CUSTOM_PARA_1_80
        the dat before 2449, return CUSTOM_PARA_1_85

        Special:
            sn == SB244800G, return CUSTOM_PARA_1_80
            ...
    """
    if sn in SPECIAL_SN_CASES:
        return CUSTOM_PARA_1_80

    try:
        date = int(sn[2:6])
    except Exception as e:
        oe_voltage_set_error("SN {} is not in expected format, return default value 1.8".format(sn))
        # return default value 1.8
        return CUSTOM_PARA_1_80
    
    if date < 2449:
        return CUSTOM_PARA_1_85
    else:
        return CUSTOM_PARA_1_80

def set_OE_AVDD_TX_1_8_parameters():
    try:
        oe_vendor_info_dict = get_all_oe_vendor_info()
    except Exception as e:
        oe_voltage_set_error("Get oe vender info failed: {}".format(e))
        oe_vendor_info_dict = {}

    for i in range(len(OE_AVDD_VOLTAGE_CUSTOM_PARAMETERS["OE_AVDD_TX_1_8"])):
        cpo_vendor_sn = oe_vendor_info_dict.get(i,{}).get("CPO Vendor SN", "")
        if cpo_vendor_sn == "":
            oe_voltage_set_error("Failed to get num {} oe vendor info.".format(i))
            # Set to default value 1.8
            voltage_value = CUSTOM_PARA_1_80
        else:
            voltage_value = get_OE_AVDD_TX_1_8_value(cpo_vendor_sn)

        oe_voltage_set_debug("Set num {} OE0_AVDD_TX1.8 to {}".format(i, voltage_value))
        ret, log = write_sysfs_value(OE_AVDD_VOLTAGE_CUSTOM_PARAMETERS["OE_AVDD_TX_1_8"][i], voltage_value)
        if ret is False:
            oe_voltage_set_error("Set {} failed, msg:{}".format(OE_AVDD_VOLTAGE_CUSTOM_PARAMETERS["OE_AVDD_TX_1_8"][i], log))


if __name__ == '__main__':
    debug_init()

    oe_voltage_set_info("Start setting static parameters of oe")
    set_static_oe_avdd_parameters()
    oe_voltage_set_info("End of setting static parameters of oe")

    oe_voltage_set_info("Start setting custom parameters of oe")
    set_OE_AVDD_TX_1_8_parameters()
    oe_voltage_set_info("End of setting custom parameters of oe")