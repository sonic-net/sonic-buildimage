#!/usr/bin/env python
#
# Copyright (C) 2018 Inventec, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import time
import syslog
import re
from sonic_sfp.bcmshell import bcmshell
from sonic_py_common.general import getstatusoutput_noshell


# =====================================================================
#  global variable init
# =====================================================================
# port object
PORT_LIST               = []
# object is to execute bcm shell command
BCM_SHELL   = None
SHELL_READY = False
# port status that is auto update by chip in data ram
STATUS_RX               = 1<<0
STATUS_TX               = 1<<1
# define data ram address
PORT_DATA_OFFSET_ADDR   = 0xA0
# define board type
INV_MAGNOLIA            = "SONiC-Inventec-d6254qs"
INV_REDWOOD             = "SONiC-Inventec-d7032-100"
INV_CYPRESS             = "SONiC-Inventec-d7054"
INV_MAPLE               = "SONiC-Inventec-d6556"
INV_SEQUOIA             = ""
BOARD_TPYE              = ""
EAGLE_CORE              = []
# define port data for bit streaming
BIT_LINK                = None
BIT_FAULT               = None
BIT_TX                  = None
BIT_RX                  = None
BIT_SPEED0              = None
BIT_SPEED1              = None
# define port speed
SPEED_100G              = 100
SPEED_40G               = 40
SPEED_25G               = 25
SPEED_10G               = 10


# =====================================================================
#  class object
# =====================================================================
class Port():

    port_num        = None
    name            = None
    bcm_id          = None
    led_up          = None
    s_addr          = None
    write2_up       = None
    led_index       = None
    link_status     = None
    speed           = None

    def write_data_ram(self, data):
        BCM_SHELL.cmd("setreg CMIC_LEDUP{0}_DATA_RAM({1}) {2}".format(self.write2_up, self.led_index, data))

    def read_data_ram(self):
        r_string = BCM_SHELL.run("getreg CMIC_LEDUP{0}_DATA_RAM({1})".format(self.led_up, self.s_addr))
        for line in r_string.split("\n"):
            re_obj = re.search(r"\<DATA\=(?P<data>.+)\>", line)
            if re_obj is not None:
                #syslog.syslog(syslog.LOG_DEBUG, "Read Led({0}) data_ram({1}): {2}".format(self.up, addr, re_obj.group("data")))
                return int(re_obj.group("data"), 16)



# =====================================================================
#  Function
# =====================================================================
def _remap_registers(fp):

    content = fp.readlines()
    fp.close()
    err = False

    for line in content:
        try:
            BCM_SHELL.cmd(line.rstrip())
        except Exception, e:
            err = True
            syslog.syslog(syslog.LOG_ERR, "remap register abnormal: {0}".format(str(e)))

    if not err:
        syslog.syslog(syslog.LOG_INFO, "remap Led registers successfully")



def _board_init():

    global BOARD_TPYE
    global BIT_LINK
    global BIT_FAULT
    global BIT_TX
    global BIT_RX
    global BIT_SPEED0
    global BIT_SPEED1
    global EAGLE_CORE
    global TOTAL_SCAN_BITS
    global SYNC_S
    global SYNC_P

    cmd = ["uname", "-n"]
    _, platform = getstatusoutput_noshell(cmd)

    if platform.rstrip() == INV_MAGNOLIA:
        BOARD_TPYE      = "inventec_d6254qs"
        BIT_RX          = 1<<0  #0x01
        BIT_TX          = 1<<1  #0x02
        BIT_SPEED1      = 1<<4  #0x10
        BIT_LINK        = 1<<7  #0x80
        fp = open('/usr/share/sonic/device/x86_64-inventec_d6254qs-r0/led_proc_init.soc', "r")
        _remap_registers(fp)

    elif platform.rstrip() == INV_REDWOOD:
        BOARD_TPYE      = "inventec_d7032q28b"
        BIT_RX          = 1<<0  #0x01
        BIT_TX          = 1<<1  #0x02
        BIT_SPEED0      = 1<<3  #0x08
        BIT_SPEED1      = 1<<4  #0x10
        BIT_FAULT       = 1<<6  #0x40
        BIT_LINK        = 1<<7  #0x80
        EAGLE_CORE      = [66, 100]
        fp = open('/usr/share/sonic/device/x86_64-inventec_d7032q28b-r0/led_proc_init.soc', "r")
        _remap_registers(fp)

    elif platform.rstrip() == INV_CYPRESS:
        BOARD_TPYE      = "inventec_d7054q28b"
        BIT_LINK        = 1<<0  #0x01
        BIT_FAULT       = 1<<1  #0x02
        BIT_SPEED0      = 1<<2  #0x04
        EAGLE_CORE      = [66, 100]

    elif platform.rstrip() == INV_SEQUOIA:
        BOARD_TPYE = "inventec_d7264q28b"

    elif platform.rstrip() == INV_MAPLE:
        BOARD_TPYE = "inventec_d6556"
        fp = open('/usr/share/sonic/device/x86_64-inventec_d6556-r0/led_proc_init.soc', "r")
        _remap_registers(fp)
        #led process: m0 led process that is controlled by linkscan_led_fw.bin and custom_led.bin
        syslog.syslog(syslog.LOG_INFO, "Found device: {0}".format(BOARD_TPYE))
        exit(0)

    else:
        BOARD_TPYE = "not found"
        syslog.syslog(syslog.LOG_ERR, "Found device: {0}".format(BOARD_TPYE))
        exit(0)

    syslog.syslog(syslog.LOG_INFO, "Found device: {0}".format(BOARD_TPYE))



def _lookup_led_index(p):

    index = 0
    if BOARD_TPYE == "inventec_d6254qs":
        if 0 <= p.port_num <= 47:
            index = p.port_num + (p.port_num / 4)
            p.write2_up = 0
        elif 48 <= p.port_num <= 71:
            index = p.port_num - 48
            p.write2_up = 1
        if p.led_up == 0:
            p.s_addr = p.port_num * 2
        elif p.led_up == 1:
            p.s_addr = (p.port_num - 36) * 2

    elif BOARD_TPYE == "inventec_d7032q28b":
        p.write2_up = 0
        index = p.port_num
        if 0 <= p.port_num <= 7:
            p.s_addr = p.port_num * 8
        elif 8 <= p.port_num <= 23:
            p.s_addr = (p.port_num - 8) * 8
        elif 24 <= p.port_num <= 31:
            p.s_addr = (p.port_num - 16) * 8

    else:
        p.write2_up = p.led_up
        for port in PORT_LIST:
            if p.bcm_id == port.bcm_id:
                break
            if p.led_up == port.led_up:
                index += 1

    return PORT_DATA_OFFSET_ADDR + index


def _update_port_list(only_update):

    global PORT_LIST
    number      = 0
    count       = 0

    content = BCM_SHELL.run("ps")
    for line in content.split("\n"):
        re_obj = re.search(r"(?P<port_name>(xe|ce)\d+)\(\s*(?P<bcm_id>\d+)\)\s+(?P<link>(up|down|!ena)).+\s+(?P<speed>\d+)G", line)
        if re_obj is not None:
            if int(re_obj.group("bcm_id")) not in EAGLE_CORE:
                if only_update:
                    PORT_LIST[number].link_status = re_obj.group("link")
                else:
                    # create port object while first time
                    port_obj = Port()
                    port_obj.port_num = number
                    port_obj.name = re_obj.group("port_name")
                    port_obj.bcm_id = int(re_obj.group("bcm_id"))
                    port_obj.link_status = re_obj.group("link")
                    port_obj.speed = int(re_obj.group("speed"))
                    PORT_LIST.append(port_obj)
                number += 1

    if not only_update:
        content = BCM_SHELL.run("led status")
        for line in content.split("\n"):
            re_obj = re.search(r"(?P<bcm_id>\d+).+(?P<led_up>\d)\:", line)
            if re_obj is not None:
                if int(re_obj.group("bcm_id")) not in EAGLE_CORE:
                    PORT_LIST[count].led_up = int(re_obj.group("led_up"))
                    PORT_LIST[count].led_index = _lookup_led_index(PORT_LIST[count])
                    count += 1

        if number is not count:
            PORT_LIST = []
            syslog.syslog(syslog.LOG_ERR, "The amount of port is not match")



def sync_bcmsh_socket():

    global BCM_SHELL
    global SHELL_READY
    waitSyncd   = True
    retryCount  = 0

    while waitSyncd:
        time.sleep(10)
        try:
            BCM_SHELL = bcmshell()
            BCM_SHELL.run("Echo")
            waitSyncd = False
        except Exception, e:
            print "{0}, Retry times({1})".format(str(e),retryCount)
            #syslog.syslog(syslog.LOG_DEBUG, "{0}, Retry times({1})".format(str(e),retryCount))
            retryCount += 1

    syslog.syslog(syslog.LOG_INFO, "bcmshell socket create successfully")

    if SHELL_READY is False:
        SHELL_READY = True
        return
    elif SHELL_READY is True:
        update_led_status()



def update_led_status():

    led_thread      = True  # True/False (gate to turn on/off)
    reset_sec       = 2
    count_down      = 0
    queue_active    = []
    port_data       = None
    s_byte          = None


    # thread for keeping update port status in data ram
    while led_thread:
        try:
            if count_down == 0:
                queue_active = []
                _update_port_list(1)
                for port in PORT_LIST:
                    if port.link_status == "up":
                        queue_active.append(port)
                    else:
                        port_data = 0
                        port.write_data_ram(port_data)
                count_down = reset_sec
            else:
                for port in queue_active:
                    port_data = 0

                    if BOARD_TPYE == "inventec_d6254qs":
                        s_byte = port.read_data_ram()
                        if s_byte&STATUS_RX:
                            port_data |= BIT_RX
                        if s_byte&STATUS_TX:
                            port_data |= BIT_TX
                        port_data |= BIT_LINK

                    elif BOARD_TPYE == "inventec_d7032q28b":
                        s_byte = port.read_data_ram()
                        if s_byte&STATUS_RX:
                            port_data |= BIT_RX
                        if s_byte&STATUS_TX:
                            port_data |= BIT_TX
                        if port.speed == SPEED_100G:
                            port_data |= BIT_SPEED0
                            port_data |= BIT_SPEED1
                        elif port.speed == SPEED_40G:
                            port_data |= BIT_SPEED1
                        elif port.speed == SPEED_25G:
                            port_data |= BIT_SPEED0
                        else:
                            pass
                        port_data |= BIT_LINK

                    elif BOARD_TPYE == "inventec_d7054q28b":
                        if port.speed != SPEED_100G and port.speed != SPEED_25G:
                            port_data |= BIT_SPEED0

                    # write data to update data ram for specific port
                    port.write_data_ram(port_data)

                time.sleep(0.5)
                count_down -= 1

        except Exception, e:
            syslog.syslog(syslog.LOG_WARNING, "{0}".format(str(e)))
            sync_bcmsh_socket()



def debug_print():

    for port in PORT_LIST:
        output = ""
        output += "name:{0} | ".format(port.name)
        output += "port_num:{0} | ".format(port.port_num)
        output += "bcm_id:{0} | ".format(port.bcm_id)
        output += "link_status:{0} | ".format(port.link_status)
        output += "speed:{0} | ".format(port.speed)
        output += "led_up:{0} | ".format(port.led_up)
        output += "s_addr:{0} | ".format(port.s_addr)
        output += "write2_up:{0} | ".format(port.write2_up)
        output += "led_index:{0} | ".format(port.led_index)
        print output


if __name__ == "__main__":

    syslog.openlog("led_proc", syslog.LOG_PID, facility=syslog.LOG_DAEMON)

    sync_bcmsh_socket()
    _board_init()
    _update_port_list(0)
    #debug_print()
    update_led_status()

    syslog.closelog()
