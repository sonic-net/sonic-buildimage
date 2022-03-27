# -*- coding:utf-8
import time
import os
import pexpect
from test_case import TestCaseCommon
from pit_util_common import run_command
from errcode import E

class CPIPORTTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "cpi_port_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME,
                                logger, platform_cfg_file, case_cfg_file)
        self.cpi_port_info_dict = None
        self.script_filename = None
        self.led_cfg_filename = None
        self.sdk_process =  None
        self.port_list = None
        self.platform_path = None
        self.test_loop_times = 0
        try:
            if self.platform_cfg_json and "cpi_port_info" in self.platform_cfg_json.keys():
                self.cpi_port_info_dict = self.platform_cfg_json["cpi_port_info"]
                self.script_filename = self.cpi_port_info_dict["script_filename"]
                self.led_cfg_filename = self.cpi_port_info_dict["led_cfg_filename"]
                self.port_list = self.cpi_port_info_dict["port_list"]
                self.platform_path = os.path.dirname(self.platform_cfg_file)
                self.cpi_port = self.cpi_port_info_dict["cpi_port"]
                self.asic_send_pkt_num =  self.cpi_port_info_dict["asic_send_pkt_num"]
                self.cpu_send_pkt_num =  self.cpi_port_info_dict["cpu_send_pkt_num"]
                self.test_loop_times = self.cpi_port_info_dict["test_loop_times"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def sdk_diag_load_command(self, process, path, delay=2):
        ret = E.EFAIL
        if process == None or path == None:
            return ret
        cmd = 'diag load script name={}'.format(path)
        process.sendline(cmd)
        process.expect('CLX#')
        cmd_output= process.before.encode()
        if "Load script OK" not in cmd_output:
            self.logger.log_info(cmd_output, True)
            ret = E.EFAIL
            return ret
        time.sleep(delay)
        return E.OK
    
    def sdk_clear_packet_counter(self, process, delay=2):
        process.sendline('stat clear')
        process.expect('CLX#')
        time.sleep(delay)
        process.sendline('phy clear fec')
        process.expect('CLX#')
        time.sleep(delay)
    
    def sdk_port_set_property_cmd(self, process, portlist, command, delay=2):
        ret = E.EFAIL
        if process == None or portlist == None or command == None:
            return ret
        cmd = 'port set property portlist={} {}'.format(portlist, command)
        process.sendline(cmd)
        process.expect('CLX#')
        cmd_output= process.before.encode()
        self.logger.log_info(cmd_output, True)
        time.sleep(delay)
    
    def sdk_run_cmd(self, process,cmd):
        process.sendline(cmd)
        process.expect('CLX#')
        cmd_output= process.before.encode()
        print("sdk cmd:{}; output:{}".format(cmd,cmd_output))
        return cmd_output

    def start_sdk_ref(self, script_filename, led_cfg_filename):
        if script_filename == None or led_cfg_filename == None:
            return E.EFAIL
        cmd = 'sdk_ref -s {} -l {}'.format(script_filename, led_cfg_filename)
        process = pexpect.spawn(cmd)
        return process
    
    def enable_port_list(self):
        # enable all portlist
        cmd = 'admin=enable'
        self.sdk_port_set_property_cmd(self.sdk_process, self.port_list, cmd)

    def disable_port_list(self):
        # disable all portlist
        cmd = 'admin=disable'
        self.sdk_port_set_property_cmd(self.sdk_process, self.port_list, cmd)

    def init_sdk(self, also_print_console=False):
        ret = E.EFAIL
        os.system('modprobe ixgbe')
        os.system('modprobe clx_dev')
        os.system('modprobe clx_netif')

        check_user_cmd = 'pgrep sdk_ref'
        status, out = run_command(check_user_cmd)
        if status == 0:
            process_id = out
            if process_id != '':
                status, out = run_command('kill -9 {}'.format(process_id))
                if status:
                    self.fail_reason.append("init sdk remote mode fail")
                    ret = E.EIO
                else:
                    if out != '':
                        self.logger.log_info(out, also_print_console)
                    else:
                        ret = E.OK
            else:
                ret = E.OK
        else:
           ret = E.OK 

        if ret == E.OK:
            self.sdk_process = self.start_sdk_ref(self.script_filename, self.led_cfg_filename)
            self.sdk_process.expect('CLX#')
        else:
            return ret

        return ret

    def config_cpu_interface(self, interface):
        #os.system('ifconfig eth1 192.168.1.2')
        os.system('ifconfig {} promisc'.format(interface))
        time.sleep(2)
        os.system('ifconfig {} up'.format(interface))
        time.sleep(2)
    
    def get_interface_rx_packets(self, interface_name):
        cmd = 'ifconfig {}'.format(interface_name) + '|grep "RX packets"|awk \'{print $3}\''
        status, out = run_command(cmd)
        if status < 0:
            self.fail_reason.append("get {} rx packets error".format(interface_name))
            return E.EFIAL
        self.logger.log_info("{} RX packets info:{}".format(interface_name, out), True)
        return out
    
    def sdk_pkt_send_tx_cmd(self, process, portlist, number=1000):
        ret = -1
        if process == None or portlist == None or number == None:
            return ret
        cmd = 'pkt send tx portlist={} len=250 num={} dmac=00:00:00:22:22:22 smac=00:00:00:00:11:11 payload=0xffff '.format(portlist, number)
        process.sendline(cmd)
        process.expect('CLX#')
        cmd_output= process.before.encode()
        #print(cmd_output)
    
    def check_cpu_port_link_status(self, port):
        ret = E.EFAIL
        cmd = 'ethtool {}|grep "Link detected"'.format(port)
        status, out = run_command(cmd)
        if status < 0:
            self.fail_reason.append("cpu port: {}, not found".format(port))
            return E.EFAIL
        
        if 'yes' in out:
            return E.OK
        else:
            self.fail_reason.append("cpu port: {}, link status error".format(port))
            return E.EFAIL


    def get_sdk_port_rx(self, port):
        cmd = 'stat show portlist {}'.format(port)
        output = self.sdk_run_cmd(self.sdk_process, cmd)
        # get counter's data
        output= output.decode()
        echo = output.split("\r\n")
        rx_octets = (echo[2].split())[3]
        return rx_octets
        

    #ASIC sends 1000 packets to CPU，and check counter on CPU NIC.
    def check_cpi_port_cpu_rx(self, cpu_nic_port, asic_cpi_port, asic_send_pkt_num):
        rx_packet_tmp = self.get_interface_rx_packets(cpu_nic_port)
        if rx_packet_tmp == E.EFAIL:
            return E.EFAIL
        self.sdk_pkt_send_tx_cmd(self.sdk_process, asic_cpi_port, asic_send_pkt_num)
        rx_packet = self.get_interface_rx_packets(cpu_nic_port)
        if rx_packet == E.EFAIL:
            return E.EFAIL
        if asic_send_pkt_num == int(rx_packet) - int(rx_packet_tmp):
            return E.OK
        else:
            self.fail_reason.append("packet loss: sdk send {}, cpu port recv {}".format(asic_send_pkt_num, int(rx_packet) - int(rx_packet_tmp)))
            return E.EFAIL
    
    #CPU sends 5 packets to ASIC, and checks counter in ASIC
    def check_cpi_port_cpu_tx(self,cpu_nic_port, asic_cpi_port,cpu_send_pkt_num):
        ret = E.EFAIL
        self.sdk_clear_packet_counter(self.sdk_process)
        rx_octets_tmp = self.get_sdk_port_rx(asic_cpi_port)
        cmd = 'ping 192.168.1.1 -c {} -I {}'.format(cpu_send_pkt_num, cpu_nic_port)
        status, out = run_command(cmd)
        rx_octets = self.get_sdk_port_rx(asic_cpi_port)
        if (int(rx_octets) - int(rx_octets_tmp)) == cpu_send_pkt_num*192:
            return E.OK
        else:
            self.fail_reason.append("packet loss: cpu send packets {}, sdk port recv octets {}".format(cpu_send_pkt_num, int(rx_octets) - int(rx_octets_tmp)))
            return E.EFAIL
        
    def cpi_port_test(self, also_print_console=False):
        ret = E.EFAIL
        ret = self.init_sdk(also_print_console)
        if ret != E.OK:
            self.logger.log_err("init sdk fail!", also_print_console)
        self.disable_port_list()
        for cpu_port,asic_port in self.cpi_port.items():
            self.config_cpu_interface(cpu_port)
            ret = self.check_cpu_port_link_status(cpu_port)
            if ret != E.OK:
                self.logger.log_err("check cpu port link status fail!", also_print_console)
                return ret
            ret = self.check_cpi_port_cpu_rx(cpu_port, asic_port, self.asic_send_pkt_num)
            if ret != E.OK:
                self.logger.log_err("check cpi port cpu rx fail!", also_print_console)
                return ret
            ret = self.check_cpi_port_cpu_tx(cpu_port, asic_port, self.cpu_send_pkt_num)
            if ret != E.OK:
                self.logger.log_err("check cpi port cpu tx fail!", also_print_console)
                return ret
        self.enable_port_list()
        self.sdk_process.sendline('exit')

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0
        loop = self.test_loop_times
        for i in range(1, loop + 1):
            ret = self.cpi_port_test(True)
            if ret == E.OK:
                pass_cnt += 1
            else:
                fail_cnt += 1
                self.logger.log_err("cpi port test loop {} failed!".format(i), True)
                break

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
