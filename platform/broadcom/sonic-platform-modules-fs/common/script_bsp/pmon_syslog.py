#!/usr/bin/python3
#   * onboard interval check
#   * FAN trays
#   * PSU
#   * SFF
import os
import time
import syslog
import traceback
import glob
import logging
from platform_util import *
from platform_config import *
from time import monotonic as _time

PMON_SYSLOG_FILE ="/tmp/.pmon_syslog_factest_mode_en"
DEBUG_FILE = "/etc/.pmon_syslog_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "pmon_syslog_debug.log"
logger = setup_logger(LOG_FILE)

STATUS_OK_LIST = [1]
PSU_STATUS_OK_LIST = [1, 2]

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def pmon_debug(s):
    logger.debug(s)

def pmon_error(s):
    logger.error(s)

def dev_syslog(s):
    syslog.openlog("PMON_SYSLOG", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_NOTICE, s)
    logger.error(s)

# status
STATUS_PRESENT = 'PRESENT'
STATUS_ABSENT = 'ABSENT'
STATUS_OK = 'OK'
STATUS_NOT_OK = 'NOT OK'
STATUS_FAILED = 'FAILED'

TEMP_FIELD = {
    "name": "alias",
    "value":"value",
    "max": "max",
    "min": "min",
    "high": "high",
    "low": "low",
}


class checkBase(object):
    def __init__(self, path, dev_name, display_name, obj_type, config, source = GET_BY_COMMON, index = None):
        self._peroid_syslog = None
        self._peroid_failed_syslog = None  # exception
        self._preDevStatus = None
        self._path = path
        self._name = dev_name
        self._display_name = display_name
        self._type = obj_type
        self._config = config
        self._source = source
        self._index = index

    def getCurstatus(self):
        # get ok/not ok/absent status
        status, log = self.getPresent()
        if status == STATUS_PRESENT:
            # check status
            property_status, log = self.getStatus()
            if property_status is not None:
                status = property_status
        return status, log

    def getResetfulType(self):
        pass

    def getPresent(self):
        presentFilepath = self.getPath()
        try:
            if self._source == GET_BY_RESTFUL:
                restful_type = self.getResetfulType()
                present_cfg = {"field_name": "present", "type": restful_type, "index": self._index}
                ret, val = get_single_info_from_restful(present_cfg)
                if not ret:
                    return STATUS_FAILED, val
                if int(val) == DEV_PRESENT:
                    return STATUS_PRESENT, None
                else:
                    return STATUS_ABSENT, None
            else:
                # get ok/not ok/absent status
                presentConfig = self._config["present"]
                mask = presentConfig.get("mask", 0xff)
                absent_val = presentConfig.get("ABSENT", None)
                absent_val = absent_val & mask
                with open(presentFilepath, "r") as fd:
                    retval = fd.read()
                    if int(retval) == absent_val:
                        return STATUS_ABSENT, None
                    return STATUS_PRESENT, None
        except Exception as e:
            return STATUS_FAILED, (str(e) + " location[%s]" % presentFilepath)

    def getStatus(self):
        if self._source == GET_BY_RESTFUL:
            restful_type = self.getResetfulType()
            dev_type = self.getType()
            status_field_name = "status"
            status_ok_list = STATUS_OK_LIST
            if dev_type == "PSU":
                status_field_name = "hw_status"
                status_ok_list = PSU_STATUS_OK_LIST
            status_cfg = {"field_name": status_field_name, "type": restful_type, "index": self._index}
            ret, val = get_single_info_from_restful(status_cfg)
            if not ret:
                return STATUS_FAILED, val
            if int(val) in status_ok_list:
                return STATUS_OK, None
            else:
                return STATUS_NOT_OK, None
        else:
            if "status" in self._config:
                statusConfig = self._config["status"]
                for itemConfig in statusConfig:
                    mask = itemConfig.get("mask", 0xff)
                    ok_val = itemConfig.get("okval", None)
                    ok_val = ok_val & mask
                    Filepath = itemConfig["path"] % self._name
                    try:
                        with open(Filepath, "r") as fd1:
                            retval = fd1.read()
                            if int(retval) != ok_val:
                                return STATUS_NOT_OK, None
                    except Exception as e:
                        return STATUS_FAILED, (str(e) + " location[%s]" % Filepath)
                return STATUS_OK, None
        return None, None

    def getPath(self):
        return self._path

    def getName(self):
        return self._name

    def getType(self):
        return self._type

    def getDisplayName(self):
        return self._display_name

    def getnochangedMsgFlag(self):
        return self._config["nochangedmsgflag"]

    def getnochangedMsgTime(self):
        return self._config["nochangedmsgtime"]

    def getnoprintFirstTimeFlag(self):
        return self._config["noprintfirsttimeflag"]

    def checkStatus(self):
        # syslog msg
        dev_type = self.getType()
        display_name = self.getDisplayName()
        nochangedMsgTime = self.getnochangedMsgTime()
        getnochangedMsgFlag = self.getnochangedMsgFlag()
        noprintFirstTimeFlag = self.getnoprintFirstTimeFlag()
        MSG_IN = '%%PMON-5-' + dev_type + '_PLUG_IN: %s is PRESENT.'
        MSG_OUT = '%%PMON-5-' + dev_type + '_PLUG_OUT: %s is ABSENT.'
        MSG_OK = '%%PMON-5-' + dev_type + '_OK: %s is OK.'
        MSG_NOT_OK = '%%PMON-5-' + dev_type + '_FAILED: %s is NOT OK.'
        MSG_ABSENT = '%%PMON-5-' + dev_type + '_ABSENT: %s is ABSENT.'
        MSG_UNKNOWN = '%%PMON-5-' + dev_type + '_UNKNOWN: %s is UNKNOWN.%s'
        MSG_RECOVER = '%%PMON-5-' + dev_type + '_OK: %s is OK. Recover from ' + dev_type + ' FAILED.'

        curStatus, log = self.getCurstatus()
        pmon_debug("%s: current status %s" % (display_name, curStatus))
        pmon_debug("%s: pre status %s" % (display_name, self._preDevStatus))
        pmon_debug("%s: peroid_syslog %s" % (display_name, self._peroid_syslog))

        if curStatus == STATUS_FAILED:
            # get status failed
            if self._peroid_failed_syslog is not None:
                if getnochangedMsgFlag and _time() - self._peroid_failed_syslog >= nochangedMsgTime:
                    # absent as before for some time, notice
                    dev_syslog(MSG_UNKNOWN % (display_name, log))
                    self._peroid_failed_syslog = _time()
            else:  # first time failed
                dev_syslog(MSG_UNKNOWN % (display_name, log))
                self._peroid_failed_syslog = _time()
            return
        self._peroid_failed_syslog = _time()

        if self._preDevStatus is None:
            # 1st time
            if noprintFirstTimeFlag == 1:
                self._peroid_syslog = _time()
            else:
                if curStatus == STATUS_PRESENT:
                    # present
                    dev_syslog(MSG_IN % display_name)
                elif curStatus == STATUS_OK:
                    # ok
                    dev_syslog(MSG_OK % display_name)
                elif curStatus == STATUS_NOT_OK:
                    # not ok
                    dev_syslog(MSG_NOT_OK % display_name)
                    self._peroid_syslog = _time()
                else:
                    # absent
                    dev_syslog(MSG_ABSENT % display_name)
                    self._peroid_syslog = _time()
        else:
            # from 2nd time...
            if self._preDevStatus == curStatus:
                # status not changed
                if self._preDevStatus == STATUS_ABSENT:
                    if self._peroid_syslog is not None:
                        if getnochangedMsgFlag and _time() - self._peroid_syslog >= nochangedMsgTime:
                            # absent as before for some time, notice
                            dev_syslog(MSG_ABSENT % display_name)
                            self._peroid_syslog = _time()
                elif self._preDevStatus == STATUS_NOT_OK:
                    if self._peroid_syslog is not None:
                        if getnochangedMsgFlag and _time() - self._peroid_syslog >= nochangedMsgTime:
                            # not ok as before for some time, notice
                            dev_syslog(MSG_NOT_OK % display_name)
                            self._peroid_syslog = _time()
            else:
                # status changed
                if self._preDevStatus == STATUS_ABSENT:
                    if curStatus == STATUS_NOT_OK:
                        # absent -> not ok
                        dev_syslog(MSG_IN % display_name)
                        dev_syslog(MSG_NOT_OK % display_name)
                        self._peroid_syslog = _time()
                    elif curStatus == STATUS_OK:
                        # absent -> ok
                        dev_syslog(MSG_IN % display_name)
                        dev_syslog(MSG_OK % display_name)
                    else:
                        # absent -> prsent
                        dev_syslog(MSG_IN % display_name)

                elif self._preDevStatus == STATUS_OK:
                    if curStatus == STATUS_NOT_OK:
                        # ok -> not ok
                        dev_syslog(MSG_NOT_OK % display_name)
                        self._peroid_syslog = _time()
                    elif curStatus == STATUS_ABSENT:
                        # ok -> absent
                        dev_syslog(MSG_OUT % display_name)
                        self._peroid_syslog = _time()
                elif self._preDevStatus == STATUS_PRESENT:
                    # present -> absent
                    dev_syslog(MSG_OUT % display_name)
                    self._peroid_syslog = _time()
                else:  # not ok
                    if curStatus == STATUS_OK:
                        # not ok -> ok
                        dev_syslog(MSG_RECOVER % display_name)
                        dev_syslog(MSG_OK % display_name)
                    else:
                        # not ok -> absent
                        dev_syslog(MSG_OUT % display_name)
                        self._peroid_syslog = _time()
        self._preDevStatus = curStatus


class checkSfp(checkBase):
    def __init__(self, path, dev_name, display_name, config, source = GET_BY_COMMON, index = None):
        super(checkSfp, self).__init__(path, dev_name, display_name, 'XCVR', config, source, index)

    def getPath(self):
        super(checkSfp, self).getPath()
        return self._path

    def getName(self):
        super(checkSfp, self).getName()
        return self._name

    def getType(self):
        super(checkSfp, self).getType()
        return self._type


class checkSlot(checkBase):
    def __init__(self, path, dev_name, display_name, config, source = GET_BY_COMMON, index = None):
        super(checkSlot, self).__init__(path, dev_name, display_name, 'SLOT', config, source, index)

    def getPath(self):
        super(checkSlot, self).getPath()
        return self._path

    def getName(self):
        super(checkSlot, self).getName()
        return self._name

    def getType(self):
        super(checkSlot, self).getType()
        return self._type


class checkPSU(checkBase):
    def __init__(self, path, dev_name, display_name, config, source = GET_BY_COMMON, index = None):
        super(checkPSU, self).__init__(path, dev_name, display_name, 'PSU', config, source, index)

    def getPath(self):
        super(checkPSU, self).getPath()
        return self._path

    def getName(self):
        super(checkPSU, self).getName()
        return self._name

    def getType(self):
        super(checkPSU, self).getType()
        return self._type

    def getResetfulType(self):
        super(checkPSU, self).getResetfulType()
        return 'psu'


class checkFAN(checkBase):
    def __init__(self, path, dev_name, display_name, config, source = GET_BY_COMMON, index = None):
        super(checkFAN, self).__init__(path, dev_name, display_name, 'FAN', config, source, index)

    def getPath(self):
        super(checkFAN, self).getPath()
        return self._path

    def getName(self):
        super(checkFAN, self).getName()
        return self._name

    def getType(self):
        super(checkFAN, self).getType()
        return self._type

    def getResetfulType(self):
        super(checkFAN, self).getResetfulType()
        return 'fan'

class platformSyslog():
    def __init__(self):
        self.__sfp_checklist = []
        self.__fan_checklist = []
        self.__psu_checklist = []
        self.__slot_checklist = []
        self.__temp_checklist = []
        self.temps_peroid_syslog = {}
        self.normal_status = 0
        self.warning_status = 1
        self.critical_status = 2
        self.poweron_flag = 0
        self.__temp_source = None

        self.pmon_syslog_config = PMON_SYSLOG_STATUS.copy()
        self.__pollingtime = self.pmon_syslog_config.get('polling_time', 3)

        tmpconfig = self.pmon_syslog_config.get('sffs', None)
        if tmpconfig is not None:
            preset_item = tmpconfig.get("present", {})
            path = preset_item.get("path", [])
            for location in path:
                if '*' not in location:
                    pmon_error("sff location config error: %s" % location)
                    continue
                dev_name_index = 0
                loc_split_list = location.split('/')
                for i, item in enumerate(loc_split_list):
                    if '*' in item:
                        dev_name_index = i
                        break
                locations = glob.glob(location)
                for dev_path in locations:
                    dev_name_list = dev_path.split('/')
                    # explame:get eth1 from /sys_switch/transceiver/eth1/present
                    dev_name = dev_name_list[dev_name_index]
                    dev_name_alias = tmpconfig.get("alias", {})
                    display_name = dev_name_alias.get(dev_name, dev_name)
                    dev = checkSfp(dev_path, dev_name, display_name, tmpconfig)
                    self.__sfp_checklist.append(dev)

        tmpconfig = self.pmon_syslog_config.get('fans', None)
        if tmpconfig is not None:
            source = tmpconfig.get("source", GET_BY_COMMON)
            if source == GET_BY_RESTFUL:
                dev_num_cfg = {"type": "fan", "field_name": "num"}
                ret, dev_num = get_single_info_from_restful(dev_num_cfg)
                if not ret:
                    pmon_error("get fan num error, source: %s, reason: %s" % (source, dev_num))
                else:
                    for index in range(1, dev_num + 1):
                        dev_name = "fan%d" % index
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkFAN("", dev_name, display_name, tmpconfig, source, index)
                        self.__fan_checklist.append(dev)
            else:
                preset_item = tmpconfig.get("present", {})
                path = preset_item.get("path", [])
                for location in path:
                    if '*' not in location:
                        pmon_error("fan location config error: %s" % location)
                        continue
                    dev_name_index = 0
                    loc_split_list = location.split('/')
                    for i, item in enumerate(loc_split_list):
                        if '*' in item:
                            dev_name_index = i
                            break
                    locations = glob.glob(location)
                    for dev_path in locations:
                        dev_name_list = dev_path.split('/')
                        dev_name = dev_name_list[dev_name_index]
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkFAN(dev_path, dev_name, display_name, tmpconfig)
                        self.__fan_checklist.append(dev)

        tmpconfig = self.pmon_syslog_config.get('psus', None)
        if tmpconfig is not None:
            source = tmpconfig.get("source", GET_BY_COMMON)
            if source == GET_BY_RESTFUL:
                dev_num_cfg = {"type": "psu", "field_name": "num"}
                ret, dev_num = get_single_info_from_restful(dev_num_cfg)
                if not ret:
                    pmon_error("get psu num error, source: %s, reason: %s" % (source, dev_num))
                else:
                    for index in range(1, dev_num + 1):
                        dev_name = "psu%d" % index
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkPSU("", dev_name, display_name, tmpconfig, source, index)
                        self.__psu_checklist.append(dev)
            else:
                preset_item = tmpconfig.get("present", {})
                path = preset_item.get("path", [])
                for location in path:
                    if '*' not in location:
                        pmon_error("psu location config error: %s" % location)
                        continue
                    dev_name_index = 0
                    loc_split_list = location.split('/')
                    for i, item in enumerate(loc_split_list):
                        if '*' in item:
                            dev_name_index = i
                            break
                    locations = glob.glob(location)
                    for dev_path in locations:
                        dev_name_list = dev_path.split('/')
                        dev_name = dev_name_list[dev_name_index]
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkPSU(dev_path, dev_name, display_name, tmpconfig)
                        self.__psu_checklist.append(dev)

        tmpconfig = self.pmon_syslog_config.get('slots', None)
        if tmpconfig is not None:
            source = tmpconfig.get("source", GET_BY_COMMON)
            if source == GET_BY_RESTFUL:
                dev_num_cfg = {"type": "slot", "field_name": "num"}
                ret, dev_num = get_single_info_from_restful(dev_num_cfg)
                if not ret:
                    pmon_error("get slot num error, source: %s, reason: %s" % (source, dev_num))
                else:
                    for index in range(1, dev_num + 1):
                        dev_name = "slot%d" % index
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkSlot("", dev_name, display_name, tmpconfig, source, index)
                        self.__slot_checklist.append(dev)
            else:
                preset_item = tmpconfig.get("present", {})
                path = preset_item.get("path", [])
                for location in path:
                    if '*' not in location:
                        pmon_error("slot location config error: %s" % location)
                        continue
                    dev_name_index = 0
                    loc_split_list = location.split('/')
                    for i, item in enumerate(loc_split_list):
                        if '*' in item:
                            dev_name_index = i
                            break
                    locations = glob.glob(location)
                    for dev_path in locations:
                        dev_name_list = dev_path.split('/')
                        dev_name = dev_name_list[dev_name_index]
                        dev_name_alias = tmpconfig.get("alias", {})
                        display_name = dev_name_alias.get(dev_name, dev_name)
                        dev = checkSlot(dev_path, dev_name, display_name, tmpconfig)
                        self.__slot_checklist.append(dev)

        tmpconfig = self.pmon_syslog_config.get('temps', None)
        if tmpconfig is not None:
            self.__temp_source = tmpconfig.get("source", GET_BY_COMMON)
            self.__temp_checklist = self.generate_temp_config_by_restful()
            self.__temps_pollingseconds = tmpconfig.get('over_temps_polling_seconds', None)

    def checkTempStaus(self, temp_item):
        temp_name = temp_item.get('name', None)
        input_path = temp_item.get('input_path', None)
        warning_temp = temp_item.get('warning', None)
        critical_temp = temp_item.get('critical', None)
        input_accuracy = temp_item.get('input_accuracy', None)
        if temp_name is None or input_path is None or warning_temp is None or critical_temp is None:
            dev_syslog('%%PMON-5-TEMP_NOTICE: get temperature config parament failed.')
            return
        try:
            input_temp = None
            if isinstance(input_path, list):
                for item in input_path:
                    locations = glob.glob(item)
                    with open(locations[0], "r") as fd:
                        tmp = float(fd.read()) / float(input_accuracy)
                    if tmp is None:
                        continue
                    if input_temp is None or input_temp < tmp:
                        input_temp = tmp
            else:
                locations = glob.glob(input_path)
                with open(locations[0], "r") as fd:
                    input_temp = fd.read()
                input_temp = float(input_temp) / float(input_accuracy)

            if 'time' not in temp_item:
                temp_item['time'] = _time()
                temp_item['status'] = self.normal_status
            if float(input_temp) >= float(warning_temp):
                if float(input_temp) >= float(critical_temp):
                    if _time() - \
                            temp_item['time'] >= self.__temps_pollingseconds or temp_item['status'] != self.critical_status:
                        dev_syslog('%%PMON-5-TEMP_HIGH: %s temperature %sC is larger than max critical threshold %sC.'
                                   % (temp_name, input_temp, critical_temp))
                        temp_item['status'] = self.critical_status
                        temp_item['time'] = _time()
                else:
                    if _time() - \
                            temp_item['time'] >= self.__temps_pollingseconds or temp_item['status'] != self.warning_status:
                        dev_syslog('%%PMON-5-TEMP_HIGH: %s temperature %sC is larger than max warning threshold %sC.'
                                   % (temp_name, input_temp, warning_temp))
                        temp_item['status'] = self.warning_status
                        temp_item['time'] = _time()
            else:
                pmon_debug(
                    "%s temperature %sC is in range [%s, %s]" %
                    (temp_name, input_temp, warning_temp, critical_temp))
                temp_item['status'] = self.normal_status
                temp_item['time'] = _time()
        except Exception as e:
            dev_syslog('%%PMON-5-TEMP_NOTICE: Cannot get %s temperature. Exception log: %s' % (temp_name, str(e)))
        return

    def generate_temp_config_by_restful(self):
        temp_configs = []
        num_config = {"type": "temp", "field_name": "num"}

        ret, num = get_single_info_from_restful(num_config)
        if ret is False:
            pmon_error("get temp num fail, reason: %s" % num)
            return temp_configs

        for i in range(1, num + 1):
            info_config = {"type": "temp", "field": TEMP_FIELD, "index": i}
            temp_configs.append(info_config)

        return temp_configs

    def checkTempStausResetful(self, temp_item):
        try:
            ret, temp_info = get_multiple_info_from_restful(temp_item)
            if not ret:
                pmon_error("get temp info fail, reason: %s" % temp_info)
                return
            temp_name = temp_info.get('name', None)
            warning_temp = float(temp_info.get('high', None)) / 1000
            critical_temp = float(temp_info.get('max', None)) / 1000
            input_temp = float(temp_info.get('value', None)) / 1000

            if 'time' not in temp_item:
                temp_item['time'] = _time()
                temp_item['status'] = self.normal_status
            if float(input_temp) >= float(warning_temp):
                if float(input_temp) >= float(critical_temp):
                    if _time() - \
                            temp_item['time'] >= self.__temps_pollingseconds or temp_item['status'] != self.critical_status:
                        dev_syslog('%%PMON-5-TEMP_HIGH: %s temperature %sC is larger than max critical threshold %sC.'
                                % (temp_name, input_temp, critical_temp))
                        temp_item['status'] = self.critical_status
                        temp_item['time'] = _time()
                else:
                    if _time() - \
                            temp_item['time'] >= self.__temps_pollingseconds or temp_item['status'] != self.warning_status:
                        dev_syslog('%%PMON-5-TEMP_HIGH: %s temperature %sC is larger than max warning threshold %sC.'
                                % (temp_name, input_temp, warning_temp))
                        temp_item['status'] = self.warning_status
                        temp_item['time'] = _time()
            else:
                pmon_debug(
                    "%s temperature %sC is in range [%s, %s]" %
                    (temp_name, input_temp, warning_temp, critical_temp))
                temp_item['status'] = self.normal_status
                temp_item['time'] = _time()
        except Exception as e:
            dev_syslog('%%PMON-5-TEMP_NOTICE: Cannot get %s temperature. Exception log: %s' % (temp_name, str(e)))
        return

    def sysfs_precondition_check(self, check_module, check_project):
        try:
            tmpconfig = self.pmon_syslog_config.get(check_module, None)
            if tmpconfig is not None:
                check_list = tmpconfig.get(check_project, [])
                for check_item in check_list:
                    location = check_item.get("path", None)
                    ok_val = check_item.get("ok_val", None)
                    mask = check_item.get("mask", 0xff)
                    ok_val = ok_val & mask
                    locations = glob.glob(location)
                    for power_path in locations:
                        with open(power_path, "r") as fd:
                            retval = fd.read()
                        if int(retval) != ok_val:
                            return
                self.poweron_flag = 1
        except Exception as e:
            dev_syslog('%%PMON-5-TEMP_NOTICE: Cannot check power status. Exception log: %s' % str(e))
        return

    def updateSysDeviceStatus(self):
        if self.poweron_flag == 1:
            for dev in self.__sfp_checklist:
                dev.checkStatus()
        else:
            self.sysfs_precondition_check('sffs', 'power')

        for dev in self.__fan_checklist:
            dev.checkStatus()
        for dev in self.__psu_checklist:
            dev.checkStatus()
        for dev in self.__slot_checklist:
            dev.checkStatus()

        if self.__temp_source == GET_BY_RESTFUL:
            for temp_item in self.__temp_checklist:
                self.checkTempStausResetful(temp_item)
        else:
            for temp_item in self.__temp_checklist:
                self.checkTempStaus(temp_item)

    def getPollingtime(self):
        return self.__pollingtime

    def doWork(self):
        try:
            self.updateSysDeviceStatus()
        except Exception as e:
            MSG_EXCEPTION = '%%PMON-5-NOTICE: Exception happened! info:%s' % str(e)
            pmon_error(MSG_EXCEPTION % traceback.format_exc())


def run():
    debug_init()
    platform = platformSyslog()
    while True:
        debug_init()
        if os.path.exists(PMON_SYSLOG_FILE) is True:
            pmon_debug("file exists, do nothing!")
            time.sleep(5)
            continue
        platform.doWork()
        time.sleep(platform.getPollingtime())


if __name__ == '__main__':
    run()
