# -*- coding:utf-8
import time
import os
import pexpect
from test_case import TestCaseCommon
from pit_util_common import run_command
from errcode import E

class SNAKETC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "snake_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME,
                                logger, platform_cfg_file, case_cfg_file)
        self.snake_info_dict = None
        self.script_filename = None
        self.led_cfg_filename = None
        self.sdk_process =  None
        self.port_list = None
        self.sdk_traffic_test_dsh = None
        self.platform_path = None
        try:
            if self.platform_cfg_json and "snake_info" in self.platform_cfg_json.keys():
                self.snake_info_dict = self.platform_cfg_json["snake_info"]
                self.script_filename = self.snake_info_dict["script_filename"]
                self.led_cfg_filename = self.snake_info_dict["led_cfg_filename"]
                self.port_list = self.snake_info_dict["port_list"]
                self.sdk_traffic_test_dsh = self.snake_info_dict["sdk_traffic_test_dsh"]
                self.platform_path = os.path.dirname(self.platform_cfg_file)
                self.total_portnum = self.snake_info_dict["total_portnum"]
                self.qsfp_portnum = self.snake_info_dict["qsfp_portnum"]
                self.port_step = self.snake_info_dict["port_step"]
                self.test_loop_times = self.snake_info_dict["test_loop_times"]
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
        if "failed" in cmd_output:
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
    
    def start_sdk_ref(self, script_filename, led_cfg_filename):
        if script_filename == None or led_cfg_filename == None:
            return E.EFAIL
        cmd = 'sdk_ref -s {} -l {}'.format(script_filename, led_cfg_filename)
        process = pexpect.spawn(cmd)
        return process
    
    def show_sdk_verion(self, process, delay=2):
        cmd = 'diag show version'
        process.sendline(cmd)
        process.expect('CLX#')
        cmd_output= process.before.encode()
        self.logger.log_info(cmd_output, True)
        time.sleep(delay)

    def init_port_list(self):
        # enable all portlist
        cmd = 'admin=enable'
        self.sdk_port_set_property_cmd(self.sdk_process, self.port_list, cmd)
        # set an-lt=lt
        cmd = 'an-lt=lt'
        self.sdk_port_set_property_cmd(self.sdk_process, self.port_list, cmd)

    def init_sdk(self, also_print_console=False):
        ret = E.EFAIL
        
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

    def clx_show_stat_ports(self, process,port1,port2):
        #######################################################
        # stat show portlist=port1,port2                      #
        # must on clx sdk module
        # get port's rx and tx packets inf                    #
        # returen rx and tx inf                               #
        #######################################################
        process.sendline('stat show portlist = %s,%s'%(port1,port2))
        process.expect('CLX#')
        show_stat_str= process.before.encode()
        self.logger.log_info(show_stat_str, True)
        # get counter's data
        show_stat_str= show_stat_str.decode()
        echo = show_stat_str.split("\r\n")
        portlist=[]
        rx_drop=[]
        rx_octets=[]
        rx_uc_pks=[]
        rx_err=[]
        tx_octets=[]
        tx_drop=[]
        tx_uc_pks=[]
        tx_err=[]
        for j in range(2):
            portlist.append(echo[j*16+1])
            rx_octets.append((echo[j*16+2].split())[3])
            rx_uc_pks.append((echo[j*16+3].split())[4])
            rx_drop.append((echo[j*16+7].split())[4])
            rx_err.append( (echo[j*16+8].split())[4])
            tx_octets.append( (echo[j*16+9].split())[3])
            tx_uc_pks.append((echo[j*16+10].split())[4])
            tx_drop.append( ( echo[j*16+14].split())[4])
            tx_err.append( (echo[j*16+15].split())[4])
            a=','
            if a in rx_drop[j]:
                rx_drop[j]=rx_drop[j].replace(a,'')
            else:
                pass
        stat_ports = [
                    ('portlist',portlist[0],portlist[1]),
                    ('rx_octets',rx_octets[0],rx_octets[1]),
                    ('rx_uc_pks',rx_uc_pks[0],rx_uc_pks[1]),
                    ('rx_drop',rx_drop[0],rx_drop[1]),
                    ('rx_err',rx_err[0],rx_err[1]),
                    ('tx_octets',tx_octets[0],tx_octets[1]),
                    ('tx_uc_pks',tx_uc_pks[0],tx_uc_pks[1]),
                    ('tx_drop',tx_drop[0],tx_drop[1]),
                    ('tx_err',tx_err[0],tx_err[1])
                    ]
        return(stat_ports)

    def parse_port_stat(self, process):
        ret = E.OK
        i = 1
        while i < self.total_portnum+1:
            if i<= self.qsfp_portnum:
                port1 = (i-1)*self.port_step
                port2 = i*self.port_step
            else:
                port1 = (i-(self.qsfp_portnum+1))*8+192
                port2 = (i-(self.qsfp_portnum+1))*8+200
            # stat show
            stat_ports=self.clx_show_stat_ports(process,port1,port2)
            # judge packet loss
            if(stat_ports[1][1]!=stat_ports[5][2] or stat_ports[1][2]!=stat_ports[5][1] or
                stat_ports[1][1] == 0 or stat_ports[5][2] == 0 or
                stat_ports[1][2] == 0 or stat_ports[5][1] == 0):
                ret = E.EFAIL
                self.logger.log_err('stat_ports: {}'.format(stat_ports), True)
                self.fail_reason.append("snake {} {} pkt count fail.".format(stat_ports[0][1], stat_ports[0][2]))

            i = i +2

        time.sleep(2)
        #exit CLX Module
        process.sendline('exit')
        return ret

    def snake_test(self, also_print_console=False):
        ret = E.EFAIL
        #curdate = time.strftime('%Y-%m-%d %H:%M:%S',time.localtime())
        #print("start time: ", curdate)
        ret = self.init_sdk(also_print_console)
        self.show_sdk_verion(self.sdk_process)
        self.init_port_list()
        # clear packet counters
        self.sdk_clear_packet_counter(self.sdk_process)
        traffic_test_path = self.platform_path + "/traffic_test_dsh/"
        # send packets
        for script in self.sdk_traffic_test_dsh:
            script_path = traffic_test_path+script
            self.logger.log_info('diag load script: {} '.format(script_path), also_print_console)
            ret = self.sdk_diag_load_command(self.sdk_process, script_path)
            if ret != E.OK:
                self.logger.log_err('diag load script: {} fail'.format(
                                                                script_path), also_print_console)                                            
                return ret          
        ret = self.parse_port_stat(self.sdk_process)

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
            ret = self.snake_test(True)
            if ret == E.OK:
                pass_cnt += 1
            else:
                fail_cnt += 1
                self.logger.log_err("snake test loop {} failed!".format(i), True)
                break

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
