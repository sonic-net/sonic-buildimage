#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

try:
    import importlib.util
    import os
    import sys
    import syslog
    import time
    import subprocess

    import redis
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

HWSKU_PATH = "/usr/share/sonic/hwsku"
PLATFORM_PATH = "/usr/share/sonic/device"

PLATFORM_SPECIFIC_MODULE_NAME = 'sfputil'
PLATFORM_SPECIFIC_CLASS_NAME = 'SfpUtil'

# Global platform-specific sfputil class instance
redis_client = None

dbNameIdMap = {
    "APPL_DB": 0,
    "ASIC_DB": 1,
    "COUNTERS_DB": 2,
    "CONFIG_DB": 4,
    "FLEX_COUNTER_DB": 5,
    "STATE_DB": 6
}

debug_file = "/tmp/tx_temp_debug"
def log_msg(msg, level=syslog.LOG_ERR):
    if os.path.exists(debug_file):
        syslog.openlog("tx_temp")
        syslog.syslog(level, msg)
        syslog.closelog()

def get_machine_info():
    if not os.path.isfile('/host/machine.conf'):
        return None
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars


def get_platform_info(machine_info):
    if machine_info is not None:
        if 'onie_platform' in machine_info:
            return machine_info['onie_platform']
        if 'aboot_platform' in machine_info:
            return machine_info['aboot_platform']
    return None

# Loads platform specific sfputil module from source
def load_platform_sfputil():
    try:
        platform_name = get_platform_info(get_machine_info())
        platform_path = "/".join([PLATFORM_PATH, platform_name])
        module_file = "/".join([platform_path, "plugins", PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
        module = importlib.machinery.SourceFileLoader(PLATFORM_SPECIFIC_MODULE_NAME, module_file).load_module()
        spec = importlib.util.spec_from_file_location(PLATFORM_SPECIFIC_MODULE_NAME, module_file)
        spec.loader.exec_module(module)
    except Exception as e:
        log_msg("Failed to load platform module '%s': %s" % (PLATFORM_SPECIFIC_MODULE_NAME, str(e)))
        return

    assert module is not None

    try:
        platform_sfputil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
        platform_sfputil = platform_sfputil_class()
    except Exception as e:
        log_msg("Failed to instantiate '%s' class: %s" % (PLATFORM_SPECIFIC_CLASS_NAME, str(e)))
        return

    assert platform_sfputil is not None
    return platform_sfputil

def get_phy_highest_temperature():
    command = "bcmcmdb \"dsh -c 'phy user_diag 0 highest_temperature_get'\""
    try:
        result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print("result = {}".format(result))
        if result.returncode != 0:
            log_msg("get phy highest temperature failed", syslog.LOG_DEBUG)
            return 0

        for line in result.stdout.split('\n'):
            if 'highest_temperature:' not in line:
                continue
            temp = float(line.split(':')[-1].strip())
            return temp
        return 0
    except Exception as e:
        log_msg("Get phy higest temperature failed. An error occurred: {}".format(e), syslog.LOG_DEBUG)
        return 0

if __name__ == '__main__':
    db_name = 'STATE_DB'
    table_name = 'TRANSCEIVER_DOM_SENSOR'
    redis_host = '127.0.0.1'
    redis_port = 6379
    period = 10
    exception_period = 30

    while True:
        try:
            platform_sfputil = load_platform_sfputil()
            if redis_client == None:
                # 创建 Redis 客户端
                redis_client = redis.StrictRedis(host=redis_host, port=redis_port, db=dbNameIdMap[db_name])

            # 循环调用接口
            for i in range(40):
                port = i + 1
                # 调用函数接口获取温度
                temperature = platform_sfputil.get_sfp_temperature(port)
                # 检查温度是否为错误值
                if temperature in [-10000, -3000, -9999]:
                    msg = f"Port {port} get temperature failed: {temperature}"
                    log_msg(msg, syslog.LOG_DEBUG)
                    temperature = 0

                msg = f"Port {port} get temperature: {temperature}"
                log_msg(msg, syslog.LOG_DEBUG)
                # 写入 Redis
                key = f"TRANSCEIVER_DOM_SENSOR|ethernet{port}"
                redis_client.hset(key, "temperature", temperature)

            time.sleep(period)
        except Exception as e:
            log_msg(str(e))
            time.sleep(exception_period)


