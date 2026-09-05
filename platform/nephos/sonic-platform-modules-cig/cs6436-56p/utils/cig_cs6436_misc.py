#!/usr/bin/env python
#
# Copyright (C) 2018 Cambridge, Inc.
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
import commands
import sys, getopt
import logging
import re
import time
import datetime
from collections import namedtuple
from threading import Thread

DEBUG = False
i2c_prefix = '/sys/bus/i2c/devices/'
leds_prefix = '/sys/devices/platform/cs6436_56p_led/leds/'
fans_prefix = '/sys/devices/platform/cs6436_56p_fan/'
fansdir_prefix = fans_prefix + 'fan{}_direction'

ageing_controlfile = '/etc/sonic/agcontrol'
AGFlag = 0


platform_misc_log = '/var/log/platform_misc.log'
misclogger = logging.getLogger('platform_misc')
misclogger.setLevel(logging.INFO)
miscformatter = logging.Formatter('%(asctime)s-%(levelname)s-%(message)s')

if not os.path.isfile(platform_misc_log):
    try:
        os.mknod(platform_misc_log)
    except:
        print 'Failed to creat platform_misc.log'

fileHandler = logging.FileHandler(platform_misc_log)
fileHandler.setLevel(logging.INFO)
fileHandler.setFormatter(miscformatter)
misclogger.addHandler(fileHandler)


starttime = datetime.datetime.now()
IsGetlswt = 0
coretemp_prefix = '/sys/class/hwmon/hwmon1/'
coretemp_ps = []
psu1_p = '/sys/bus/i2c/devices/5-005a/psu_present'
psu2_p = '/sys/bus/i2c/devices/5-005b/psu_present'
psu1_d = '/sys/bus/i2c/devices/5-0052/psu_eeprom'
psu2_d = '/sys/bus/i2c/devices/5-0053/psu_eeprom'
psu1led_d = leds_prefix + 'cs6436_56p_led::psu1/brightness'
psu2led_d = leds_prefix + 'cs6436_56p_led::psu2/brightness'
cs6436_ledpath = {'fan':leds_prefix + 'cs6436_56p_led::fan/brightness',
           'fan1':leds_prefix + 'cs6436_56p_led::fan1/brightness',
           'fan2':leds_prefix + 'cs6436_56p_led::fan2/brightness',
           'fan3':leds_prefix + 'cs6436_56p_led::fan3/brightness',
           'fan4':leds_prefix + 'cs6436_56p_led::fan4/brightness',
           'fan5':leds_prefix + 'cs6436_56p_led::fan5/brightness',
           'psu1':leds_prefix + 'cs6436_56p_led::psu1/brightness',
           'psu2':leds_prefix + 'cs6436_56p_led::psu2/brightness',
           'sys':leds_prefix + 'cs6436_56p_led::sys/brightness'}


def system_read_filestr(node):
    with open(node, 'r') as f:
        try:
            str = f.read()
        except IOError as e:
            misclogger.error('Failed to get node, str={}'.format(node))
            return "0"
    return str


def system_bright_leds(dev, colour):
    global AGFlag

    if AGFlag == 1:
        return

    cmd = 'echo {} > {}'.format(colour, dev)
    log_os_system(cmd, 1)
    return

'''
1: front in tail out
0: front out tail in
'''
def system_getpsu_direction(dev):
    try:
        with open(dev) as f:
            f.seek(0x30)
            str = f.read(2)
    except IOError as e:
        misclogger.error('Failed to get psu eep')
        return 1
    if str == 'AA':  ## front in tail out
        return 1
    elif str == 'RA':## tail in front out
        return 0
    else:
        misclogger.error('Failed to get psu eep, str={}'.format(str))
        return -1


def system_get_cputype():
    cmdretfd = os.popen("lscpu | grep 'Model name'")
    retstring = cmdretfd.read()
    endindex = retstring.find('@') - 1
    startindex = retstring[:endindex].rfind(' ') + 1
    cputype = retstring[startindex:endindex]

    return cputype


def system_init_coretemppath():
    global coretemp_ps

    cmdstr = "ls {} | grep 'input'".format(coretemp_prefix)
    cmdretfd = os.popen(cmdstr)

    coretemppss = cmdretfd.read().splitlines()
    if len(coretemppss) < 3:
        cputype = system_get_cputype()
        misclogger.error('Failed to init core temperature path.'
                         ' cpu type = {}, num thermal = {}'.format(cputype, len(coretemp_ps)))
        return 1

    for i in range(0,3):
        coretemp_ps.append(coretemp_prefix + coretemppss[i])

    print coretemp_ps

    return 0


class cs6436_fanattr:
    def __init__(self, name):
        self.name = name
        self.direction = 0
        self.direction_p = ''
        self.rear = 0
        self.rear_p = ''
        self.front = 0
        self.front_p = ''
        self.fault = 0
        self.fault_p = ''
        self.status = 0
        self.setpath()
        self.updatedevice()

        return

    def setpath(self):
        self.direction_p = fans_prefix + '{}_direction'.format(self.name)
        self.rear_p = fans_prefix + '{}_rear_speed_rpm'.format(self.name)
        self.front_p = fans_prefix + '{}_front_speed_rpm'.format(self.name)
        self.fault_p = fans_prefix + '{}_fault'.format(self.name)

        return

    def updatedevice(self):
        self.direction = int(system_read_filestr(self.direction_p))
        self.rear = int(system_read_filestr(self.rear_p))
        self.front = int(system_read_filestr(self.front_p))
        self.fault = int(system_read_filestr(self.fault_p))

        return

    def checkspeedrpm(self, speedrpm):
        frontrpmexp = speedrpm * 21000 / 100
        rearrpmexp = speedrpm * 19000 / 100
        deviationfront = abs(frontrpmexp - self.front) / float(frontrpmexp)
        deviationrear = abs(rearrpmexp - self.rear) / float(rearrpmexp)

        if deviationfront < 0.3 and deviationrear < 0.3:
            return 0
        else:
            misclogger.error(':{} speed wrong. frontexp is {}, but rpm is {}.rearexp is {}, but rpm is {}'.format(self.name, frontrpmexp, self.front, rearrpmexp, self.rear))
            return 1

    def checkstatus(self, speedrpm, totaldirct):
        speedstatus = self.checkspeedrpm(speedrpm)
        if self.direction != totaldirct:
            self.status = 1
            misclogger.error(':{} direction = {}.fan direction is not ok.'.format(self.name, self.direction))
        elif speedstatus != 0:
            self.status = 1
        elif self.fault != 0:
            misclogger.error(':{} fault.'.format(self.name))
            self.status = 1
        else:
            self.status = 0

        if self.status == 1:
            system_bright_leds(cs6436_ledpath[self.name], 3)
        else:
            system_bright_leds(cs6436_ledpath[self.name], 1)

        return self.status

cs6436_fanattrnodes = []


class cs6436_psuattr:
    def __init__(self, name):
        self.name = name
        self.direction = 0
        self.direction_p = ''
        self.present = 0
        self.present_p = ''
        self.status = 0

        self.setpath()
        self.updatepresent()
        self.updatedirection()

        return

    def setpath(self):
        if self.name == 'psu1':
            self.present_p = psu1_p
            self.direction_p = psu1_d
        if self.name == 'psu2':
            self.present_p = psu2_p
            self.direction_p = psu2_d

        return

    def updatepresent(self):
        self.present = int(system_read_filestr(self.present_p))

        return

    def updatedirection(self):
        if self.present == 1:
            self.direction = system_getpsu_direction(self.direction_p)
        else:
            self.direction = 2

        return

    def checkstatus(self, totaldirct):
        if self.present != 1:
            self.status = 1
            misclogger.error(':{} not present.'.format(self.name))
        elif self.direction == 2:
            self.status = 0
            misclogger.info(':{} direction need to be update.'.format(self.name))
        elif self.direction != totaldirct:
            self.status = 1
            misclogger.info(':{} direction is wrong.'.format(self.name))
        else:
            self.status = 0

        if self.status == 1:
            system_bright_leds(cs6436_ledpath[self.name], 3)
        else:
            system_bright_leds(cs6436_ledpath[self.name], 1)

        return self.status

cs6436_psuattrnodes = []



def my_log(txt):
    if DEBUG == True:
        print "[ROY]"+txt
    return

def device_exist():
    ret1, log = log_os_system("ls "+i2c_prefix+"5-005a", 0)
    ret2, log = log_os_system("ls "+i2c_prefix+"5-005b", 0)
    ret3, log = log_os_system("ls "+leds_prefix+"cs6436_56p_led*", 0)
    return not(ret1 or ret2 or ret3)


def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status, output = commands.getstatusoutput(cmd)
    my_log (cmd +"with result:" + str(status))
    my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print('Failed :'+cmd)
    return status, output


def system_get_coretemp():
    temp1 = system_read_filestr(coretemp_ps[0]).strip()
    temp2 = system_read_filestr(coretemp_ps[1]).strip()
    temp3 = system_read_filestr(coretemp_ps[2]).strip()

    return int(temp1), int(temp2), int(temp3)

def system_get_boardtemp():
    for i in range(0,16):
        temp1path = "/sys/bus/i2c/devices/5-004a/hwmon/hwmon%d/temp1_input" % i
        if os.access(temp1path, os.F_OK):
            break
    for i in range(0,16):
        temp2path = "/sys/bus/i2c/devices/5-004b/hwmon/hwmon%d/temp1_input" % i
        if os.access(temp2path, os.F_OK):
            break
    temp1 = system_read_filestr(temp1path).strip()
    temp2 = system_read_filestr(temp2path).strip()

    return int(temp1), int(temp2)


def system_get_lswtemp():
    global IsGetlswt
    global starttime
    if IsGetlswt == 0:
        now = datetime.datetime.now()
        misclogger.info("time wait.")
        misclogger.info("start = {}, now = {}.".format(starttime, now))
        if (now - starttime).seconds > 150:
            misclogger.info("time = ".format((now - starttime).seconds))
            IsGetlswt = 1

        return 25

#    chp = subprocess.Popen("docker ps --filter name=syncd", shell=True, stdout=subprocess.PIPE)
#   if chp.poll() is None:
#       misclogger.info("No subp.")
#       chp.kill()
#
#        return 25

#    retstring = chp.stdout.read()
#    chp.kill()
#    if 'Up' not in retstring:
#        misclogger.info("lsw not up.")
#
#        return 25

    status, output = log_os_system('npx_diag swc show temperature', 1)
    if status:
        misclogger.error('failed to show lsw temperature')

        return 25

    output = output.strip()
    if output.find("it 0, temperature ") > 0:
        startindex = output.find('temperature') + len('temperature') + 1
        endindex = output[startindex:].find(" ")
        endindex = startindex + endindex
        temp = output[startindex:endindex]
        b = temp.find('.')
        if b > 0:
           temp=temp[:b]
        temp = int(temp)
    else:
        misclogger.error("Failed to get temperature.")
        temp = 0

    return int(temp)

def system_monitor_temperature():

    ctemp1, ctemp2, ctemp3 = system_get_coretemp()
    btemp1, btemp2 = system_get_boardtemp()
    ltemp = system_get_lswtemp()
    fan_speed_str = system_cs6436_getfanexspeed()
    fan_speed = int(fan_speed_str)
    policy = 'stay'
    pos = 0
                  #speed       c1     c2     c3     b1     b2  lsw
    fan_policy_up = ([30,  40000, 40000, 40000, 42000, 35000, 95],
                     [40,  44000, 44000, 44000, 44000, 39000, 96],
                     [50,  49000, 49000, 49000, 47000, 44000, 91],
                     [60,  52000, 52000, 52000, 51500, 47500, 92],
                     [70,  53000, 53000, 53000, 52000, 49000, 93],
                     [100,999999,999999,999999,999999,999999,999])

    fan_policy_down=([30,      0,     0,     0,     0,     0,  0],
                     [40,  34000, 34000, 34000, 34000, 30000, 80],
                     [50,  38000, 38000, 38000, 37000, 33000, 81],
                     [60,  44000, 44000, 44000, 43000, 39000, 84],
                     [70,  44000, 44000, 44000, 43000, 40000, 84],
                     [100, 48000, 48000, 48000, 46000, 42000, 85],)

    for policytable in fan_policy_up:
        if fan_speed <= policytable[0]:
            break
        pos = pos + 1
    fan_speed = policytable[0]
    if (ctemp1 < policytable[1]) and (ctemp2 < policytable[2]) and (ctemp3 < policytable[3]) and (btemp1 < policytable[4]) and (btemp2 < policytable[5]) and (ltemp < policytable[6]):
        policy = 'stay'
        policytable = fan_policy_down[pos]
        if (ctemp1 < policytable[1]) and (ctemp2 < policytable[2]) and (ctemp3 < policytable[3]) and (btemp1 < policytable[4]) and (btemp2 < policytable[5]) and (ltemp < policytable[6]):
            policy = 'down'
    else:
        policy = 'up'

    if policy == 'up':
        misclogger.info("speed = %d." % fan_speed)
        misclogger.info("core1 = %d, core2 = %d, core3 = %d." % (ctemp1, ctemp2, ctemp3))
        misclogger.info("board1 = %d, board2 = %d." % (btemp1, btemp2))
        misclogger.info("lsw = %d" % ltemp)
        fan_speed = fan_policy_down[pos + 1][0]
        misclogger.info("fan policy: up. speedexp = {}".format(fan_speed))

    if policy == 'stay':
        fan_speed = fan_policy_down[pos]
        return

    if policy == 'down':
        misclogger.info("speed = %d." % fan_speed)
        misclogger.info("core1 = %d, core2 = %d, core3 = %d." % (ctemp1, ctemp2, ctemp3))
        misclogger.info("board1 = %d, board2 = %d." % (btemp1, btemp2))
        misclogger.info("lsw = %d" % ltemp)
        fan_speed = fan_policy_down[pos - 1][0]
        misclogger.info("fan policy: down.speedexp = {}".format(fan_speed))

    cmd = "echo %d > /sys/devices/platform/cs6436_56p_fan/fan_duty_cycle_percentage" % fan_speed
    status, output = log_os_system(cmd, 1)
    if status:
        misclogger.error("set fan speed fault")

    return


def system_cs6436_setfanexspeed(num):
    fanspeednode = fans_prefix + 'fan_duty_cycle_percentage'
    numstr = str(num)
    with open(fanspeednode, 'w') as f:
        f.write(numstr)


def system_cs6436_getfanexspeed():
    fanspeednode = fans_prefix + 'fan_duty_cycle_percentage'
    fanspeedstr = system_read_filestr(fanspeednode)
    fanspeedexp = int(fanspeedstr)

    return fanspeedexp


def system_cs6436_getdirection():
    global cs6436_fanattrnodes
    direction = 0

    for fan in cs6436_fanattrnodes:
        direction = direction + fan.direction

    if direction > 2:
        direction = 1
    else:
        direction = 0

    return direction


def system_check_psusdirection():
    global cs6436_psuattrnodes
    cs6436totaldirct = system_cs6436_getdirection()
    psutatus = 0

    for psu in cs6436_psuattrnodes:
        psu.updatedirection()
        psu.checkstatus(cs6436totaldirct)
        psutatus = psu.status + psutatus

    return (psutatus != 0)


def system_check_psuspresent():
    global cs6436_psuattrnodes
    cs6436totaldirct = system_cs6436_getdirection()
    psutatus = 0

    for psu in cs6436_psuattrnodes:
        psu.updatepresent()
        psu.checkstatus(cs6436totaldirct)
        psutatus = psu.status + psutatus

    return (psutatus != 0)


def system_check_fansstate():
    global cs6436_fanattrnodes
    global cs6436_ledpath
    cs6436totaldirct = system_cs6436_getdirection()
    fanstatus = 0
    fanexspeed = 0

    fanexspeed = system_cs6436_getfanexspeed()

    for fan in cs6436_fanattrnodes:
        fan.updatedevice()
        fan.checkstatus(fanexspeed, cs6436totaldirct)
        fanstatus = fanstatus + fan.status

    if fanstatus > 0:
        misclogger.error(':fan error.set fans speed 100.')
        system_cs6436_setfanexspeed(100)
        system_bright_leds(cs6436_ledpath['fan'], 3)
    else:
        system_bright_leds(cs6436_ledpath['fan'], 1)

    return (fanstatus != 0)


def system_misc_polling(threadName,delay):
    for count in range(1,5):
        if device_exist() == False:
            time.sleep(delay+3)
            print "%s: %s, count=%d" % ( threadName, time.ctime(time.time()), count)
        else:
            break

    if count == 4:
        return

    status, output = log_os_system("echo 1 > /sys/devices/platform/cs6436_56p_led/leds/cs6436_56p_led::sys/brightness", 1)
    status, output = log_os_system("hwconfig -cfp 1", 1)

    global AGFlag
    if os.access(ageing_controlfile, os.F_OK):
        AGFlag = 1
    else:
        AGFlag = 0

    os.system('csw_daemon &')


    global cs6436_fanattrnodes
    global cs6436_psuattrnodes

    for num in range(1,6):
        name = 'fan{}'.format(num)
        fannode = cs6436_fanattr(name)
        cs6436_fanattrnodes.append(fannode)
    for num in range(1,3):
        name = 'psu{}'.format(num)
        psunode = cs6436_psuattr(name)
        cs6436_psuattrnodes.append(psunode)

    tempcontrol = system_init_coretemppath()

    misclogger.info("%s: %s misc start." % ( threadName, time.ctime(time.time())))
    count = 0
    while 1:
        count = count + 1
        ret = system_check_psuspresent()
        ret = system_check_fansstate()

        if count % 10 == 0:
            misclogger.info(": adjust fans and check psu direction.")
            system_check_psusdirection()
            if tempcontrol == 0:
                system_monitor_temperature()
            count = 0
        time.sleep(delay)

    return

if __name__ == '__main__':
    target=system_misc_polling("Thread-misc",10)

