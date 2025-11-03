#!/usr/bin/python
'''
H3C hardware monitor program
'''

from fcntl import LOCK_UN, flock
from sonic_platform.platform import Platform

try:
    import os
    import sys
    import time
    import syslog
    import fcntl
    from swsssdk import ConfigDBConnector
    from vendor_sonic_platform.devcfg import Devcfg
    from vendor_sonic_platform.utils import find_all_hwmon_paths, get_ssd_temp, \
        get_vr_temp, get_mac_temp_validata, UtilsHelper
except ImportError as error:
    raise ImportError('%s - required module not found' % str(error))

DMESG_NEW_LOG_INTERVAL = 3600


def read_file(path):
    '''
    Read file content
    '''
    try:
        if os.path.exists(path):
            with open(path, "r") as temp_read:
                value = temp_read.read()
        else:
            return 'N/A'
    except Exception as error:
        log_error("unable to read %s file , Error: %s" % (path, str(error)))
        return 'N/A'

    return value


def data_write(file_path, data):
    '''
    write data to file
    '''
    try:
        if os.path.exists(file_path):
            with open(file_path, "w") as temp_write:
                temp_write.write(str(data))
        else:
            log_error("file %s is not exist!" % (file_path))
    except Exception as error:
        log_error("unable to write %s file , Error: %s" % (file_path, str(error)))


def log_notice(log_str):
    '''notice logging'''
    syslog.syslog(syslog.LOG_NOTICE, log_str)


def log_error(log_str):
    '''error logging'''
    syslog.syslog(syslog.LOG_ERR, log_str)


def get_reboot_cause():
    '''
    Get reboot cause from CPLD
    and write it to LAST_REBOOT_CAUSE_PATH (/var/cache/last_reboot_cause)
    '''
    last_reboot_reg_value = 0
    reboot_reg_value = 0
    reboot_cause = Devcfg.REBOOT_CAUSE_NONE
    log_notice('start get reboot cause')
    try:
        '''
        with open(Devcfg.DEBUG_CPLD_DIR + 'cpu_cpld', 'rb+') as cpu_cpld:
            contents = cpu_cpld.readlines()
            for content in contents:
                if content.startswith('0x0030'):
                    last_reboot_reg_value = int(content.split()[9], 16)
                if content.startswith('0x0020'):
                    reboot_reg_value = int(content.split()[1], 16)
        '''
        contents = read_file(Devcfg.LAST_REBOOT_CAUSE_CPLD_PATH)
        if contents == 'N/A':
            return False
        last_reboot_reg_value = int(contents, 16) & 0x7F
        contents = read_file(Devcfg.REBOOT_CAUSE_CPLD_PATH)
        if contents == 'N/A':
            return False
        reboot_reg_value = int(contents, 16) & 0x7F
        if reboot_reg_value == 0:
            reboot_cause = Devcfg.REBOOT_CAUSE_NONE
            log_notice('The reboot cause:REBOOT_CAUSE_NONE')

        elif reboot_reg_value == last_reboot_reg_value:
            if reboot_reg_value & 0x02:
                reboot_cause = Devcfg.REBOOT_CAUSE_CODE_WATCHDOG
                log_notice('The reboot cause:REBOOT_CAUSE_WATCHDOG')
            elif reboot_reg_value & 0x04:
                reboot_cause = Devcfg.REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC
                log_notice('The reboot cause:REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC')
            elif reboot_reg_value & 0x08:
                reboot_cause = Devcfg.REBOOT_CAUSE_CODE_SW
                log_notice('The reboot cause:REBOOT_CAUSE_SW')
            elif reboot_reg_value & 0x40:
                reboot_cause = Devcfg.REBOOT_CAUSE_CODE_POWER_LOSS
                log_notice('The reboot cause:REBOOT_CAUSE_POWER_LOSS')
            elif reboot_reg_value & 0x80:
                reboot_cause = Devcfg.REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_CPU
                log_notice('The reboot cause:REBOOT_CAUSE_THERMAL_OVERLOAD_CPU')

        elif ((reboot_reg_value != 0) and (reboot_reg_value != last_reboot_reg_value)):
            if reboot_reg_value & 0x02:
                if last_reboot_reg_value & 0x02:
                    reboot_cause = Devcfg.REBOOT_CAUSE_CODE_WATCHDOG
                    log_notice('The reboot cause:REBOOT_CAUSE_WATCHDOG')
                else:
                    log_notice('Before the last_reboot_cause: REBOOT_CAUSE_WATCHDOG')
            if reboot_reg_value & 0x04:
                if last_reboot_reg_value & 0x04:
                    reboot_cause = Devcfg.REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC
                    log_notice('The reboot cause:Devcfg.REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC')
                else:
                    log_notice('Before the last_reboot_cause: REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC')
            if reboot_reg_value & 0x08:
                if last_reboot_reg_value & 0x02:
                    reboot_cause = Devcfg.REBOOT_CAUSE_CODE_SW
                    log_notice('The reboot cause:Devcfg.REBOOT_CAUSE_CODE_SW')
                else:
                    log_notice('Before the last_reboot_cause: REBOOT_CAUSE_SW')
            if reboot_reg_value & 0x40:
                if last_reboot_reg_value & 0x40:
                    reboot_cause = Devcfg.REBOOT_CAUSE_CODE_POWER_LOSS
                    log_notice('The reboot cause:REBOOT_CAUSE_POWER_LOSS')
                else:
                    log_notice('Before the last_reboot_cause: REBOOT_CAUSE_POWER_LOSS')
            if reboot_reg_value & 0x80:
                if last_reboot_reg_value & 0x40:
                    reboot_cause = Devcfg.REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_CPU
                    log_notice('The reboot cause:REBOOT_CAUSE_THERMAL_OVERLOAD_CPU')
                else:
                    log_notice('Before the last_reboot_cause: REBOOT_CAUSE_THERMAL_OVERLOAD_CPU')

        with open(Devcfg.LAST_REBOOT_CAUSE_PATH, 'w') as temp_write:
            log_notice('Set reboot cause to %s' % Devcfg.LAST_REBOOT_CAUSE_PATH)
            temp_write.write(str(reboot_cause))
            time.sleep(0.01)

            with open(Devcfg.HW_CLR_RST, 'w') as temp:
                log_notice('Clear the hardware reboot record')
                temp.write("1")
            return True
    except Exception as error:
        log_error(str(error))
        return False

    return False

# Make a class we can use to capture stdout and sterr in the log


class dev_hw_monitor(object):
    '''
    hardware monitor class
    '''

    def __init__(self):

        self.temp_his = list()
        self.temp_his_num = 30 / Devcfg.MONITOR_INTERVAL_SEC
        self.max6696_dir = list()
        self.i350_dir = list()
        self.come_dir = list()
        self._old_target_speed = 0
        self.fandog_feed_value = 3
        self.run_time_seconds = 5
        self.sfps = Platform('sfp').get_chassis().get_all_sfps()
        self.state_db = ConfigDBConnector()
        self.state_db.db_connect("STATE_DB", wait_for_init=False, retry_on=True)
        self.mac_cunt_err = 0
        self.latest_mac_temp = 0
        self.mac_temp_list = list()

        # It tells pylint that we don't want to use the variable in range
        for _ in range(0, 5):
            self.max6696_dir = find_all_hwmon_paths("Max6696")
            self.i350_dir = find_all_hwmon_paths("i350bb")
            self.come_dir = find_all_hwmon_paths("coretemp")
            drv_installed = os.path.exists(Devcfg.SFP_DIR)
            if len(self.come_dir) != Devcfg.COME_DIR_NUM or not drv_installed:
                time.sleep(2)
            else:
                break

    def get_sfp_max_temp_from_db(self):
        temp_list = [0]
        dom_table = self.state_db.get_table('TRANSCEIVER_DOM_SENSOR')

        for dom in dom_table.values():
            temperature = dom.get('temperature', 0)
            if temperature != 'N/A':
                temp_list.append(float(temperature))

        return max(temp_list)

    def _get_spot_temp(self, sensor_type, sensor_index, spot_index):
        temp = 0

        try:
            if sensor_type == 'max6696':
                spot = 'temp_input'
                sysfs_path = self.max6696_dir[sensor_index]
                temp = read_file(os.path.join(sysfs_path, spot))
                if temp != 'N/A':
                    temp = int(float(temp))

            elif sensor_type == 'coretemp':
                temp_list = list()
                for i in range(1, Devcfg.CPU_THERMAL_NUM + 1):
                    sysfile = self.come_dir[sensor_index] + "temp{}_input".format(i)
                    r_temp = read_file(sysfile)
                    if r_temp != 'N/A':
                        temp_list.append(int(r_temp) * Devcfg.TEMP_RATIO)
                temp = max(temp_list)

            elif sensor_type == 'i350':
                spot = 'temp%d_input' % (spot_index + 1)
                sysfs_path = self.i350_dir[sensor_index]
                temp = read_file(os.path.join(sysfs_path, spot))
                if temp != 'N/A':
                    temp = int(temp) * Devcfg.TEMP_RATIO

            elif sensor_type == 'ssd':
                temp = get_ssd_temp()

            elif sensor_type == 'tvr':
                temp = get_vr_temp()

            elif sensor_type == 'mac':
                temp = get_mac_temp_validata(self.latest_mac_temp)
                if len(self.mac_temp_list) < 5:
                    self.mac_temp_list.append(temp)
                else:
                    val = self.latest_mac_temp - temp
                    if val >= 5:
                        self.mac_cunt_err = self.mac_cunt_err + 1
                        if (self.mac_cunt_err % 60 == 0):
                            syslog.syslog(syslog.LOG_INFO, str("mac temp info have mac_cunt_change, temp {}".format(temp)))

                        if self.mac_cunt_err >= 1800:
                            log_error("mac temp error have mac_cunt_err, temp {} , list {}  latest_temp {}".format(temp, self.mac_temp_list, self.latest_mac_temp))
                            self.mac_cunt_err = 0
                            self.latest_mac_temp = temp
                            self.mac_temp_list = []
                            self.mac_temp_list.append(temp)
                            return temp
                        else:
                            temp = self.latest_mac_temp
                    else:
                        self.mac_cunt_err = 0
                    self.mac_temp_list.pop(0)
                    self.mac_temp_list.append(temp)

                median_temp = sorted(self.mac_temp_list)[int((len(self.mac_temp_list)- 1) /2)]
                self.latest_mac_temp = median_temp
                return median_temp

            elif sensor_type == 'sfp':
                temp = self.get_sfp_max_temp_from_db()
        except BaseException as err:
            syslog.syslog(syslog.LOG_ERR, str(err))
            temp = 'N/A'
            return temp
        return temp

    def manage_monitorin(self, file_path):
        '''
        get status
        '''
        status = read_file(os.path.join(file_path, "status"))
        if status != 'N/A':
            status_int = int(status)
        else:
            g_status = Devcfg.STATUS_ABSENT
            return g_status

        if status_int == Devcfg.STATUS_ABSENT:
            g_status = Devcfg.STATUS_ABSENT
        elif status_int == Devcfg.STATUS_OK:
            g_status = Devcfg.STATUS_OK
        elif status_int == Devcfg.STATUS_NOT_OK:
            g_status = Devcfg.STATUS_NOT_OK

        return g_status

    def data_write_asic_over_temp(self):
        '''
            cpu_cpld 0x14 bit7 write 0
        '''
        status, result = UtilsHelper.exec_cmd("cat /sys/switch/debug/cpld/cpu_cpld")
        if status != 0:
            log_error("Read cpu_cpld failed!")
            return
        tmp = hex(int(result[299:301], 16) & 127)
        cmd = "echo 0x14:" + tmp + " > /sys/switch/debug/cpld/cpu_cpld"
        os.system(cmd)

    def data_write_fan_dog_enable(self):
        '''
            board_cpld 0x7e bit0 write 0,then write 1 to enable the fandog
        '''
        cmd = "echo 0x7e:" + hex(int(0)) + " > /sys/switch/debug/cpld/board_cpld"
        status, result = UtilsHelper.exec_cmd(cmd)
        if status != 0:
            log_error("write board_cpld, fan dog enable failed!")
        time.sleep(1)
        cmd = "echo 0x7e:" + hex(int(1)) + " > /sys/switch/debug/cpld/board_cpld"
        status, result = UtilsHelper.exec_cmd(cmd)
        if status != 0:
            log_error("write board_cpld, fan dog enable failed!")

    def feed_fan_dog(self):
        '''
            board_cpld 0x7e bit1 write 1 and 0 to feed dog
        '''
        tmp = hex(int(self.fandog_feed_value))
        cmd = "echo 0x7e:" + tmp + " > /sys/switch/debug/cpld/board_cpld"
        status, result = UtilsHelper.exec_cmd(cmd)
        if status != 0:
            log_error("write board_cpld ,feed fan dog failed!")
        if self.fandog_feed_value == 3:
            self.fandog_feed_value = 1
        else:
            self.fandog_feed_value = 3

    def manage_temperature_monitor(self):
        '''
            temperature manage
        '''
        for spot_info in Devcfg.FAN_ADJ_LIST:
            temp_shut = spot_info['Tshut']
            if spot_info['type'] == 'mac':
                temp_input = self._get_spot_temp(spot_info['type'],
                                                 spot_info['index'],
                                                 spot_info['spot'])
                if temp_input == 'N/A':
                    continue
                if len(self.temp_his) < self.temp_his_num:
                    self.temp_his.append(temp_input)
                else:
                    self.temp_his.pop(0)
                    self.temp_his.append(temp_input)
                if len(self.temp_his) == self.temp_his_num and min(self.temp_his) >= temp_shut:
                    log_error(
                        'TEMP_SHUT:temperature %sC is too high > %sC, will reboot device.' %
                        (temp_input, temp_shut))
                    log_error('ASIC Over temperature reboot')
                    os.system("sync; sync; sync;")
                    self.data_write_asic_over_temp()

    def fan_adjust_new(self):
        '''
        Adjust the fan by temperature
        '''
        target_pwm = self.get_target_pwm()
        if self._old_target_speed != target_pwm:
            self._old_target_speed = target_pwm
            with open(Devcfg.FAN_ADJ_DIR, 'w') as fp:
                fcntl.flock(fp.fileno(), fcntl.LOCK_EX)
                fp.write(str(int(target_pwm)))
                fcntl.flock(fp.fileno(), fcntl.LOCK_UN)

    def get_target_pwm(self):

        fan_nor_num = self.get_fan_normalnum()

        if fan_nor_num < Devcfg.FAN_NUM:
            target_pwm = Devcfg.SPEED_TARGET_MAX
            return target_pwm

        target_pwm_list = []

        for spot_info in Devcfg.FAN_ADJ_LIST:
            temp = self._get_spot_temp(spot_info['type'], spot_info['index'], spot_info['spot'])
            if temp == 'N/A':
                continue
            if temp >= spot_info['Th']:
                target_pwm_list.append(spot_info['Nh'])
            elif temp <= spot_info['Tl']:
                target_pwm_list.append(spot_info['Nl'])
            else:
                target_pwm_list.append(spot_info['Nh'] - spot_info['k'] * (spot_info['Th'] - temp))
            # print("type:{},temp:{},target_pwm_list:{}"
            # .format(spot_info['type'], temp, target_pwm_list))

        target_pwm = max(target_pwm_list)

        return target_pwm

    def get_fan_normalnum(self):
        '''
        Get the normal FAN count
        '''
        return self.get_normalnum(Devcfg.FAN_SUB_PATH, Devcfg.FAN_NUM)

    def get_psu_normalnum(self):
        '''
        Get the normal PSU count
        '''
        return self.get_normalnum(Devcfg.PSU_SUB_PATH, Devcfg.PSU_NUM)

    def check_psu_num(self, psu_num):
        '''
        check psu num,th4 need 2 psu for work
        '''
        if Devcfg.PSU_MONITOR_ENABLE and psu_num < 2:
            log_error("This device needs at least two psu supplies to work properly. Please plug in the power supply!")

    def get_normalnum(self, path, count):
        '''
        Get the normal device count
        '''
        normalnum = 0
        for dev_index in range(0, count):
            dev_path = path.format(dev_index + 1)
            status = self.manage_monitorin(dev_path)
            if status == Devcfg.STATUS_OK:
                normalnum = normalnum + 1

        return normalnum

    def manage_fan_led_monitor(self, path, count):
        '''
        if status = ok; led = green;
        else led = red;
        '''
        for dev_index in range(0, count):
            dev_path = path.format(dev_index + 1)
            status = self.manage_monitorin(dev_path)
            if status == Devcfg.STATUS_OK:
                data_write(dev_path + "led_status", Devcfg.LED_COLOR_CODE_GREEN)  # 0->dark 1->green;3->red
            elif status == Devcfg.STATUS_ABSENT:
                data_write(dev_path + "led_status", Devcfg.LED_COLOR_CODE_DARK)
            else:
                data_write(dev_path + "led_status", Devcfg.LED_COLOR_CODE_RED)

    def monitor_dmesg(self, run_time):
        if self.run_time_seconds >= DMESG_NEW_LOG_INTERVAL:
            cmd = "echo 0 > /sys/switch/debug/recent_log"
            os.system(cmd)
            self.run_time_seconds = 5
        else:
            self.run_time_seconds += run_time

    def manage_sysled_control(self, current_running_fan_num, current_running_psu_num):
        '''
        sysled, fan, psu,
        '''
        fan_sysled_color = 0
        psu_sysled_color = 0
        # kuaishou deal with it in health_checker
        #sysled_color = 0

        if current_running_fan_num == Devcfg.FAN_NUM:
            fan_sysled_color = Devcfg.LED_COLOR_CODE_GREEN
        elif current_running_fan_num == Devcfg.FAN_NUM - 1:
            fan_sysled_color = Devcfg.LED_COLOR_CODE_YELLOW
        else:
            fan_sysled_color = Devcfg.LED_COLOR_CODE_RED
        data_write(Devcfg.FAN_LED_DIR, fan_sysled_color)

        if current_running_psu_num == Devcfg.PSU_NUM:
            psu_sysled_color = Devcfg.LED_COLOR_CODE_GREEN
        elif current_running_psu_num == Devcfg.PSU_NUM - 1:
            psu_sysled_color = Devcfg.LED_COLOR_CODE_YELLOW
        else:
            psu_sysled_color = Devcfg.LED_COLOR_CODE_RED
        data_write(Devcfg.PSU_LED_DIR, psu_sysled_color)

        if (fan_sysled_color == Devcfg.LED_COLOR_CODE_GREEN and psu_sysled_color == Devcfg.LED_COLOR_CODE_GREEN):
            sysled_color = Devcfg.LED_COLOR_CODE_GREEN
        elif (fan_sysled_color == Devcfg.LED_COLOR_CODE_RED or psu_sysled_color == Devcfg.LED_COLOR_CODE_RED):
            sysled_color = Devcfg.LED_COLOR_CODE_RED
        elif (fan_sysled_color == Devcfg.LED_COLOR_CODE_YELLOW and psu_sysled_color == Devcfg.LED_COLOR_CODE_YELLOW):
            sysled_color = Devcfg.LED_COLOR_CODE_RED
        else:
            sysled_color = Devcfg.LED_COLOR_CODE_YELLOW
        data_write(Devcfg.SYS_LED_DIR, sysled_color)


def main(argv):
    '''
    Entrypoint
    '''
    # create monitor
    monitor = dev_hw_monitor()

    #global time_cnt
    current_running_fan_num = 0
    data_write(Devcfg.CPU_INIT_OK_REG_DIR, Devcfg.CPU_INIT_OK_CODE)
    data_write(Devcfg.MAC_INIT_OK_REG_DIR, Devcfg.MAC_INIT_OK_CODE)
    monitor.data_write_fan_dog_enable()
    count = 0

    while True:
        time.sleep(Devcfg.MONITOR_INTERVAL_SEC)
        try:
            count = count + 1
            monitor.manage_temperature_monitor()
            if count >= Devcfg.MONITOR_INTERVAL_COUNT:
                count = 0
                current_running_fan_num = monitor.get_fan_normalnum()
                monitor.fan_adjust_new()
                monitor.feed_fan_dog()

                current_running_psu_num = monitor.get_psu_normalnum()
                monitor.check_psu_num(current_running_psu_num)

                monitor.manage_fan_led_monitor(Devcfg.FAN_SUB_PATH, Devcfg.FAN_NUM)

                monitor.manage_sysled_control(current_running_fan_num, current_running_psu_num)

                monitor.monitor_dmesg(Devcfg.MONITOR_INTERVAL_COUNT)
        except BaseException as err:
            import traceback
            traceback.print_stack()
            log_error("ERROR:" + str(err))


if __name__ == '__main__':
    main(sys.argv[1:])
