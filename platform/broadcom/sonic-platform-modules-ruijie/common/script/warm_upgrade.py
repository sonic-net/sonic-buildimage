#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import syslog
import signal
import click
from platform_util import get_value, set_value, exec_os_cmd, exec_os_cmd_log
from platform_config import WARM_UPGRADE_PARAM


WARM_UPGRADE_DEBUG_FILE = "/etc/.warm_upgrade_debug_flag"

WARMUPGRADEDEBUG = 1

debuglevel = 0

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


class AliasedGroup(click.Group):

    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None


def debug_init():
    global debuglevel
    if os.path.exists(WARM_UPGRADE_DEBUG_FILE):
        debuglevel = debuglevel | WARMUPGRADEDEBUG
    else:
        debuglevel = debuglevel & ~(WARMUPGRADEDEBUG)


def warmupgradewarninglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("WARMUPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def warmupgradecriticallog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("WARMUPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)


def warmupgradeerror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("WARMUPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def warmupgradedebuglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    if WARMUPGRADEDEBUG & debuglevel:
        syslog.openlog("WARMUPGRADE", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def subprocess_warm_upgrade(file, main_type, sub_type, slot):
    command = "firmware_upgrade %s 0x%x 0x%x %s" % (file, main_type, sub_type, slot)
    warmupgradedebuglog("warm upgrade firmware cmd:%s" % command)
    if os.path.exists(WARM_UPGRADE_DEBUG_FILE):
        return exec_os_cmd_log(command)
    return exec_os_cmd(command)


def signal_init():
    signal.signal(signal.SIGINT, signal.SIG_IGN)  # ignore ctrl+c signal
    signal.signal(signal.SIGTERM, signal.SIG_IGN)  # ignore kill signal
    signal.signal(signal.SIGTSTP, signal.SIG_IGN)  # ignore ctrl+z signal


class RefreshUpgradeBase(object):

    def __init__(self, config, slot_num, devtype, subtype):
        self._config = config
        self._slot_num = slot_num
        self._devtype = devtype
        self._subtype = subtype
        self.device_name = self._config.get("name", None)
        self.refresh_file = self._config.get("refresh_file", None)
        self.init_cmd_list = self._config.get("init_cmd", [])
        self.save_set_reg_list = self._config.get("save_set_reg", [])
        self.rw_recover_reg_list = self._config.get("rw_recover_reg", [])
        self.after_upgrade_delay = self._config.get("after_upgrade_delay", None)
        self.after_upgrade_delay_timeout = self._config.get("after_upgrade_delay_timeout", None)
        self.refresh_finish_flag_check_config = self._config.get("refresh_finish_flag_check", None)
        self.access_check_reg_config = self._config.get("access_check_reg", {})
        self.time_delay = 0
        self.finish_cmd_list = self._config.get("finish_cmd", [])

    def get_config(self):
        pass

    def get_slot_num(self):
        pass

    def save_value(self, cfg_list):
        for config in cfg_list:
            ret, val = get_value(config)
            if ret:
                config["value"] = val
            else:
                warmupgradeerror(val)
                return False, val
        return True, "save value success"

    def save_and_set_value(self, cfg_list):
        for config in cfg_list:
            ret, val = get_value(config)
            if ret:
                config["save_value"] = val
            else:
                warmupgradeerror(val)
                return False, "get save value fail"
            set_val = config.get("set_value", None)
            if set_val is not None:
                config["value"] = set_val
            else:
                warmupgradeerror("save_and_set_value lack of set_val config")
                return False, "set value is not config"
            ret, log = set_value(config)
            if ret is False:
                warmupgradeerror(log)
                return False, log
        return True, "save value success"

    def recover_value(self, cfg_list):
        fail_flag = 0
        for config in cfg_list:
            ret, log = set_value(config)
            if ret is False:
                fail_flag = -1
                warmupgradeerror("recover_value set_value failed, log: %s" % log)
        if fail_flag != 0:
            warmupgradeerror("recover_value write failed")
            return False, "recover write failed"
        return True, "recover write success"

    def recover_save_value(self, cfg_list):
        total_err = 0
        for config in cfg_list:
            val = config.get("save_value", None)
            if val is None:
                warmupgradeerror("recover_save_value lack of save_value config")
                total_err -= 1
                continue
            config["value"] = val
            ret, log = set_value(config)
            if ret is False:
                total_err -= 1
                warmupgradeerror("recover save value write failed, log: %s" % log)
            else:
                warmupgradedebuglog("recover save value success")
        if total_err < 0:
            return False, "recover save value failed"
        return True, "recover save value success"

    def do_fw_upg_init_cmd(self, init_cmd_list):
        # pre operation
        try:
            for init_cmd_config in init_cmd_list:
                ret, log = set_value(init_cmd_config)
                if ret is False:
                    warmupgradeerror("%s do init cmd: %s failed, msg: %s" % (self.device_name, init_cmd_config, log))
                    return False, log
            msg = "%s warm upgrade init cmd all set success" % self.device_name
            warmupgradedebuglog(msg)
            return True, msg
        except Exception as e:
            return False, str(e)

    def do_fw_upg_finish_cmd(self, finish_cmd_list):
        # end operation
        total_err = 0
        for finish_cmd_config in finish_cmd_list:
            ret_t, log = set_value(finish_cmd_config)
            if ret_t is False:
                warmupgradeerror("%s do finish cmd: %s failed, msg: %s" % (self.device_name, finish_cmd_config, log))
                total_err -= 1
        if total_err < 0:
            msg = "%s warm upgrade finish cmd exec failed" % self.device_name
            warmupgradeerror(msg)
            return False, msg
        msg = "%s warm upgrade finish cmd all set success" % self.device_name
        warmupgradedebuglog(msg)
        return True, msg

    def access_test(self, config):
        # polling execute command
        polling_cmd_list = config.get("polling_cmd", [])
        for polling_cmd_config in polling_cmd_list:
            ret, log = set_value(polling_cmd_config)
            if ret is False:
                warmupgradeerror(log)
                return False
        polling_delay = config.get("polling_delay", None)
        if polling_delay is not None:
            time.sleep(polling_delay)

        # record check val
        check_val = config.get("value", None)
        # write value
        ret, log = set_value(config)
        if ret is False:
            warmupgradeerror(log)
            return False
        # read value
        ret, val = get_value(config)
        if ret is False:
            warmupgradeerror(val)
            return False

        # compare write and read val
        warmupgradedebuglog("check_val:%s" % check_val)
        warmupgradedebuglog("get_value:%s" % val)
        if val != check_val:
            warmupgradeerror("check_val:%s != get_value:%s" % (check_val, val))
            return False
        return True

    def check_value(self, config):
        # record check val
        check_val = config.get("value", None)
        ret, val = get_value(config)
        if ret is False:
            warmupgradeerror(val)
            return False
        # compare write and read val
        warmupgradedebuglog("check_val:%s" % check_val)
        warmupgradedebuglog("get_value:%s" % val)
        if val != check_val:
            warmupgradeerror("check_val:%s != get_value:%s" % (check_val, val))
            return False
        return True

    def refresh_file_upgrade(self):
        try:
            warmupgradedebuglog("start %s warm upgrading" % self.device_name)

            # save and set reg
            ret, log = self.save_and_set_value(self.save_set_reg_list)
            if ret is False:
                warmupgradeerror(log)
                self.recover_save_value(self.save_set_reg_list)
                self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                return False, log
            warmupgradedebuglog("%s save and set reg cmd all set success" % self.device_name)
            time.sleep(0.5)  # delay 0.5s after execute save and set reg

            # pre operation
            ret, log = self.do_fw_upg_init_cmd(self.init_cmd_list)
            if ret is False:
                warmupgradeerror(log)
                self.recover_save_value(self.save_set_reg_list)
                self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            # save reg
            ret, log = self.save_value(self.rw_recover_reg_list)
            if ret is False:
                warmupgradeerror("%s save reg failed" % self.device_name)
                self.recover_save_value(self.save_set_reg_list)
                self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                return False, log
            warmupgradedebuglog("%s all reg save success" % self.device_name)

            # upgrade refresh file
            if self.refresh_file is not None:
                status, output = subprocess_warm_upgrade(
                    self.refresh_file, self._devtype, self._subtype, self._slot_num)
                if status:
                    log = "%s refresh file upg failed, msg: %s" % (self.device_name, output)
                    warmupgradeerror(log)
                    self.recover_save_value(self.save_set_reg_list)
                    self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                    return False, log
                warmupgradedebuglog("%s refresh file upg success" % self.device_name)

            # delay the preset time after the upgrade is complete
            if self.after_upgrade_delay is not None:
                time.sleep(self.after_upgrade_delay)

            # check something in the timeout period
            if self.after_upgrade_delay_timeout is not None:
                while self.time_delay < self.after_upgrade_delay_timeout:

                    # check refresh finish flag
                    if self.refresh_finish_flag_check_config is not None:
                        ret = self.check_value(self.refresh_finish_flag_check_config)
                        if ret is False:
                            time.sleep(1)
                            self.time_delay = self.time_delay + 1
                            warmupgradedebuglog("doing refresh_finish_flag_check, time_delay:%s" % self.time_delay)
                            continue
                        warmupgradedebuglog("%s upgrade_finish_flag_check success. self.time_delay:%d"
                                            % (self.device_name, self.time_delay))

                    # doing logic device rw access test
                    ret = self.access_test(self.access_check_reg_config)
                    if ret:
                        warmupgradedebuglog(
                            "%s rw test success. self.time_delay:%d" %
                            (self.device_name, self.time_delay))
                        break
                    time.sleep(1)
                    self.time_delay = self.time_delay + 1
                    warmupgradedebuglog("doing access_test, self.time_delay:%s" % self.time_delay)

                if self.time_delay >= self.after_upgrade_delay_timeout:
                    log = "wait %s access test timeout" % self.device_name
                    warmupgradeerror(log)
                    self.recover_save_value(self.save_set_reg_list)
                    self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                    return False, log
                warmupgradedebuglog("%s access test success" % self.device_name)

            # recover reg
            ret, log = self.recover_value(self.rw_recover_reg_list)
            if ret is False:
                warmupgradeerror("recover %s reg failed" % self.device_name)
                self.recover_save_value(self.save_set_reg_list)
                self.do_fw_upg_finish_cmd(self.finish_cmd_list)
                return False, log
            warmupgradedebuglog("recover %s reg success" % self.device_name)
            # finally
            ret1, log1 = self.recover_save_value(self.save_set_reg_list)
            if ret1 is False:
                warmupgradeerror("bmc upgrade recover save value failed, msg: %s" % log1)
            ret2, log2 = self.do_fw_upg_finish_cmd(self.finish_cmd_list)
            if ret2 is False:
                warmupgradeerror("bmc upgrade do finish command failed, msg: %s" % log2)
            if ret1 is False or ret2 is False:
                return False, "upgrading %s recover save value or finish command failed" % self.device_name
            return True, "upgrading %s success" % self.device_name

        except Exception as e:
            log = "refresh file upgrade Exception happend, error log : %s" % str(e)
            self.recover_save_value(self.save_set_reg_list)
            self.do_fw_upg_finish_cmd(self.finish_cmd_list)
            return False, log


class RefreshUpgrade(RefreshUpgradeBase):

    def __init__(self, config, slot_num, devtype, subtype):
        super(RefreshUpgrade, self).__init__(config, slot_num, devtype, subtype)

    def get_config(self):
        super(RefreshUpgrade, self).get_config()
        return self._config

    def get_slot_num(self):
        super(RefreshUpgrade, self).get_slot_num()
        return self._slot_num


class WarmBasePlatform():

    def __init__(self):
        signal_init()
        debug_init()
        self.warm_upgrade_param = WARM_UPGRADE_PARAM.copy()
        self.stop_services_cmd_list = self.warm_upgrade_param.get("stop_services_cmd", [])
        self.start_services_cmd_list = self.warm_upgrade_param.get("start_services_cmd", [])
        self.__warm_upgrade_config_list = []

    def execute_command_list(self, cmd_list):
        for cmd_item in cmd_list:
            warmupgradedebuglog("execute cmd: %s" % cmd_item)
            status, output = exec_os_cmd(cmd_item)
            if status:
                log = "execute %s failed, msg: %s" % (cmd_item, output)
                warmupgradeerror(log)
                return False, log
        return True, "execute success"

    def stop_services_access(self):
        return self.execute_command_list(self.stop_services_cmd_list)

    def start_services_access(self):
        return self.execute_command_list(self.start_services_cmd_list)

    def check_slot_present(self, slot_present_config):
        totalerr = 0
        presentbit = slot_present_config.get('presentbit')
        ret, value = get_value(slot_present_config)
        if ret is False:
            return "NOT OK"
        if isinstance(value, str):
            val_t = int(value, 16)
        else:
            val_t = value
        val_t = (val_t & (1 << presentbit)) >> presentbit
        if val_t != slot_present_config.get('okval'):
            status = "ABSENT"
        else:
            status = "PRESENT"
        return status

    def linecard_present_check(self, slot_name, slot_present_config):
        present_status = self.check_slot_present(slot_present_config)
        present_status_tuple = ("ABSENT", "NOT OK")
        if present_status in present_status_tuple:
            return False, ("%s not present, warm upgrade exit" % slot_name)
        warmupgradedebuglog("%s present" % slot_name)
        return True, ("%s present" % slot_name)

    def start_warmupgrade(self):
        try:
            # start refresh file upgrade process
            for dev in self.__warm_upgrade_config_list:
                ret, log = dev.refresh_file_upgrade()
                if ret is False:
                    return ret, log
            return True, "all success"
        except Exception as e:
            log = "Exception happend, error log : %s" % str(e)
            return False, log

    def do_warmupgrade(self, file, main_type, sub_type, slot, file_type, chain):
        try:
            # upgrade file existence check
            if not os.path.isfile(file):
                return False, "%s not found" % file

            # get slot config
            slot_name = "slot%d" % slot
            slot_config = self.warm_upgrade_param.get(slot_name, {})
            if len(slot_config) == 0:
                return False, ("%s config not found" % slot_name)

            # linecard present check
            slot_present_config = slot_config.get("present", {})
            if len(slot_present_config) != 0:
                ret, log = self.linecard_present_check(slot_name, slot_present_config)
                if ret is False:
                    return False, log

            # match file_type and chain_num get chain_config
            file_type_config = slot_config.get(file_type, {})
            chain_name = "chain%d" % chain
            chain_list = file_type_config.get(chain_name, [])
            self.__warm_upgrade_config_list = []
            for refresh_config in chain_list:
                # refresh_file existence check
                refresh_file_judge_flag = refresh_config.get("refresh_file_judge_flag", 0)
                if refresh_file_judge_flag == 1:
                    refresh_file = refresh_config.get("refresh_file", None)
                    if not os.path.isfile(refresh_file):
                        log = "%s not found" % refresh_file
                        return False, log
                # each refresh_config add as an instance of RefreshUpgrade Class
                refresh_instance = RefreshUpgrade(refresh_config, slot, main_type, sub_type)
                self.__warm_upgrade_config_list.append(refresh_instance)

            ret, log = self.start_warmupgrade()
            if ret is False:
                warmupgradeerror("doing warm upgrade failed")
                warmupgradeerror(log)
                return ret, log

        except Exception as e:
            log = "Exception happend, error log : %s" % str(e)
            return False, log
        return True, "all success"

    def do_warm_upgrade(self, file, main_type, sub_type, slot, file_type, chain):
        print("+================================+")
        print("|Begin warm upgrade, please wait..|")
        ret, log = self.do_warmupgrade(file, main_type, sub_type, slot, file_type, chain)
        if ret:
            print("|     warm upgrade succeeded!    |")
            print("+================================+")
            sys.exit(0)
        else:
            print("|       warm upgrade failed!     |")
            print("+================================+")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)


@click.group(invoke_without_command=True, context_settings=CONTEXT_SETTINGS)
@click.argument('file', required=True)
@click.argument('main_type', required=True)
@click.argument('sub_type', required=True)
@click.argument('slot', required=True)
@click.argument('file_type', required=True)
@click.argument('chain', required=True)
def main(file, main_type, sub_type, slot, file_type, chain):
    '''warm upgrade'''
    signal_init()
    debug_init()
    platform = WarmBasePlatform()
    platform.do_warm_upgrade(file, int(main_type, 16), int(sub_type, 16), int(slot), file_type, int(chain))


# warm upgrade
if __name__ == '__main__':
    main()
