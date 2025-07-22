#!/usr/bin/env python3
import time
import syslog
import redis

mac_mac_temp = "/etc/sonic/max_mac_temp"
redis_local_host = '127.0.0.1'
redis_port = 6379
redis_info_db = 6
period = 3

MACTEMP_DEBUG_FILE = "/etc/.mactemp_debug_flag"

MACTEMPERROR = 1
MACTEMPDEBUG = 2

debuglevel = 0
redis_info_client = redis.Redis(host=redis_local_host, port=redis_port, db=redis_info_db)

def mactemp_info(s):
    syslog.openlog("MACTEMP", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def mactemp_error(s):
    syslog.openlog("MACTEMP", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def mactemp_debug(s):
    if MACTEMPDEBUG & debuglevel:
        syslog.openlog("MACTEMP", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def mactemp_debug_error(s):
    if MACTEMPERROR & debuglevel:
        syslog.openlog("MACTEMP", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)
        
def debug_init():
    global debuglevel
    try:
        with open(MACTEMP_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0

def write_temperature_to_file(file_path, temperature):
    temperature = round(temperature * 1000)
    mactemp_debug(f"write_temperature_to_file: {temperature}")
    with open(file_path, 'w') as f:
        f.write(str(temperature))

def get_info_form_db(redis_client, key):
    try:
        return redis_client.hgetall(key)
    except Exception as e:
        mactemp_debug(f"An error occurred: {e}")
        return None

def get_max_temperature():
    max_value = None
    try:
        mactemp_debug("try to get max temperature.")
        key =  "ASIC_TEMPERATURE_INFO"
        result = get_info_form_db(redis_info_client, key)
        mactemp_debug("get max temperature finish.")
        if result is not None:
            mactemp_debug(f"Hash data for '{key}':")
            for field, value in result.items():
                mactemp_debug(f"{field.decode('utf-8')}: {value.decode('utf-8')}")
                if field.decode('utf-8') == "maximum_temperature":
                    max_value = value.decode('utf-8')
        else:
            mactemp_debug(f"No data found for key '{key}'")

        mactemp_debug("parse info finish.")
    except Exception:
        pass

    return max_value

def generate_mactemp():
    while True:
        max_temperature = get_max_temperature()
        if max_temperature is None:
            mactemp_debug("get_max_temperature fail.")
            max_temperature = 0

        write_temperature_to_file(mac_mac_temp, float(max_temperature))
        time.sleep(period)
    
if __name__ == '__main__':
    debug_init()
    mactemp_debug("enter main")
    generate_mactemp()
