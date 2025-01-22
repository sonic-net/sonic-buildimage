#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import click
import os
import time
import syslog
import subprocess
from ruijieconfig import MONITOR_CONST, FANCTROLDEBUG, MONITOR_FANS_LED, DEV_LEDS, MONITOR_PSU_STATUS, \
        MONITOR_SYS_PSU_LED, MONITOR_DEV_STATUS, MONITOR_FAN_STATUS, MONITOR_DEV_STATUS_DECODE, \
        MONITOR_SYS_FAN_LED, MONITOR_SYS_LED, fanloc

from ruijieutil import rji2cget, getMacTemp_sysfs, write_sysfs_value, get_sysfs_value, strtoint, \
        rji2cset

import traceback
import glob


CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

DEBUG_COMMON     = 0x01
DEBUG_LEDCONTROL = 0x02
DEBUG_FANCONTROL = 0x04


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
    
def fanwarninglog(s):
    #s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("FANCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING,s)
    
def fancriticallog(s):
    #s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("FANCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT,s)
    
def fanerror(s):
    #s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("FANCONTROL",syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR,s)
    
def fanwarningdebuglog(debuglevel,s):
    #s = s.decode('utf-8').encode('gb2312')
    if FANCTROLDEBUG & debuglevel:
        syslog.openlog("FANCONTROL",syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG,s)
 

class FanControl(object):
    critnum = 0
    def __init__(self):
        self._fanOKNum = 0
        self._psuOKNum = 0
        self._intemp = -100.0
        self._mac_aver = -100.0
        self._mac_max = -100.0
        self._preIntemp = -1000 # previous temperature
        self._outtemp = -100
        self._boardtemp = -100
        self._cputemp = -1000

    @property
    def fanOKNum(self):
        return self._fanOKNum;

    @property
    def psuOKNum(self):
        return self._psuOKNum;

    @property
    def cputemp(self):
        return self._cputemp;
 
    @property
    def intemp(self):
        return self._intemp;
        
    @property
    def outtemp(self):
        return self._outtemp;
        
    @property
    def boardtemp(self):
        return self._boardtemp;

    @property
    def mac_aver(self):
        return self._mac_aver;

    @property
    def preIntemp(self):
        return self._preIntemp;

    @property
    def mac_max(self):
        return self._mac_max;

    def sortCallback(self, element):
        return element['id']

    def gettemp(self,ret):
        u'''get inlet, outlet, hot-point and cpu temperature'''
        temp_conf = MONITOR_DEV_STATUS.get('temperature', None)

        if temp_conf is None:
            fanerror("gettemp: config error")
            return False
        for item_temp in temp_conf:
            try:
                retval = ""
                rval = None
                name = item_temp.get('name')
                location = item_temp.get('location')
                if name == "cpu":
                    L=[]
                    for dirpath, dirnames, filenames in os.walk(location):
                        for file in filenames :
                            if file.endswith("input"):
                                L.append(os.path.join(dirpath, file))
                        L =sorted(L,reverse=False)
                    for i in range(len(L)):
                        nameloc = "%s/temp%d_label"%(location,i+1)
                        valloc  = "%s/temp%d_input"%(location,i+1)
                        with open(nameloc, 'r') as fd1:
                            retval2 = fd1.read()
                        with open(valloc, 'r') as fd2:
                            retval3 = fd2.read()
                        ret_t ={}
                        ret_t["name"] = retval2.strip()
                        ret_t["value"] = float(retval3)/1000
                        ret.append(ret_t)
                        fanwarningdebuglog(DEBUG_COMMON,"gettemp %s : %f" % (ret_t["name"],ret_t["value"]))
                else:
                    locations = glob.glob(location)
                    with open(locations[0], 'r') as fd1:
                        retval = fd1.read()
                    rval = float(retval)/1000
                    ret_t ={}
                    ret_t["name"] = name
                    ret_t["value"] = rval
                    ret.append(ret_t)
                    fanwarningdebuglog(DEBUG_COMMON,"gettemp %s : %f" % (ret_t["name"],ret_t["value"]))
            except Exception as e:
                fanerror("gettemp error:name:%s" % name)
                fanerror(str(e))
        return True

    def checkslot(self,ret):
        u'''get slot present status'''
        slots_conf = MONITOR_DEV_STATUS.get('slots', None)
        slotpresent = MONITOR_DEV_STATUS_DECODE.get('slotpresent',None)

        if slots_conf is None or slotpresent is None:
            return False
        for item_slot in slots_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_slot.get('name')
                ret_t["status"] = ""
                gettype = item_slot.get('gettype')
                presentbit = item_slot.get('presentbit')
                if gettype == "io":
                    io_addr = item_slot.get('io_addr')
                    val = io_rd(io_addr)
                    if val is not None:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_slot.get('name'), "lpc read failed"))
                else:
                    bus = item_slot.get('bus')
                    loc = item_slot.get('loc')
                    offset = item_slot.get('offset')
                    ind, val = rji2cget(bus, loc,offset)
                    if ind == True:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_slot.get('name'), "i2c read failed"))
                if totalerr < 0 :
                    ret_t["status"] = "NOT OK"
                    ret.append(ret_t)
                    continue
                val_t = (int(retval,16) & (1<< presentbit)) >> presentbit
                fanwarningdebuglog(DEBUG_COMMON,"%s present:%s" % (item_slot.get('name'),slotpresent.get(val_t)))
                if val_t != slotpresent.get('okval'):
                    ret_t["status"] = "ABSENT"
                else:
                    ret_t["status"] = "PRESENT"
            except Exception as e:
                ret_t["status"] = "NOT OK"
                totalerr -= 1
                fanerror("checkslot error")
                fanerror(str(e))
            ret.append(ret_t)
        return True

    def checkpsu(self,ret):
        u'''get  psu status present, output and warning'''
        psus_conf = MONITOR_DEV_STATUS.get('psus', None)
        psupresent = MONITOR_DEV_STATUS_DECODE.get('psupresent',None)
        psuoutput = MONITOR_DEV_STATUS_DECODE.get('psuoutput',None)
        psualert = MONITOR_DEV_STATUS_DECODE.get('psualert',None)

        if psus_conf is None or psupresent is None or psuoutput is None:
            fanerror("checkpsu: config error")
            return False
        for item_psu in psus_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_psu.get('name')
                ret_t["status"] = ""
                gettype = item_psu.get('gettype')
                presentbit = item_psu.get('presentbit')
                statusbit = item_psu.get('statusbit')
                alertbit = item_psu.get('alertbit')
                if gettype == "io":
                    io_addr = item_psu.get('io_addr')
                    val = io_rd(io_addr)
                    if val is not None:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_psu.get('name'), "lpc read failed"))
                else:
                    bus = item_psu.get('bus')
                    loc = item_psu.get('loc')
                    offset = item_psu.get('offset')
                    ind, val = rji2cget(bus, loc,offset)
                    if ind == True:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_psu.get('name'), "i2c read failed"))
                if totalerr < 0 :
                    ret_t["status"] = "NOT OK"
                    ret.append(ret_t)
                    continue
                val_t = (int(retval,16) & (1<< presentbit)) >> presentbit
                val_status = (int(retval,16) & (1<< statusbit)) >> statusbit
                val_alert = (int(retval,16) & (1<< alertbit)) >> alertbit
                fanwarningdebuglog(DEBUG_COMMON,"%s present:%s output:%s alert:%s" % (item_psu.get('name'),psupresent.get(val_t),psuoutput.get(val_status),psualert.get(val_alert)))
                if val_t != psupresent.get('okval') or val_status != psuoutput.get('okval') or val_alert != psualert.get('okval'):
                    totalerr -=1
            except Exception as e:
                totalerr -= 1
                fanerror("checkpsu error")
                fanerror(str(e))
            if totalerr < 0:
                ret_t["status"] = "NOT OK"
            else:
                ret_t["status"] = "OK"
            ret.append(ret_t)
        return True

    def checkfan(self,ret):
        u'''get fan status present and roll'''
        fans_conf = MONITOR_DEV_STATUS.get('fans', None)
        fanpresent = MONITOR_DEV_STATUS_DECODE.get('fanpresent',None)
        fanroll = MONITOR_DEV_STATUS_DECODE.get('fanroll',None)

        if fans_conf is None or fanpresent is None or fanroll is None:
            fanerror("checkfan: config error")
            return False
        for item_fan in fans_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_fan.get('name')
                ret_t["status"] = ""
                presentstatus = item_fan.get('presentstatus')
                presentbus = presentstatus.get('bus')
                presentloc = presentstatus.get('loc')
                presentaddr = presentstatus.get('offset')
                presentbit = presentstatus.get('bit')
                ind, val = rji2cget(presentbus, presentloc,presentaddr)
                if ind == True:
                    val_t = (int(val,16) & (1<< presentbit)) >> presentbit
                    fanwarningdebuglog(DEBUG_COMMON,"checkfan:%s present status:%s" % (item_fan.get('name'),fanpresent.get(val_t)))
                    if val_t != fanpresent.get('okval'):
                        ret_t["status"] = "ABSENT"
                        ret.append(ret_t)
                        continue
                else:
                    fanerror("checkfan: %s get present status error." % item_fan.get('name'))
                motors = item_fan.get("rollstatus")
                for motor in motors:
                    statusbus = motor.get('bus', None)
                    statusloc = motor.get('loc', None)
                    statusaddr = motor.get('offset', None)
                    statusbit = motor.get('bit', None)
                    ind, val = rji2cget(statusbus, statusloc, statusaddr)
                    if ind == True:
                        val_t = (int(val,16) & (1<< statusbit)) >> statusbit
                        fanwarningdebuglog(DEBUG_COMMON,"checkfan:%s roll status:%s" % (motor.get('name'),fanroll.get(val_t)))
                        if val_t != fanroll.get('okval'):
                            totalerr -= 1
                    else:
                        totalerr -= 1
                        fanerror("checkfan: %s " % item_fan.get('name'))
                        fanerror("get %s status error." % motor["name"])
            except Exception as e:
                totalerr -= 1
                fanerror("checkfan error")
                fanerror(str(e))
            if totalerr < 0:
                ret_t["status"] = "NOT OK"
            else:
                ret_t["status"] = "OK"
            ret.append(ret_t)
        return True

    def getCurrentSpeed(self):
        try:
            loc = fanloc[0].get("location","")
            sped = get_sysfs_value(loc)
            value = strtoint(sped)
            return value
        except Exception as e:
            fanerror("%%policy: get current speedlevel error")
            fanerror(str(e))
            return None

    # guarantee the speed is lowest when speed lower than lowest value after speed-adjustment
    def checkCurrentSpeedSet(self):
        fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy: guarantee the lowest speed after speed-adjustment")
        value =  self.getCurrentSpeed()
        if value is None or value == 0:
            raise Exception("%%policy: getCurrentSpeed None")
        elif value < MONITOR_CONST.MIN_SPEED:
            self.fanSpeedSet(MONITOR_CONST.MIN_SPEED)
        
    
    def fanSpeedSet(self, level):        
        if level >= MONITOR_CONST.MAX_SPEED:
            level = MONITOR_CONST.MAX_SPEED
        for item in fanloc:
            try:
                loc = item.get("location","")
                write_sysfs_value(loc, "0x%02x"% level )
            except Exception as e:
                fanerror(str(e))
                fanerror("%%policy: config fan runlevel error")
        self.checkCurrentSpeedSet() # guaranteed minimum
    
    def fanSpeedSetMax(self):
        try:
            self.fanSpeedSet(MONITOR_CONST.MAX_SPEED)
        except Exception as e:
            fanerror("%%policy:fanSpeedSetMax failed")
            fanerror(str(e))
    
    def fanStatusCheck(self): # fan status check , max speed if fan error
        if self.fanOKNum < MONITOR_CONST.FAN_TOTAL_NUM:
            fanwarninglog("%%DEV_MONITOR-FAN: Normal fan number: %d" % (self.fanOKNum))
            self.fanSpeedSetMax()
            return False
        return True

    def setFanAttr(self,val):
        u'''set status of each fan'''
        for item in val:
            fanid = item.get("id")
            fanattr = fanid + "status"
            fanstatus = item.get("status")
            setattr(FanControl,fanattr,fanstatus)
            fanwarningdebuglog(DEBUG_COMMON,"fanattr:%s,fanstatus:%s"% (fanattr,fanstatus))

    def getFanPresentNum(self,curFanStatus):
        fanoknum = 0;
        for item in curFanStatus:
            if item["status"] == "OK":
                fanoknum += 1
        self._fanOKNum = fanoknum
        fanwarningdebuglog(DEBUG_COMMON,"fanOKNum = %d"% self._fanOKNum)

    def getFanStatus(self):
        try:
            curFanStatus = []
            ret = self.checkfan(curFanStatus)
            if ret == True:
                self.setFanAttr(curFanStatus) 
                self.getFanPresentNum(curFanStatus)
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getFanStatus success" )
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getPsuOkNum(self,curPsuStatus):
        psuoknum = 0;
        for item in curPsuStatus:
            if item.get("status") == "OK":
                psuoknum += 1
        self._psuOKNum = psuoknum
        fanwarningdebuglog(DEBUG_COMMON,"psuOKNum = %d"% self._psuOKNum)

    def getPsuStatus(self):
        try:
            curPsuStatus = []
            ret = self.checkpsu(curPsuStatus)
            if ret == True:
                self.getPsuOkNum(curPsuStatus)
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getPsuStatus success" )
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getMonitorTemp(self, temp):
        for item in temp:
            if item.get('name') == "lm75in":
                self._intemp = item.get('value',self._intemp)
            if item.get('name') == "lm75out":
                self._outtemp = item.get('value',self._outtemp)
            if item.get('name') == "lm75hot":
                self._boardtemp = item.get('value',self._boardtemp)
            if item.get('name') == "Physical id 0":
                self._cputemp = item.get('value',self._cputemp)
        fanwarningdebuglog(DEBUG_COMMON,"intemp:%f, outtemp:%f, boadrtemp:%f, cputemp:%f"% (self._intemp,self._outtemp,self._boardtemp,self._cputemp))

    def getTempStatus(self):
        try:
            monitortemp =[]
            ret = self.gettemp(monitortemp)
            if ret == True:
                self.getMonitorTemp(monitortemp)
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getTempStatus success" )
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getMacStatus_bcmcmd(self):
        try:
            if waitForDocker(timeout = 0) == True :
                sta, ret = getMacTemp()
                if sta == True:
                    self._mac_aver = float(ret.get("average",self._mac_aver))
                    self._mac_max  = float(ret.get("maximum",self._mac_max))
                    fanwarningdebuglog(DEBUG_COMMON,"mac_aver:%f, mac_max:%f" % (self.mac_aver,self._mac_max))
                else:
                    fanwarningdebuglog(DEBUG_COMMON,"%%policy:getMacStatus_bcmcmd failed" )
            else:
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getMacStatus_bcmcmd SDK not OK" )
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def getMacStatus_sysfs(self,conf):
        try:
            sta, ret = getMacTemp_sysfs(conf)
            if sta == True:
                self._mac_aver = float(ret) / 1000
                self._mac_max = float(ret) / 1000
                fanwarningdebuglog(DEBUG_COMMON,"mac_aver:%f, mac_max:%f" % (self.mac_aver,self._mac_max))
            elif conf.get("try_bcmcmd", 0) == 1:
                fanwarningdebuglog(DEBUG_COMMON,"get sysfs mac temp failed.try to use bcmcmd")
                self.getMacStatus_bcmcmd()
            else:
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getMacStatus_sysfs failed" )
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def getMacStatus(self):
        try:
            mactempconf = MONITOR_DEV_STATUS.get('mac_temp', None)
            if mactempconf is not None:
                self.getMacStatus_sysfs(mactempconf)
            else:
                self.getMacStatus_bcmcmd()
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def settSlotAttr(self,val):
        u'''set each slot present status attribute'''
        for item in val:
            slotid = item.get("id")
            slotattr = slotid + "status"
            slotstatus = item.get("status")
            setattr(FanControl,slotattr,slotstatus)
            fanwarningdebuglog(DEBUG_COMMON,"slotattr:%s,slotstatus:%s"% (slotattr,slotstatus))

    def getSlotStatus(self):
        try:
            curSlotStatus = []
            ret = self.checkslot(curSlotStatus)
            if ret == True:
                self.settSlotAttr(curSlotStatus)
                fanwarningdebuglog(DEBUG_COMMON,"%%policy:getSlotStatus success" )
        except AttributeError as e:
            fanerror(str(e))
        return 0

    def fanctrol(self): #fan speed-adjustment 
        try:
            if self.preIntemp <= -1000:
                self.preIntemp = self.intemp
            fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:previous temperature[%.2f] , current temperature[%.2f]" % (self.preIntemp,self.intemp))
            if self.intemp < MONITOR_CONST.TEMP_MIN:
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:inlet  %.2f  minimum temperature: %.2f" %(self.intemp,MONITOR_CONST.TEMP_MIN))
                self.fanSpeedSet(MONITOR_CONST.DEFAULT_SPEED) # default level
            elif self.intemp >=  MONITOR_CONST.TEMP_MIN and self.intemp > self.preIntemp:
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:increase temperature")
                self.policySpeed(self.intemp)
            elif self.intemp >=  MONITOR_CONST.TEMP_MIN and (self.preIntemp - self.intemp) > MONITOR_CONST.MONITOR_FALL_TEMP:
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:temperature reduce over %d degree" % MONITOR_CONST.MONITOR_FALL_TEMP)
                self.policySpeed(self.intemp)
            else:
                speed = self.getCurrentSpeed()# set according to current speed, prevent fan watch-dog
                if speed is not None:
                    self.fanSpeedSet(speed)
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:change nothing")
        except Exception as e:
            fanerror("%%policy: fancontrol error")

    # start speed-adjustment 
    def startFanCtrol(self):
        self.checkCrit()
        if self.critnum == 0 and self.checkWarning() == False and self.fanStatusCheck() ==True:
            self.fanctrol()
            self.checkDevError()
        fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy: speed after speed-adjustment is %0x" % (self.getCurrentSpeed()))

    def policySpeed(self, temp): # fan speed-adjustment algorithm
        fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:fan speed-adjustment algorithm")
        sped_level = MONITOR_CONST.DEFAULT_SPEED + MONITOR_CONST.K * (temp - MONITOR_CONST.TEMP_MIN)
        self.fanSpeedSet(sped_level)
        self.preIntemp = self.intemp

    def getBoardMonitorMsg(self,ledcontrol = False):
        ret_t = 0
        try:
            ret_t += self.getFanStatus()  # get fan status, get number of fan which status is OK
            ret_t += self.getTempStatus() # get inlet, outlet, hot-point temperature, CPU temperature
            ret_t += self.getMacStatus()  # get MAC highest and average temperature
            if ledcontrol == True:
                ret_t += self.getSlotStatus() # get slot present status
                ret_t += self.getPsuStatus()  # get psu status
            if ret_t == 0:
                return True
        except Exception as e:
            fanerror(str(e))
        return False
    
    # device error algorithm    Tmac-Tin≥50℃, or Tmac-Tin≤-50℃
    def checkDevError(self):
        try:
            if (self.mac_aver - self.intemp) >= MONITOR_CONST.MAC_UP_TEMP or (self.mac_aver - self.intemp) <= MONITOR_CONST.MAC_LOWER_TEMP:
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%DEV_MONITOR-TEMP: MAC temp get failed.")
                value =  self.getCurrentSpeed()
                if MONITOR_CONST.MAC_ERROR_SPEED >= value:
                    self.fanSpeedSet(MONITOR_CONST.MAC_ERROR_SPEED)
                else:
                    self.fanSpeedSetMax()
            else:
                pass
        except Exception as e:
            fanerror("%%policy:checkDevError failed")
            fanerror(str(e))

    def checkTempWarning(self):
        u'''check whether temperature above the normal alarm value'''
        try:
            if self._mac_aver >= MONITOR_CONST.MAC_WARNING_THRESHOLD \
            or self._outtemp >= MONITOR_CONST.OUTTEMP_WARNING_THRESHOLD \
            or self._boardtemp >= MONITOR_CONST.BOARDTEMP_WARNING_THRESHOLD \
            or self._cputemp>=MONITOR_CONST.CPUTEMP_WARNING_THRESHOLD \
            or self._intemp >=MONITOR_CONST.INTEMP_WARNING_THRESHOLD:
                fanwarningdebuglog(DEBUG_COMMON,"check whether temperature above the normal alarm value")
                return True
        except Exception as e:
            fanerror("%%policy: checkTempWarning failed")
            fanerror(str(e))
        return False

    def checkTempCrit(self):
        u'''check whether temperature above the critical alarm value'''
        try:
            if self._mac_aver >= MONITOR_CONST.MAC_CRITICAL_THRESHOLD \
            or ( self._outtemp >= MONITOR_CONST.OUTTEMP_CRITICAL_THRESHOLD \
            and self._boardtemp >= MONITOR_CONST.BOARDTEMP_CRITICAL_THRESHOLD \
            and self._cputemp>= MONITOR_CONST.CPUTEMP_CRITICAL_THRESHOLD \
            and self._intemp >= MONITOR_CONST.INTEMP_CRITICAL_THRESHOLD):
                fanwarningdebuglog(DEBUG_COMMON,"temperature above the critical alarm value")
                return True
        except Exception as e:
            fanerror("%%policy: checkTempCrit failed")
            fanerror(str(e))
        return False

    def checkFanStatus(self):
        u'''check fan status'''
        for item in MONITOR_FAN_STATUS:
            maxoknum = item.get('maxOkNum')
            minoknum = item.get('minOkNum')
            status = item.get('status')
            if self.fanOKNum >= minoknum and self.fanOKNum <= maxoknum :
                fanwarningdebuglog(DEBUG_COMMON,"checkFanStatus:fanOKNum:%d,status:%s" % (self.fanOKNum,status))
                return status
        fanwarningdebuglog(DEBUG_COMMON,"checkFanStatus Error:fanOKNum:%d" % (self.fanOKNum))
        return None

    def checkPsuStatus(self):
        u'''check psu status'''
        for item in MONITOR_PSU_STATUS:
            maxoknum = item.get('maxOkNum')
            minoknum = item.get('minOkNum')
            status = item.get('status')
            if self.psuOKNum >= minoknum and self.psuOKNum <= maxoknum :
                fanwarningdebuglog(DEBUG_COMMON,"checkPsuStatus:psuOKNum:%d,status:%s" % (self.psuOKNum,status))
                return status
        fanwarningdebuglog(DEBUG_COMMON,"checkPsuStatus Error:psuOKNum:%d" % (self.psuOKNum))
        return None

    def dealSysLedStatus(self):
        u'''set up SYSLED according to temperature, fan and psu status'''
        try:
            fanstatus = self.checkFanStatus()
            psustatus = self.checkPsuStatus()
            if self.checkTempCrit() == True or fanstatus == "red" or psustatus == "red":
                status = "red"
            elif self.checkTempWarning() == True or fanstatus == "yellow" or psustatus == "yellow":
                status = "yellow"
            else:
                status = "green"
            self.setSysLed(status)
            fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:dealSysLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror(str(e))

    def dealSysFanLedStatus(self):
        u'''light panel fan led according to status'''
        try:
            status = self.checkFanStatus()
            if status is not None:
                self.setSysFanLed(status)
                fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:dealSysFanLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror("%%ledcontrol:dealSysLedStatus error")
            fanerror(str(e))

    def dealPsuLedStatus(self):
        u'''set up PSU-LED according to psu status'''
        try:
            status = self.checkPsuStatus()
            if status is not None:
                self.setSysPsuLed(status)
            fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:dealPsuLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror("%%ledcontrol:dealPsuLedStatus error")
            fanerror(str(e))

    def dealLocFanLedStatus(self):
        u'''light fan led according to fan status'''
        for item in MONITOR_FANS_LED:
            try:
                index = MONITOR_FANS_LED.index(item) + 1
                fanattr = "fan%dstatus" % index
                val_t = getattr(FanControl,fanattr,None)
                if val_t == "NOT OK":
                    rji2cset(item["bus"],item["devno"],item["addr"], item["red"])
                elif val_t == "OK":
                    rji2cset(item["bus"],item["devno"],item["addr"], item["green"])
                else:
                    pass
                fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:dealLocFanLed success.fanattr:%s, status:%s"% (fanattr,val_t))
            except Exception as e:
                fanerror("%%ledcontrol:dealLocFanLedStatus error")
                fanerror(str(e))

    def dealSlotLedStatus(self):
        u'''light slot status led according to slot present status'''
        slotLedList = DEV_LEDS.get("SLOTLED",[])
        for item in slotLedList:
            try:
                index = slotLedList.index(item) + 1
                slotattr = "slot%dstatus" % index
                val_t = getattr(FanControl,slotattr,None)
                if val_t == "PRESENT":
                    rji2cset(item["bus"],item["devno"],item["addr"], item["green"])
                fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:dealSlotLedStatus success.slotattr:%s, status:%s"% (slotattr,val_t))
            except Exception as e:
                fanerror("%%ledcontrol:dealSlotLedStatus error")
                fanerror(str(e))

    def dealBmcLedstatus(self,val):
        pass

    def dealLctLedstatus(self,val):
        pass

    def setled(self, item, color):
        if item.get('type', 'i2c') == 'sysfs':
            rjsysset(item["cmdstr"], item.get(color))
        else :
            mask = item.get('mask', 0xff)
            ind, val = rji2cget(item["bus"], item["devno"], item["addr"])
            if ind == True:
                setval = (int(val,16) & ~mask) | item.get(color)
                rji2cset(item["bus"], item["devno"], item["addr"], setval)
            else:
                fanerror("led %s" % "i2c read failed")

    def setSysLed(self,color):
        for item in MONITOR_SYS_LED:
            self.setled(item, color)

    def setSysFanLed(self,color):
        for item in MONITOR_SYS_FAN_LED:
            self.setled(item, color)

    def setSysPsuLed(self,color):
        for item in MONITOR_SYS_PSU_LED:
            self.setled(item, color)

    def checkWarning(self):
        try:
            if self.checkTempWarning() == True:
                fanwarningdebuglog(DEBUG_FANCONTROL,"anti-shake start")
                time.sleep(MONITOR_CONST.SHAKE_TIME)
                fanwarningdebuglog(DEBUG_FANCONTROL,"anti-shake end")
                self.getBoardMonitorMsg()# re-read
                if self.checkTempWarning() == True:
                    fanwarninglog("%%DEV_MONITOR-TEMP:The temperature of device is over warning value.")
                    self.fanSpeedSetMax()  # fan full speed
                    return True
        except Exception as e:
            fanerror("%%policy: checkWarning failed")
            fanerror(str(e))
        return False

    def checkCrit(self):
        try:
            if self.checkTempCrit() == True:
                fanwarningdebuglog(DEBUG_FANCONTROL,"anti-shake start")
                time.sleep(MONITOR_CONST.SHAKE_TIME)
                fanwarningdebuglog(DEBUG_FANCONTROL,"anti-shake end")
                self.getBoardMonitorMsg()# re-read
                if self.checkTempCrit() == True:
                    fancriticallog("%%DEV_MONITOR-TEMP:The temperature of device is over critical value.")
                    self.fanSpeedSetMax()  # fan full speed
                    self.critnum += 1 # anti-shake
                    if self.critnum >= MONITOR_CONST.CRITICAL_NUM:
                        subprocess.call(["reboot"])
                    fanwarningdebuglog(DEBUG_FANCONTROL,"crit次数:%d" % self.critnum)
                else:
                    self.critnum = 0
            else:
                self.critnum = 0
        except Exception as e:
            fanerror("%%policy: checkCrit failed")
            fanerror(str(e))
            

def callback():
    pass

def doFanCtrol(fanCtrol):
    ret = fanCtrol.getBoardMonitorMsg()
    if ret==True:
        fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:startFanCtrol")
        fanCtrol.startFanCtrol()
    else:
        fanCtrol.fanSpeedSetMax()
        fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:getBoardMonitorMsg error")
    
def doLedCtrol(fanCtrol):
    fanCtrol.getBoardMonitorMsg(ledcontrol = True) # get status
    fanCtrol.dealSysLedStatus()        # light system led
    fanCtrol.dealSysFanLedStatus()     # light panel fan led
    fanCtrol.dealLocFanLedStatus()     # light fan led
    fanCtrol.dealPsuLedStatus()        # light psu led
    fanCtrol.dealSlotLedStatus()       # light slot status led
    fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:doLedCtrol success")

def run(interval, fanCtrol):
    loop = 0
    # waitForDocker()
    while True:
        try:
            if loop % MONITOR_CONST.MONITOR_INTERVAL ==0: # fan speed-adjustment
                fanwarningdebuglog(DEBUG_FANCONTROL,"%%policy:fanCtrol")
                doFanCtrol(fanCtrol)
            else:
                fanwarningdebuglog(DEBUG_LEDCONTROL,"%%ledcontrol:start ledctrol")# LED control
                doLedCtrol(fanCtrol)
            time.sleep(interval)
            loop += interval
        except Exception as e:
            traceback.print_exc()
            fanerror(str(e))

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass
    
@main.command()
def start():
    '''start fan control'''
    fanwarninglog("FANCTROL start")
    fanCtrol = FanControl()
    interval = MONITOR_CONST.MONITOR_INTERVAL /30
    run(interval, fanCtrol)

@main.command()
def stop():
    '''stop fan control '''
    fanwarninglog("stop")

##device_i2c operation
if __name__ == '__main__':
    main()
