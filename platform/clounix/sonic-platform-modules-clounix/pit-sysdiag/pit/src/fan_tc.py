#!/usr/bin/env python
# -*- coding: utf-8 -*-
__version__ = 'v0.1.0'
__author__  = "Pytool Li<admin@pytool.com>"
import os
from re import T
import sys
import json
import time
import requests
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import load_platform_util_module
from pit_util_common import run_command

'''
4.6 风扇信息Sysfs
4.6.1 风扇信息Sysfs路径必须为/sys_switch/fan/。
4.6.2 一个电机的风扇,motor_number文件内容必须为1,motor1目录提供电机的详细信息。
4.6.3 两个电机的风扇,motor_number文件内容必须为2, motor1目录提供风扇的front或INLET电机详细信息,motor2目录提供风扇的rear或OUTLET电机详细信息。
4.6.4 目录/sys_switch/fan/fan[n]提供编号为n的风扇的信息,如目录/sys_switch/fan/fan1提供编号为1的风扇的信息。
路径	权限	类型	描述
/sys_switch/fan/number	只读	int	设备中总的风扇数量	

/sys_switch/fan/fan[n]/model_name	            只读	string	风扇名称
/sys_switch/fan/fan[n]/serial_number	        只读	string	风扇序列号
/sys_switch/fan/fan[n]/part_number	            只读	string	风扇零部件号
/sys_switch/fan/fan[n]/harware_version	        只读	string	风扇硬件版本号
/sys_switch/fan/fan[n]/status	                只读	enum	风扇状态定义如下：	0：不在位	1：在位且正常	2：在位且异常
/sys_switch/fan/fan[n]/led_status	            读写	enum	风扇状态灯定义如下：	详见LED状态灯枚举值定义

/sys_switch/fan/fan[n]/motor_number	            只读	int	风扇马达数量
/sys_switch/fan/fan[n]/motor[n]/speed	        只读	int	当前转速值,单位RPM
/sys_switch/fan/fan[n]/motor[n]/speed_tolerance	只读	int	风扇转速公差（误差),单位RPM
/sys_switch/fan/fan[n]/motor[n]/speed_target	只读	int	电机标准转速值,单位RPM
/sys_switch/fan/fan[n]/motor[n]/speed_max	    只读	int	电机转速最大值,单位RPM
/sys_switch/fan/fan[n]/motor[n]/speed_min	    只读	int	电机转速最小值,单位RPM
/sys_switch/fan/fan[n]/ratio	        读写	int	电机转速百分比,取值范围0-100	调整该值可完成风扇调速

/sys_switch/fan/fan[n]/direction	    只读	enum	风道类型定义如下：	0：F2B,前向风道	1：B2F,后向风道
'''
from function import restful_command
class FANTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "fanutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "FanUtil"
    
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "fan_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.fan_util = None
        self.fan_info_dict =None
        self.fan_info_cfg =None
        try:
            if self.platform_cfg_json and 'fan_info' in self.platform_cfg_json:
                self.fan_info_cfg = self.platform_cfg_json['fan_info']
        except Exception as e:
            self.logger.log_err(str(e), True)

        # fan_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        # try:
        #     platform_util_class = getattr(fan_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
        #     self.fan_util = platform_util_class()
        # except AttributeError as e:
        #     self.logger.log_err(str(e))
        #     sys.exit(1)
    def write_sysfs(self,file,v):
        self.position = self.fan_info_dict["position"]
        if self.position == "bmc":
            rc,_ = restful_command("echo {} > {}".format(v,file))
            return rc
        else:
            with open(file,'w') as f:
                return f.write(str(v))

    def read_sysfs(self,file):
        self.position = self.fan_info_dict["position"]
        if self.position == "bmc":
            rc,v = restful_command("cat {}".format(file))
            if rc:
                return v
        else:
            with open(file,'r') as f:
                return f.read().strip()
        return ""
    
    def get_fan_count(self):
        try:
            return int(self.fan_info_dict["fan_count"])
        except Exception as e:
            self.logger.log_err(str(e), True)
            return 0

    def get_motor_count(self,fanid):
        try:
            return int(self.read_sysfs('/sys_switch/fan/fan{n}/motor_number'.format(n=fanid)),10)
        except Exception as e:
            self.logger.log_err(str(e), True)
            return 0

    def get_motor_property(self,fanid,motorid,name):
        try:
            v = self.read_sysfs('/sys_switch/fan/fan{}/motor{}/{}'.format(fanid,motorid,name))
            return int(v)
        except Exception as e:
            self.fail_reason.append("fail to get fan{}.motor{}.{}".format(fanid,motorid,name))
            self.logger.log_err("fail to get fan{}.motor{}.{} {}".format(fanid,motorid,name,str(e)), True)
            return 0

    def speed_check(self, fanid, motorid):
        # 1. test speed threshold 
        speed = self.get_motor_property(fanid, motorid, "speed")
        speed_max = self.get_motor_property(fanid, motorid, "speed_max")
        speed_min = self.get_motor_property(fanid, motorid, "speed_min")

        ratio = self.read_sysfs('/sys_switch/fan/fan{n}/ratio'.format(n=fanid))
        ratio = int(ratio)
        speed_tolerance = float(speed_max/10)
        speed_tolerance = int(speed_tolerance * ratio/100)
        speed_per_ratio = self.fan_info_dict["speed_per_ratio"]

        # 2. test speed tolerance
        speed_target = ratio * speed_per_ratio
        speed_delta = abs(speed_target - speed)
        if (speed_delta <= speed_tolerance):
                self.logger.log_info('fan{n} control test done, PASS'.format(n=fanid), True)
                return E.OK
        else:
                self.fail_reason.append("fan {} speed {}, speed_target {},speed_delta {}, out of speed tolerance {}, ratio {}%.".format(fanid,speed,speed_target,speed_delta, speed_tolerance, ratio))
                return E.EFAN6000

    def verify_fan_info(self, v):
        return True


    def test_fan_info(self):
        '''
        4.9.1 风扇信息
        a)、测试方法
            (1) 获取风扇数量, 记为$(fan_count)
            (2) 遍历所有风扇, 执行以下
                I) 获取风扇FRU信息
                II) 检查key
                III) 检查状态信息
        b)、判断标准
            1) 所有命令执行成功
            2) 步骤2-a), key必须包含[PartNumber, SerialNumber, FanName, Airflow, Presence]
            3) PartNumber, SerialNumber, FanName非空且不为相同字符
            4) Presence为True, Airflow为与系统风向相同
            5) $(fan_count)与$(system_fan_count)相同
    '''
        ret = E.OK

        for num in self.fan_info_dict["fan_list"]:
            model_name = self.read_sysfs('/sys_switch/fan/fan{n}/model_name'.format(n=num))
            if not model_name :
                self.logger.log_err("FAIL to read fan {n} model_name".format(n=num), True)
                return E.EFAN6000

            part_number = self.read_sysfs('/sys_switch/fan/fan{n}/part_number'.format(n=num))
            if not part_number :
                self.logger.log_err("FAIL to read fan {n} part_number".format(n=num), True)
                return E.EFAN6000

            serial_number = self.read_sysfs('/sys_switch/fan/fan{n}/serial_number'.format(n=num))
            if not serial_number :
                self.logger.log_err("FAIL to read fan {n} serial_number".format(n=num), True)
                return E.EFAN6000

            if model_name == part_number or part_number == serial_number or model_name == serial_number:
                self.logger.log_err("FAIL fan {n} fru has same value".format(n=num), True)

            presence = self.read_sysfs('/sys_switch/fan/fan{n}/status'.format(n=num))
            if not presence :
                self.logger.log_err("FAIL to read fan {n} presence".format(n=num), True)
                return E.EFAN6000
            
        self.logger.log_info("test_fan_info PASS.", True)
        return ret
 
    def test_fan_status(self):
        '''
        4.9.2 风扇状态
        a)、测试方法
            (1) 获取风扇数量
            (2) 遍历所有风扇, 执行以下步骤
                I) 获取motor个数
                II) 获取每个motor的speed, running_state, alarm_status
                III) 获取每个motor的speed范围
                IV) 对比speed和speed范围
                V) 检查running_state, alarm_status
        b)、 判断标准
            (1) 风扇数量和 $(system_fan_count)一致
            (2) 上述命令执行成功
            (3) 每个motor的running_state为True, alarm_status为False
            (4) 每个motor的speed都在speed范围之内)
        '''
        ret = E.OK

        for n in self.fan_info_dict["fan_list"]:
            n = int(n)
            motor_number = self.get_motor_count(n)
            for i in range(1, motor_number+1):
                ret = self.speed_check(n,i)
        self.logger.log_info("test_fan_status PASS.", True)
        return ret


    def test_fan_control(self):
        '''
        4.9.3 风扇控制
        a)、测试方法
            1) 停止风扇调速控制守护进程
            2) 设置风扇转速到$(fan_speed_test1)
            3) 等待固定时间(3秒) 读取 风扇转速, 记为$(fan_speed_readback)
            4) 对比风扇转速$(fan_speed_readback)和$(fan_speed_test1)          ### verify
            5) 替换$(fan_speed_test1)为$(fan_speed_test2), $(fan_speed_test3),

            重复执行步骤2)~步骤4)
            6) 重新启动风扇控制守护进程
        b)、判断标准
            1) 所有命令执行成功
            2) 步骤4)对比, 一致
    '''
        ret = E.OK
        self.previous_setting()
        for num in self.fan_info_dict["fan_list"]:
            for ratio in self.fan_info_dict['ratio_target']:
                self.write_sysfs('/sys_switch/fan/fan{n}/ratio'.format(n=num), ratio)
                time.sleep(6)
                rt = self.speed_check(num, 1)
                if rt != E.OK:
                    ret = rt
                    self.fail_reason.append("fan {n} control test fail.".format(n=num))
                    self.restore_default_setting()
                    return ret

        self.logger.log_info("test_fan_control PASS.", True)
        self.restore_default_setting()
        return ret            


    def run_test(self, *argv):
        data = json.dumps(self.fan_info_cfg)
        data = json.loads(data)
        fail_cnt = 0
        pass_cnt = 0
        for dict_data in data["test_cfg"]:
            self.fan_info_dict = dict_data

            ret = self.test_fan_info()
            if ret != E.OK :
                fail_cnt += 1
            else:
                pass_cnt += 1

            ret = self.test_fan_status()
            if ret != E.OK :
                fail_cnt += 1
            else:
                pass_cnt += 1

            ret = self.test_fan_control()
            if ret != E.OK :
                fail_cnt += 1
            else:
                pass_cnt += 1

            result = E.OK if fail_cnt == 0  else E.EFAIL

        return [pass_cnt, fail_cnt, result]

    def start_pmon_daemon(self, daemon_name):
        """
        @summary: start daemon in pmon docker using supervisorctl start command.
        """
        pmon_daemon_start_cmd = "docker exec pmon supervisorctl start {}".format(daemon_name)
        status, result = run_command(pmon_daemon_start_cmd)
        

    def stop_pmon_daemon_service(self, daemon_name):
        """
        @summary: stop daemon in pmon docker using supervisorctl stop command.
        """
        pmon_daemon_stop_cmd = "docker exec pmon supervisorctl stop {}".format(daemon_name)

        status, result = run_command(pmon_daemon_stop_cmd)

    def start_fancontrol(self):
        self.start_pmon_daemon("fancontrol")
        self.start_pmon_daemon("thermalctld")

    def stop_fancontrol(self):
        self.stop_pmon_daemon_service("fancontrol")
        self.stop_pmon_daemon_service("thermalctld")

    def stop_pmon(self):
        run_command("systemctl stop pmon.service")

    def start_pmon(self):
        run_command("systemctl start pmon.service")

    def previous_setting(self):
        #self.stop_fancontrol()
        self.stop_pmon()

    # restore fan auto mode
    def restore_default_setting(self):
        #self.start_fancontrol()
        self.start_pmon()
