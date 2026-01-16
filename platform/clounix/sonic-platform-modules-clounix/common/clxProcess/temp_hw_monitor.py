#!/usr/bin/env python3

import common
import os
import time
import socket
import threading

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

def process_adm1166_fault():
    fd = os.popen("cat " + ADM1166_1_FAULT_POSITION)
    adm1166_1_fault_position = fd.read()
    adm1166_1_fault_position = adm1166_1_fault_position.strip('\n')

    fd = os.popen("cat " + ADM1166_2_FAULT_POSITION)
    adm1166_2_fault_position = fd.read()
    adm1166_2_fault_position = adm1166_2_fault_position.strip('\n')

    fault_1 = 0
    fault_2 = 0

    if os.path.exists(ADM1166_1_FAULT_HISTORY_FILE):
        fd = os.popen("cat " + ADM1166_1_FAULT_HISTORY_FILE)
        adm1166_1_fault_position_history = fd.read()

        if adm1166_1_fault_position_history[0:2] != adm1166_1_fault_position[0:2]:
            os.system("echo " + adm1166_1_fault_position + " > " + ADM1166_1_FAULT_HISTORY_FILE);
            fault_1 = 1

    else:
        os.system("echo " + adm1166_1_fault_position + " > " + ADM1166_1_FAULT_HISTORY_FILE);

    if os.path.exists(ADM1166_2_FAULT_HISTORY_FILE):
        fd = os.popen("cat " + ADM1166_2_FAULT_HISTORY_FILE)
        adm1166_2_fault_position_history = fd.read()

        if adm1166_2_fault_position_history[0:2] != adm1166_2_fault_position[0:2]:
            os.system("echo " + adm1166_2_fault_position + " > " + ADM1166_2_FAULT_HISTORY_FILE);
            fault_2 = 1
    else:
        os.system("echo " + adm1166_2_fault_position + " > " + ADM1166_2_FAULT_HISTORY_FILE);

    if fault_1 == 0 and fault_2 == 0:
        return

    pos = ""
    if fault_1 != 0:
        pos = pos + " adm1166_1"
    if fault_2 != 0:
        pos = pos + " adm1166_2"

    fd = os.popen("date")
    time = fd.read()
    time = time.strip('\x0a')

    msg = ADM1166_CAUSE_MSG.format(pos, time)
    os.system("echo " + msg + " > " + ADDITIONAL_FAULT_CAUSE_FILE)

def serv_process(sock):
    while 1:
        data = sock.recvfrom(32)
        if data is not None:
            sock.close()
            os._exit(0)

def cause_reboot(pos):
    fd = os.popen("cat " + sensor_path + dir_name.format(pos) + "temp_type")
    where = fd.read()
    where = where.strip('\x0a')
    
    fd = os.popen("date")
    time = fd.read()
    time = time.strip('\x0a')

    msg = REBOOT_CAUSE_MSG.format(where, time)
    os.system("echo \"" + msg + "\" > " + THERMAL_OVERLOAD_POSITION_FILE)
    os.system(reboot)
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

    fd = os.popen("cat " + sensor_path + num_node)
    total_num = fd.read()
    total_num = int(total_num)

    while 1:
        for pos in range(total_num):
            fd = os.popen("cat " + sensor_path + dir_name.format(pos) + check_list['out_put'])
            curr = fd.read()
            curr = int(curr)
        
            fd = os.popen("cat " + sensor_path + dir_name.format(pos) + check_list['warn'])
            warn =  fd.read()
            warn = int(warn)
        
            fd = os.popen("cat " + sensor_path + dir_name.format(pos) + check_list['danger'])
            danger =  fd.read()
            danger = int(danger)

            if (danger <= warn):
                if (curr >= (warn + 5*TEMP_UNIT)):
                    cause_reboot(pos)
            elif (curr >= ((danger+warn)/2)):
                cause_reboot(pos)

        time.sleep(10)

if __name__ == "__main__" :
    main()
