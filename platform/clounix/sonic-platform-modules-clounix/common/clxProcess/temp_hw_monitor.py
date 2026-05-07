#!/usr/bin/env python3

import common
import os
import time
import socket
import threading
import subprocess
from datetime import datetime

reboot = "/usr/local/bin/reboot"
sensor_path = "/sys/switch/sensor/"
num_node = "num_temp_sensors"
dir_name = 'temp{}/'
check_list = {
    'out_put' :'temp_input',
    'warn' : 'temp_max',
    'danger' : 'temp_max_hyst'
}

TEMP_UNIT=1000
PORT=58888

ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"
ADM1166_CAUSE_MSG = "User issued 'adm1166_fault' command [User:{}, Time: {}]"
REBOOT_CAUSE_MSG = "User issued 'temp_ol' command [User: {}, Time: {}]"
THERMAL_OVERLOAD_POSITION_FILE = "/host/reboot-cause/platform/thermal_overload_position"

ADM1166_1_FAULT_HISTORY_FILE = "/host/reboot-cause/platform/adm1166_1_fault_position"
ADM1166_2_FAULT_HISTORY_FILE = "/host/reboot-cause/platform/adm1166_2_fault_position"
ADM1166_1_FAULT_POSITION = "/sys/bus/i2c/devices/i2c-2/2-0034/adm1166_fault_log_addr"
ADM1166_2_FAULT_POSITION = "/sys/bus/i2c/devices/i2c-2/2-0036/adm1166_fault_log_addr"


def _read_text(path):
    with open(path, "r", encoding="utf-8") as fd:
        return fd.read().strip()


def _write_text(path, value):
    with open(path, "w", encoding="utf-8") as fd:
        fd.write(str(value))

def process_adm1166_fault():
    adm1166_1_fault_position = _read_text(ADM1166_1_FAULT_POSITION)

    adm1166_2_fault_position = _read_text(ADM1166_2_FAULT_POSITION)

    fault_1 = 0
    fault_2 = 0

    if os.path.exists(ADM1166_1_FAULT_HISTORY_FILE):
        adm1166_1_fault_position_history = _read_text(ADM1166_1_FAULT_HISTORY_FILE)

        if adm1166_1_fault_position_history[0:2] != adm1166_1_fault_position[0:2]:
            _write_text(ADM1166_1_FAULT_HISTORY_FILE, adm1166_1_fault_position)
            fault_1 = 1

    else:
        _write_text(ADM1166_1_FAULT_HISTORY_FILE, adm1166_1_fault_position)

    if os.path.exists(ADM1166_2_FAULT_HISTORY_FILE):
        adm1166_2_fault_position_history = _read_text(ADM1166_2_FAULT_HISTORY_FILE)

        if adm1166_2_fault_position_history[0:2] != adm1166_2_fault_position[0:2]:
            _write_text(ADM1166_2_FAULT_HISTORY_FILE, adm1166_2_fault_position)
            fault_2 = 1
    else:
        _write_text(ADM1166_2_FAULT_HISTORY_FILE, adm1166_2_fault_position)

    if fault_1 == 0 and fault_2 == 0:
        return

    pos = ""
    if fault_1 != 0:
        pos = pos + " adm1166_1"
    if fault_2 != 0:
        pos = pos + " adm1166_2"

    current_time = datetime.now().strftime("%a %b %d %H:%M:%S %Z %Y").strip()

    msg = ADM1166_CAUSE_MSG.format(pos, current_time)
    _write_text(ADDITIONAL_FAULT_CAUSE_FILE, msg)

def serv_process(sock):
    while 1:
        data = sock.recvfrom(32)
        if data is not None:
            sock.close()
            os._exit(0)

def cause_reboot(pos):
    where = _read_text(sensor_path + dir_name.format(pos) + "temp_type")
    
    current_time = datetime.now().strftime("%a %b %d %H:%M:%S %Z %Y").strip()

    msg = REBOOT_CAUSE_MSG.format(where, current_time)
    _write_text(THERMAL_OVERLOAD_POSITION_FILE, msg)
    subprocess.run([reboot], check=False)
    os._exit(1)

def main():
    ops = common.sys.argv[1]

    host = socket.gethostname()
    ip = socket.gethostbyname(host)

    if ops == 'uninstall':
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        except socket.error as mag:
            print(msg)
            os._exit(1)

        sock.sendto(str("exit").encode(), (ip, PORT))
        sock.close()
        os._exit(0)

    while 1:
        if os.path.exists(ADM1166_1_FAULT_POSITION):
            break

    while 1:
        if os.path.exists(ADM1166_2_FAULT_POSITION):
            break

    process_adm1166_fault()

    while 1:
        if os.path.exists(sensor_path + num_node):
            break
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((ip, PORT))
    except socket.error as msg:
        print(msg)
        os._exit(1)

    t = threading.Thread(target=serv_process, args=(sock,))
    t.start()

    total_num = int(_read_text(sensor_path + num_node))

    while 1:
        for pos in range(total_num):
            curr = int(_read_text(sensor_path + dir_name.format(pos) + check_list['out_put']))
        
            warn = int(_read_text(sensor_path + dir_name.format(pos) + check_list['warn']))
        
            danger = int(_read_text(sensor_path + dir_name.format(pos) + check_list['danger']))

            if (danger <= warn):
                if (curr >= (warn + 5*TEMP_UNIT)):
                    cause_reboot(pos)
            elif (curr >= ((danger+warn)/2)):
                cause_reboot(pos)

        time.sleep(10)

if __name__ == "__main__" :
    main()
