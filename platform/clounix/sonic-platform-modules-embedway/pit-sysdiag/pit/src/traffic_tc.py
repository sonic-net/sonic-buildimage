import sys
import os
import re
import time
import threading
import subprocess
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import *

CTC_CMD_FORMAT = "ctc_shell -e \"{}\""

def run_command(cmd):
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode == 0:
        if re.search("connect error", err):
            return 1, err
    return proc.returncode, out.rstrip('\n')


class TRAFFICTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "traffic_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        
        if self.platform_cfg_json and self.platform_cfg_json['asic']:
            self.asic = self.platform_cfg_json['asic']
        else:
            self.asic = None
        if self.asic == "centec":
            cmd = "docker cp syncd:/usr/bin/ctc_shell /usr/bin/"
            run_command(cmd)

        platform, hwsku = get_platform_and_hwsku()
        if len(platform) != 0:
            self.platform_path = "/".join([PLATFORM_ROOT_PATH, platform])
        else:
            self.platform_path = PLATFORM_ROOT_PATH_DOCKER
        self.hwsku_path = "/".join([self.platform_path, hwsku])
        self.sdk_path = "/".join([self.platform_path, "sdk"])
        self.port_lanes = []
        self.port_names = []


    def load_port_config_ini(self, also_print_console=False):        
        self.logger.log_info("Get port config ...", also_print_console)
        port_config_file = "/".join([self.hwsku_path, "port_config.ini"])

        try:
            f = open(port_config_file, "r")
        except:
            return IO_ERR
        
        for line in f:
            line.strip()
            if re.search("^#", line):
                continue
            self.port_names.append(line.split()[0])
            self.port_lanes.append(int(line.split()[1]))
        
        return NO_ERR
                

    def port_link_status_check(self, also_print_console=False):
        RET = NO_ERR

        self.logger.log_info("[PORT LINK STATUS CHECK]:", also_print_console)
        cmd = CTC_CMD_FORMAT.format("show port mac-link")
        ret, out = run_command(cmd)
        if ret:
            self.fail_reason.append("exec cmd fail")
            RET |= IO_ERR
        else:
            lines = out.splitlines()
            for line in lines:
                line = line.strip()
                if re.search(r"0x[0-9A-F]+", line):
                    port_lane = int(re.findall("0x[0-9A-F]+",line)[0].strip(), 16)
                    index = self.port_lanes.index(port_lane)
                    port_link = line.split()[2]
                    if port_link == 'down':
                        self.fail_reason.append("{} down".format(self.port_names[index]))
                        RET |= NORUN_ERR
        if RET:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return RET
    
    
    def clear_default_config(self, also_print_console=False):
        self.logger.log_info("Clear default config ...", also_print_console)
        cmd = "systemctl stop lldp snmp swss syncd"
        run_command(cmd)
        cmd = "echo 1 > /proc/sys/net/ipv6/conf/all/disable_ipv6"
        run_command(cmd)
        cmd = "echo 1 > /proc/sys/net/ipv6/conf/default/disable_ipv6"
        run_command(cmd)
        cmd = "config vlan del 1000"
        run_command(cmd)
        return NO_ERR
    
    
    def restore_default_config(self, also_print_console=False):
        self.logger.log_info("Restore default config ...", also_print_console)
        
        ret = NO_ERR
        cmd = "systemctl start swss"
        run_command(cmd)
        return ret
    
    
    def clear_port_packets(self, also_print_console=False):
        self.logger.log_info("Clear port counters ...", also_print_console)
        ret = NO_ERR
        cmd = CTC_CMD_FORMAT.format("clear packet stats")
        ret, output = run_command(cmd)
        cmd = CTC_CMD_FORMAT.format("clear stats mac-all all")
        ret, output = run_command(cmd)
        return ret
    
    
    def is_sdk_running(self):
        cmd = "ps -ef | grep ctcsdk | grep -v grep"
        ret, out = run_command(cmd)
        if ret == 0 and out != '':
            return True
        else:
            return False
    
    
    def start_sdk(self, also_print_console=False):
        self.logger.log_info("Start sdk ...", also_print_console)
        cmd = "chmox +x {}/ctcsdk".format(self.sdk_path)
        run_command(cmd)
        cmd = "chmox +x {}/ctc_shell".format(self.sdk_path)
        run_command(cmd)
        cmd = "cp {}/start_up.cfg-snake-traffic {}/start_up.cfg".format(self.sdk_path, self.sdk_path)
        run_command(cmd)
        cmd = "cd {} && ./ctcsdk &".format(self.sdk_path)
        thread = threading.Thread(target=run_command, args=(cmd,))
        thread.start()
    
    
    def stop_sdk(self, also_print_console=False):
        self.logger.log_info("Stop sdk ...", also_print_console)
        cmd = "ps -ef | grep ctcsdk | grep -v grep | awk '{print $2}' | xargs kill -9 "
        run_command(cmd)


    def start_send_packets(self, count, also_print_console=False):
        self.logger.log_info("Start traffic ...", also_print_console)
        ret = NO_ERR
        cmd = CTC_CMD_FORMAT.format("packet tx mode 0 dest-gport {} bypass pkt-file {}/pkt_file.1 count {}".format(self.port_lanes[0], self.sdk_path, count))
        ret, output = run_command(cmd)
        #time.sleep(1)
        cmd = CTC_CMD_FORMAT.format("packet tx mode 0 dest-gport {} bypass pkt-file {}/pkt_file.2 count {}".format(self.port_lanes[1], self.sdk_path, count))
        ret, output = run_command(cmd)
        #time.sleep(1)
        
        cmd = CTC_CMD_FORMAT.format("packet tx mode 0 dest-gport {} bypass pkt-file {}/pkt_file.1 count {}".format(self.port_lanes[-1], self.sdk_path, count))
        ret, output = run_command(cmd)
        #time.sleep(1)
        cmd = CTC_CMD_FORMAT.format("packet tx mode 0 dest-gport {} bypass pkt-file {}/pkt_file.2 count {}".format(self.port_lanes[-2], self.sdk_path, count))
        ret, output = run_command(cmd)
        #time.sleep(1)
        return ret
    
    
    def stop_send_packets(self, also_print_console=False):
        ret = NO_ERR
        self.logger.log_info("Stop traffic ...", also_print_console)

        try:
            cmd = CTC_CMD_FORMAT.format("vlan remove vlan 10")
            ret, out = run_command(cmd)
            cmd = CTC_CMD_FORMAT.format("vlan remove vlan 100")
            ret, out = run_command(cmd)
            if ret:
                return status
        except Exception as e:
            ret |= IO_ERR
            self.logger.log_err(str(e))
        
        return ret
    
    
    def __get_port_packets(self, port, direc="tx"):
        good = 0
        fcs = 0
        
        if direc == 'tx':
            cmd = CTC_CMD_FORMAT.format("show stats mac-tx port {}".format(port))
        elif direc =='rx':
            cmd = CTC_CMD_FORMAT.format("show stats mac-rx port {}".format(port))
        ret, out = run_command(cmd)
        lines = out.splitlines()
        for line in lines:
            line = line.strip()
            if line.startswith("good unicast"):
                good += int(line.split()[2])
            
            if line.startswith("fcs error"):
                fcs += int(line.split()[2])
            
            if line.startswith("bad"):
                fcs += int(line.split()[2])
            
        return (good, fcs)
    
    
    def port_counter_check(self, also_print_console=False):
        ret = NO_ERR
        header = ["Port", "Tx_count", "Tx_err", "Rx_count", "Rx_err"]
        counter_tbl = []
        
        self.logger.log_info("[PORT TX/RX_COUNTER CHECK]:", also_print_console)
        
        try:
            for port in self.port_lanes:
                name = self.port_names[self.port_lanes.index(port)]
                tx_good, tx_fcs = self.__get_port_packets(port, "tx")
                rx_good, rx_fcs = self.__get_port_packets(port, "rx")
                
                if tx_fcs != 0 or rx_fcs != 0:
                    ret |= FCS_ERR
                    self.fail_reason.append("{} fcs error".format(name))
                
                if tx_good == 0 or rx_good == 0:
                    ret |= NORUN_ERR
                    self.fail_reason.append("{} no traffic".format(name))

                counter_tbl.append([name, tx_good, tx_fcs, rx_good, rx_fcs])
        except Exception as e:
            ret |= IO_ERR
            self.fail_reason.append(str(e))
        
        if len(counter_tbl) > 0:
            self.logger.log_info(tabulate(counter_tbl, header, tablefmt="simple"), also_print_console)
        
        if ret:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
        

    def __port_rate_valid(self, speed, rate):
        ret = NO_ERR
        limit = 0.9
        if speed.find("G") > 0:
            speed_value = int(speed.strip('G'))
            rate_value = int(rate[0])
            rate_unit = rate[1]
            
            if rate_unit == "Mbps":
                standard_rate = speed_value * 1000 * limit
                ret = NO_ERR if rate_value >= standard_rate else UNMATCH_ERR
            elif rate_unit == "Gbps":
                standard_rate = speed_value * limit
                ret = NO_ERR if rate_value >= standard_rate else UNMATCH_ERR
            else:
                ret = UNMATCH_ERR
        else:
            ret = UNMATCH_ERR
        
        return ret


    def port_speed_check(self, also_print_console=False):
        ret = NO_ERR
        header = ["Port", "Speed", "Tx_rate", "Rx_rate", "Result"]
        rate_tbl = []
        
        self.logger.log_info("[PORT SPEED CHECK]:", also_print_console)
        
        try:
            cmd = CTC_CMD_FORMAT.format("show port mac-link")
            status, out = run_command(cmd)
            time.sleep(2)
            cmd = CTC_CMD_FORMAT.format("show port mac-link")
            status, out = run_command(cmd)
            if status:
                self.fail_reason.append("exec cmd fail")
                ret |= IO_ERR
            else:
                lines = out.splitlines()
                for line in lines:
                    line = line.strip()
                    if line.startswith("0x"):
                        port = int(line.split()[0], 16)
                        name = self.port_names[self.port_lanes.index(port)]
                        speed = line.split()[4]
                        rx_rate = (line.split()[8], line.split()[9])
                        tx_rate = (line.split()[10], line.split()[11])
                        status = self.__port_rate_valid(speed, rx_rate)
                        status |= self.__port_rate_valid(speed, tx_rate)
                        if status:
                            self.fail_reason.append("{} low speed".format(name))
                            result = "Fail"
                        else:
                            result = "Pass"
                        ret |= status
                        rate_tbl.append([name, speed, tx_rate[0]+tx_rate[1], rx_rate[0]+rx_rate[1], result])
        except Exception as e:
            ret |= IO_ERR
            self.fail_reason.append(str(e))
        
        if len(rate_tbl) > 0:
                self.logger.log_info(tabulate(rate_tbl, header, tablefmt="simple"), also_print_console)
        if ret:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret

    
    def run_test(self, *argv):
        ret = NO_ERR
        
        self.stop_sdk()
        ret = self.clear_default_config(True)
        time.sleep(2)
        ret |= self.load_port_config_ini(True)
        if ret:
            self.logger.log_err("get port config fail!", True)
            self.restore_default_config(True)
            return ret
        
        ret = self.start_sdk(True)
        time.sleep(30)
        if not self.is_sdk_running():
            self.logger.log_err("sdk start failed!", True)
            self.restore_default_config(True)
            return NORUN_ERR
        
        ret = self.port_link_status_check(True)
        if ret:
            self.stop_sdk(True)
            self.restore_default_config(True)
            return ret
        
        self.clear_port_packets(True)
        time.sleep(1)
        self.start_send_packets(1000, True)
        
        # traffic test time length
        time.sleep(60)
        
        ret = self.port_speed_check(True)
        
        self.stop_send_packets(True)
        
        time.sleep(2)
        ret |= self.port_counter_check(True)
        
        self.stop_sdk(True)
        self.restore_default_config(True)
        return ret
