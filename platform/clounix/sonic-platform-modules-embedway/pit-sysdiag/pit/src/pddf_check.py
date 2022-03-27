import os
from function import exec_cmd
from errcode import E
from decimal import Decimal, getcontext

NUM_NODE = "number"
TEMP_SENSOR_DIR = "/sys_switch/temp_sensor/"
VOLT_SENSOR_DIR = "/sys_switch/vol_sensor/"
CURR_SENSOR_DIR = "/sys_switch/curr_sensor/"
PSU_DIR = "/sys_switch/psu/"

curr_sensor_node = ['alias', 'label', 'max', 'min', 'type', 'value']
volt_sensor_node = ['alias', 'label', 'max', 'min', 'type', 'value']
temp_sensor_node = ['alias', 'min', 'max', 'type', 'value']
psu_sensor_node = [ ['fan_ratio', 'fan_speed'],
                    ['model_name', 'num_power_sensors', 'part_number', 'serial_number'],
                    ['in_curr', 'in_vol', 'in_power'],
                    ['out_curr', 'out_vol', 'out_power', 'out_status', 'out_max_power'] ]


class pddf_base(object):
    def __init__(self):
        return

    def get_sysfs_node_value(self, path):
        fd = os.open(path, os.O_RDONLY)
        s_val = os.read(fd,12)
        if 'NA' in s_val:
            return ['NA', False]
        elif len(s_val) > 1:
            try:
                return [int(s_val), True]
            except:
                return [s_val, True]
        else:
            return [-1, False]

class pddf_rule(pddf_base):
    def __init__(self, logger):
        self.logger = logger
        self.temp_sensor_num = int(exec_cmd("cat " + TEMP_SENSOR_DIR + NUM_NODE))
        self.volt_sensor_num = int(exec_cmd("cat " + VOLT_SENSOR_DIR + NUM_NODE))
        self.curr_sensor_num = int(exec_cmd("cat " + CURR_SENSOR_DIR + NUM_NODE))
        self.temp_num_per_psu = 3
        self.psu_num = 2

    def get_curr_sensor_num(self):
        return self.curr_sensor_num

    def get_volt_sensor_num(self):
        return self.volt_sensor_num

    def get_temp_sensor_num(self):
        return self.temp_sensor_num

    def check_psu_range(self, psu_dir):
        pass_cnt = 0
        fail_cnt = 0
        ret = E.OK

        in_elec = []
        out_elec = []
        for i in range(0,3):
            val_in, status_in = self.get_sysfs_node_value(psu_dir + psu_sensor_node[2][i])
            val_out, status_out = self.get_sysfs_node_value(psu_dir + psu_sensor_node[3][i])
            if status_in is False or status_out is False:
                self.logger.log_err("{} can not read volt|curr|power".format(psu_dir))
                return [0, -1, E.EFAIL]

            if val_in == 'NA':
                val_in = 0
            if val_out == 'NA':
                val_out = 0
            in_elec.append(val_in)
            out_elec.append(val_out)

        cacl_p_in = in_elec[0] * in_elec[1]
        cacl_p_out = out_elec[0] * out_elec[1]
        getcontext().prec = 4

        # check power
        if in_elec[2] != 0 and out_elec[2] != 0:
            if out_elec[2] >= in_elec[2]:
                self.logger.log_err('{} in_power <= out_power'.format(psu_dir), True)
                fail_cnt += 1
                ret = E.EFAIL
                return [pass_cnt, fail_cnt, ret]
            else:
                pass_cnt += 1

            delta_power = Decimal(in_elec[2] - out_elec[2])
            delta_power = round(delta_power/Decimal(in_elec[2]), 2)
            if delta_power > 0.10:
                self.logger.log_warn('{} conversion efficiency is low'.formt(psu_dir))
                fail_cnt += 1
            else:
                pass_cnt += 1

        # check single ring
        if in_elec[2] != 0 and cacl_p_in != 0:
            abs_in = Decimal(abs(cacl_p_in - in_elec[2]))
            delta_in = round(abs_in/Decimal(in_elec[2]), 2)
            if delta_in > 0.05:
                self.logger.log_err('{} input power check fail'.format(psu_dir), True)
                fail_cnt += 1
            else:
                pass_cnt += 1

        if out_elec[2] != 0 and cacl_p_out != 0:
            abs_out = Decimal(abs(cacl_p_out - out_elec[2]))
            delta_out = round(abs_out/Decimal(out_elec[2]), 2)
            if delta_out > 0.05:
                self.logger.log_err('{} output power check fail'.format(psu_dir), True)
                fail_cnt += 1
            else:
                pass_cnt += 1

        if fail_cnt > 0:
            ret = E.EFAIL

        return [pass_cnt, fail_cnt, ret]

    def check_psu_node(self):
        pass_cnt = 0
        fail_cnt = 0
        ret = E.OK
        for index in range(1, self.psu_num+1):
            present, status = self.get_sysfs_node_value(PSU_DIR + 'psu{}/present'.format(index))
            if present != 1:
                self.logger.log_warn(('psu%d not present skip check' % index), True)
                continue
            for nodes in psu_sensor_node:
                for node in nodes:
                    node = PSU_DIR + 'psu{}/'.format(index) + node
                    val, status = self.get_sysfs_node_value(node)
                    if status is False:
                        ret = E.EFAIL
                        fail_cnt += 1
                    else:
                        pass_cnt += 1

            # check psu temp nodes
            for i in range(1, self.temp_num_per_psu+1):
                for node in temp_sensor_node:
                    path = PSU_DIR + 'psu{}/temp{}/'.format(index, i) + node
                    val, status = self.get_sysfs_node_value(path)
                    if status is False:
                        ret = E.EFAIL
                        fail_cnt += 1
                    else:
                        pass_cnt += 1

            self.check_psu_range(PSU_DIR + 'psu{}/'.format(index))
        return [pass_cnt, fail_cnt, ret]

    def check_range(self, path, check_list):
        pass_cnt = 0
        fail_cnt = 0
        low, status = self.get_sysfs_node_value(path + check_list[0])
        if status is False:
            self.logger.log_err("%s%s not work" % (path, check_list[0]), True)
            return [0, 3, E.EFAIL]
        mid, status = self.get_sysfs_node_value(path + check_list[1])
        if status is False:
            self.logger.log_err("%s%s not work" % (path, check_list[1]), True)
            return [0, 3, E.EFAIL]
        high, status = self.get_sysfs_node_value(path + check_list[2])
        if status is False:
            self.logger.log_err("%s%s not work" % (path, check_list[2]), True)
            return [0, 3, E.EFAIL]

        # if mid is not 'NA':
        #     pass_cnt += 1
        # else:
        #     self.logger.log_err("%s%s not work" % (path, check_list[1]), True)
        #     return [0, 3, E.EFAIL]

        if (low is not 'NA') and (mid is not 'NA'):
            if low <= mid:
                pass_cnt += 1
            else:
                self.logger.log_err("%s%s range check fail" % (path, check_list[0]), True)
                fail_cnt += 1
        else:
            self.logger.log_warn("%s%s No actual features are provided" % (path, check_list[0]), True)

        if (mid is not 'NA') and (high is not 'NA'):
            if mid <= high:
                pass_cnt += 1
            else:
                self.logger.log_err("%s%s range check fail" % (path, check_list[0]), True)
                fail_cnt += 1
        else:
            self.logger.log_warn("%s%s No actual features are provided" % (path, check_list[2]), True)

        return [pass_cnt, fail_cnt, E.OK]

    def update_ret_val(self, result_list, new_list):
        result_list[0] += new_list[0]
        result_list[1] += new_list[1]
        if result_list[2] is E.OK:
            result_list[2] = new_list[2]
        return result_list

    def check_node(self, sensor_type):
        pass_cnt = 0
        fail_cnt = 0
        result = E.OK
        if sensor_type == 'vol':
            path = VOLT_SENSOR_DIR
            total_sensor = self.volt_sensor_num
            check_list = ['value', 'min', 'max']
            node_dict = volt_sensor_node
        elif sensor_type == 'curr':
            path = CURR_SENSOR_DIR
            total_sensor = self.curr_sensor_num
            check_list = ['value', 'min', 'max']
            node_dict = curr_sensor_node
        elif sensor_type == 'temp':
            path = TEMP_SENSOR_DIR
            total_sensor = self.temp_sensor_num
            check_list = ['value', 'min', 'max']
            node_dict = temp_sensor_node
        else:
            self.logger.log_warn("sensor type %s not exists" % node_dict, True)
            return [pass_cnt, fail_cnt, result]

        for num in range(1, total_sensor+1):
            check_path = path + sensor_type + ("%d/" % num)
            for node in node_dict:
                node = check_path + node
                if os.path.exists(node):
                    pass_cnt += 1
                else:
                    fail_cnt += 1
                    result = E.EFAIL
                    self.logger.log_err("%s not exists" % node, True)
            ret_val = self.check_range(check_path, check_list)
            pass_cnt += ret_val[0]
            fail_cnt += ret_val[1]
            if result == E.OK:
                result = ret_val[2]
        return [pass_cnt, fail_cnt, result]
