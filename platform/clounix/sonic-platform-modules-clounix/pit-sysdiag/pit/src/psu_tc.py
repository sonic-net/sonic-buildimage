#!/usr/bin/env python
# -*- coding: utf-8 -*-
__version__ = 'v0.1.0'
__author__  = "Pytool Lishoulei<admin@pytool.com>"
import os
import sys
import json
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import E
from function import load_platform_util
'''
4.7 电源信息Sysfs
4.7.1 电源信息Sysfs路径必须为/sys_switch/psu/。
4.7.2 目录/sys_switch/psu/psu[n]提供编号为n的电源的信息,如目录/sys_switch/psu/psu1提供编号为1的电源的信息
路径	权限	类型	描述	
/sys_switch/psu/number	                只读	int	    设备总的电源个数	
/sys_switch/psu/psu[n]/model_name	    只读	string	电源名称	
/sys_switch/psu/psu[n]/hardware_version	只读	string	电源固件版本号	ODCC-2021-03001 S3IP-Sysfs 规范	10	
/sys_switch/psu/psu[n]/serial_number	只读	string	电源序列号	
/sys_switch/psu/psu[n]/part_number	    只读	string	电源零部件号	

/sys_switch/psu/psu[n]/type	            只读	enum	电源类型:	0:直流	1:交流	
/sys_switch/psu/psu[n]/in_curr	        只读	float	电源输入电流，单位:A，保留小数点后3位	
/sys_switch/psu/psu[n]/in_vol	        只读	float	电源输入电压，单位:V，保留小数点后3位	
/sys_switch/psu/psu[n]/in_power	        只读	float	电源输入功率，单位:W，保留小数点后3位	
/sys_switch/psu/psu[n]/out_max_power	只读	float	电源最大输出功率，单位W，保留小数点后3位	
/sys_switch/psu/psu[n]/out_curr	        只读	float	电源输出电流，单位A，保留小数点后3位	
/sys_switch/psu/psu[n]/out_vol	        只读	float	电源输出电压，单位V，保留小数点后3位	
/sys_switch/psu/psu[n]/out_power	    只读	float	电源输出功率，单位W，保留小数点后3位	
/sys_switch/psu/psu[n]/num_temp_sensors	只读	int	    温度传感器数量	
/sys_switch/psu/psu[n]/temp[n]	        只读	        参考温度传感器定义

/sys_switch/psu/psu[n]/present	        只读	enum	状态：	0: 不在位	1: 在位	
/sys_switch/psu/psu[n]/out_status	    只读	enum	输出状态，通过电源内部的POWER_OK pin判断	0:不正常	1:正常	
/sys_switch/psu/psu[n]/in_status	    只读	enum	输入状态，通过电源内部AC_OK pin判断	0:不正常	1:正常	
/sys_switch/psu/psu[n]/fan_speed	    读写	int	    电源风扇转速，单位RPM	
/sys_switch/psu/psu[n]/fan_direction	只读	enum	风道类型定义如下：	0：F2B，前向风道	1：B2F，后向风道	
/sys_switch/psu/psu[n]/led_status	    只读	enum	电源状态灯定义如下：	详见LED状态灯枚举值定义

'''
from function import restful_command
class PSUTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "psuutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "PsuUtil"
    
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "psu_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.psu_util = None
        self.psu_info_dict =None
        try:
            if self.platform_cfg_json and 'psu_info' in self.platform_cfg_json:
                self.psu_info_dict = self.platform_cfg_json['psu_info']
        except Exception as e:
            self.logger.log_err(str(e), True)

        self.position = self.psu_info_dict['position']
        self.psu_count = self.get_psu_count()

        psu_module = load_platform_util(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(psu_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.psu_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e))
            sys.exit(1)

    def write_sysfs(self,file,v):
        if self.position == "bmc":
            rc,_ = restful_command("echo {} > {}".format(v,file))
            return rc
        else:
            with open(file,'w') as f:
                return f.write(v)

    def read_sysfs(self,file):
        if self.position == "bmc":
            rc,v = restful_command("cat {}".format(file))
            if rc:
                return v
        else:
            with open(file,'r') as f:
                return f.read().strip()
        return ""
    
    def get_psu_count(self):
        try:
            return int(self.read_sysfs("/sys_switch/psu/number"),10)
        except Exception as e:
            self.fail_reason.append("get_psu_count fail.")
            self.logger.log_err(str(e), True)
            return 0

    def get_psu_attribute(self,psuid,name):
        try:
            return int(self.read_sysfs('/sys_switch/psu/psu{n}/{}'.format(psuid,name)))
        except Exception as e:
            self.fail_reason.append("fail to get psu{}.{}".format(psuid,name))
            self.logger.log_err("fail to get psu{}.{}".format(psuid,name), True)
            return 0

    def test_psu_info(self):
        '''
        4.9.4 电源信息
        a)、测试方法
            1) 获取PSU数量, 记为$(psu_count)
            2) 遍历所有PSU, 执行以下
                a) 获取PSU FRU 信息
                b) 检查key
                c) 检查状态信息
        b)、 判断标准
            1) 所有命令执行成功
            2) 步骤2-a), key必须包含[PartNumber, SerialNumber, PsuName, Airflow, Presence]
            3) PartNumber, SerialNumber, PsuName 非空且不为相同字符
            4) Presence为True, Airflow为与系统风向相同
            5) $(psu_count)与$(system_psu_count)相同
    '''
        ret = E.OK
        self.psu_count = self.get_psu_count()
        if self.psu_count != self.psu_info_dict['count']:
            self.logger.log_err("FAIL!", True)
            return E.EPSU5000

        for n in range(self.psu_count):
            # present = self.read_sysfs('/sys_switch/psu/psu{n}/present'.format(n=n+1))
            present = self.psu_util.get_psu_presence(n+1)
            if present == 0:
                continue

            model_name = self.read_sysfs('/sys_switch/psu/psu{n}/model_name'.format(n=n+1))
            if not model_name :
                self.logger.log_err("FAIL to read psu {n} model_name".format(n=n+1), True)
                return E.EPSU5001

            part_number = self.read_sysfs('/sys_switch/psu/psu{n}/part_number'.format(n=n+1))
            if not part_number :
                self.logger.log_err("FAIL to read psu {n} part_number".format(n=n+1), True)
                return E.EPSU5001

            # skiped
            # if model_name == part_number:
            #     self.logger.log_err("FAIL psu {n} fru model_name and part_number have the same value".format(n=n+1), True)
            #     return E.EPSU5001
            
            serial_number = self.read_sysfs('/sys_switch/psu/psu{n}/serial_number'.format(n=n+1))
            if not serial_number :
                self.logger.log_err("FAIL to read psu {n} serial_number".format(n=n+1), True)
                return E.EPSU5001

            if serial_number == part_number:
                self.logger.log_err("FAIL psu {n} fru serial_number and part_number have the same value".format(n=n+1), True)
                return E.EPSU5001

            if model_name == serial_number :
                self.logger.log_err("FAIL psu {n} fru model_name and serial_number have the same value".format(n=n+1), True)
                return E.EPSU5001

            # presence = self.read_sysfs('/sys/s3ip/psu/psu{n}/present'.format(n=n+1))
            # if not presence :
            #     self.logger.log_err("FAIL to read psu {n} presence".format(n=n+1), True)
            #     return E.EPSU5001
            
        self.logger.log_info("test_psu_info PASS.", True)
        return ret

    def test_psu_status(self):
        '''
        4.9.5 电源状态
        a)、 测试方法
            1) 获取PSU数量
            2) 遍历所有PSU, 执行以下
                I) 读取PSU input状态, 包括in_status, in_power, in_voltage, in_current
                II) 读取PSU input sensor的范围
                III) 读取PSU output状态, 包括out_status, out_power, out_voltage, out_current
                IV) 读取PSU output sensor的范围
        b)、判断标准
            1) $(psu_count) 和 $(system_psu_count)一致
            2) in_status 应为 True, in_power, in_voltage, in_current 分别处在 in_power_range, in_voltage_range, in_current_range之内
            3) out_status 应为 True, out_power, out_voltage, out_current 分别处在 out_power_range, out_voltage_range, out_current_range
        '''
        ret = E.OK
        self.psu_count = self.get_psu_count()
        if self.psu_count != self.psu_info_dict['count']:
            self.logger.log_err("psu_count FAIL  !", True)
            return E.EPSU5000

        for n in range(self.psu_count):
            # present = self.read_sysfs('/sys_switch/psu/psu{n}/present'.format(n=n+1))
            present = self.psu_util.get_psu_presence(n+1)
            """
                present status:
                0: ABSENT, 不在位
                1: OK, 在位且正常（在位上电）
                2: NOT OK, 在位且异常（在位没上电）
            """
            if present == 0:
                self.logger.log_err("psu {} not present !".format(n+1), True)
                continue
            if  present == 2:
                self.logger.log_err("psu {} present, not ok !".format(n+1), True)
                continue

            # in_status = self.read_sysfs('/sys_switch/psu/psu{n}/in_status'.format(n=n+1))
            # if in_status != "1":
            #     self.logger.log_err("psu {} input AC error !".format(n+1), True)
            #     return E.EPSU5000

            in_curr = int(self.read_sysfs('/sys_switch/psu/psu{n}/in_curr'.format(n=n+1)))
            if self.psu_info_dict['in_curr_max'] < in_curr or self.psu_info_dict['in_curr_min'] > in_curr :
                self.logger.log_err("psu {} in_curr out of range !".format(n+1), True)
                return E.EPSU5000

            in_vol = int(self.read_sysfs('/sys_switch/psu/psu{n}/in_vol'.format(n=n+1)))
            if self.psu_info_dict['in_vol_max'] < in_vol or self.psu_info_dict['in_vol_min'] > in_vol :
                self.logger.log_err("psu {} in_vol out of range !".format(n+1), True)
                return E.EPSU5000

            # in_power = int(self.read_sysfs('/sys_switch/psu/psu{n}/in_power'.format(n=n+1)))
            # if self.psu_info_dict['in_power_max'] < in_power or self.psu_info_dict['in_power_min'] > in_power :
            #     self.logger.log_err("psu {} in_power out of range !".format(n+1), True)
            #     return E.EPSU5000

            # output
            # out_status = self.read_sysfs('/sys_switch/psu/psu{n}/out_status'.format(n=n+1))
            out_status = self.psu_util.get_psu_status(n+1)
            if out_status != 1:
                self.logger.log_err("psu {} input AC error !".format(n+1), True)
                return E.EPSU5000

            out_curr = int(self.read_sysfs('/sys_switch/psu/psu{n}/out_curr'.format(n=n+1)))
            if self.psu_info_dict['out_curr_max'] < out_curr or self.psu_info_dict['out_curr_min'] > out_curr :
                self.logger.log_err("psu {} out_curr out of range !".format(n+1), True)
                return E.EPSU5000

            out_vol = int(self.read_sysfs('/sys_switch/psu/psu{n}/out_vol'.format(n=n+1)))
            if self.psu_info_dict['out_vol_max'] < out_vol or self.psu_info_dict['out_vol_min'] > out_vol :
                self.logger.log_err("psu {} out_vol out of range !".format(n+1), True)
                return E.EPSU5000

            out_power = int(self.read_sysfs('/sys_switch/psu/psu{n}/out_power'.format(n=n+1)))
            if self.psu_info_dict['out_power_max'] < out_power or self.psu_info_dict['out_power_min'] > out_power :
                self.logger.log_err("psu {} out_power out of range !".format(n+1), True)
                return E.EPSU5000
            # out_max_power = self.read_sysfs('/sys/s3ip/psu/psu{n}/out_max_power'.format(n=n+1))
        self.logger.log_info("test_psu_status PASS.", True)
        return ret

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0

        ret = self.test_psu_status()
        if ret != E.OK :
            fail_cnt += 1
        else:
            pass_cnt += 1

        ret = self.test_psu_info()
        if ret != E.OK :
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
