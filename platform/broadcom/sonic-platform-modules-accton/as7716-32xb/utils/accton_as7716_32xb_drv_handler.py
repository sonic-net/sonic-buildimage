#!/usr/bin/env python
#
# Copyright (C) 2017 Accton Technology Corporation
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ------------------------------------------------------------------
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    1/18/2018: Jostar create for as7716_32xb
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import subprocess
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'as7716_32xb_drv_handler'
DEBUG = False

global log_file
global log_level

def my_log(txt):
    if DEBUG == True:
        print("[ACCTON DBG]: "+txt)
    return

def log_os_system(cmd, show):
    logging.info('Run :'+cmd)
    status = 1
    output = ""
    status, output = subprocess.getstatusoutput(cmd)
    if DEBUG == True:
        my_log (cmd +" , result:" + str(status))
    else:
        if show:
            print("ACC: " + str(cmd) + " , result:"+ str(status))
    #my_log ("cmd:" + cmd)
    #my_log ("      output:"+output)
    if status:
        logging.info('Failed :'+cmd)
        if show:
            print(('Failed :'+cmd))
    return  status, output

      
# Make a class we can use to capture stdout and sterr in the log
class accton_as7716xb_drv_handler(object):    
    
    QSFP_PORT_START = 1
    QSFP_PORT_END = 32
    BASE_PATH = "/usr/local/bin/"
    BASE_I2C_PATH="/sys/bus/i2c/devices/"
    QSFP_PRESENT_PATH = "/sys/bus/i2c/devices/0-0060/module_present_"
    QSFP_RESET_PATH = "/sys/bus/i2c/devices/0-0060/module_reset_"
    QSFP_PRESENT_FILE = "/tmp/ipmi_qsfp_pres"
    QSFP_EEPROM_FILE = "/tmp/ipmi_qsfp_ee_"
    THERMAL_FILE = "/tmp/ipmi_thermal"    
    IPMI_CMD_QSFP = "ipmitool raw 0x34 0x10 "
    IPMI_CMD_THERMAL = "ipmitool raw 0x34 0x12 "
    IPMI_CMD_FAN     = "ipmitool raw 0x34 0x14 "
    IPMI_CMD_PSU     ="ipmitool raw 0x34 0x16 "
    IPMI_CMD_SYS_EEPROM_1  ="ipmitool raw 0x34 0x18 0x0 0x80"
    IPMI_CMD_SYS_EEPROM_2  ="ipmitool raw 0x34 0x18 0x80 0x80"
    FAN_ID_START = 1
    FAN_ID_END = 6
    FAN_FILE = "/tmp/ipmi_fan"
    FAN_PATH = "/sys/bus/i2c/devices/0-0066/fan"
    PSU_ID_START = 1
    PSU_ID_END = 2
    PSU1_PATH = "/sys/bus/i2c/devices/0-0053/"
    PSU2_PATH = "/sys/bus/i2c/devices/0-0050/"
    PSU1_PMBUS_PATH = "/sys/bus/i2c/devices/0-005b/"
    PSU2_PMBUS_PATH = "/sys/bus/i2c/devices/0-0058/"
    PSU_FILE = "/tmp/ipmi_psu_"
    SYS_EEPROM_FILE_1 = "/tmp/ipmi_sys_eeprom_1"
    SYS_EEPROM_FILE_2 = "/tmp/ipmi_sys_eeprom_2"
    SYS_EEPROM_PATH = "/sys/bus/i2c/devices/0-0056/eeprom"
    

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def manage_ipmi_qsfp(self):        
        logging.debug ("drv hanlder-manage_ipmi_qsfp")
        print("drv hanlder")
        #Handle QSFP case
        ipmi_cmd = self.IPMI_CMD_QSFP + " 0x10 > " +self.QSFP_PRESENT_FILE
        log_os_system(ipmi_cmd, 0)
        file_path = self.QSFP_PRESENT_FILE
        check_file = open(file_path)
        try:
            check_file = open(file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))   
        line = check_file.readline()
        pres_line= line.rstrip().replace(" ","")
        while line:
            line = check_file.readline()            
            pres_line+=line.rstrip().replace(" ","")
        check_file.close() 
        
        
        for i in range(self.QSFP_PORT_START, self.QSFP_PORT_END+1, 1):
            if i>1:
                k=(i-1)*2 +1 
            else:
                k=1;
            if pres_line[k] == '1':
                #Set QSFP present
                #print "port-%d present" %i
                set_drv_cmd = "echo 1 > " + self.QSFP_PRESENT_PATH + str(i)
                log_os_system(set_drv_cmd, 0)
                #Read QSFP EEPROM
                ipmi_cmd = self.IPMI_CMD_QSFP +str(i)+ " 0x00 > " +self.QSFP_EEPROM_FILE + str(i) + "_1"
                log_os_system(ipmi_cmd, 0)
                ipmi_cmd = self.IPMI_CMD_QSFP +str(i)+ " 0x01 > " +self.QSFP_EEPROM_FILE + str(i) + "_2"
                log_os_system(ipmi_cmd, 0)
                file_path = self.QSFP_EEPROM_FILE + str(i) + "_1"
                check_file = open(file_path)
                try:
                    check_file = open(file_path)
                except IOError as e:
                    print("Error: unable to open file: %s" % str(e))
                line = check_file.readline()
                str_line= line.rstrip().replace(" ","")
                while line:
                    line = check_file.readline()            
                    str_line+=line.rstrip().replace(" ","")
                check_file.close()
                file_path = self.QSFP_EEPROM_FILE + str(i) + "_2"
                check_file = open(file_path)
                try:
                    check_file = open(file_path)
                except IOError as e:
                    print("Error: unable to open file: %s" % str(e))
                line = check_file.readline()
                str_line+= line.rstrip().replace(" ","")
                
                while line:
                    line = check_file.readline()
                    str_line+=line.rstrip().replace(" ","")
                check_file.close()
                #Set QSFP EEPROM
                if(i < 10):
                    set_drv_cmd = "echo " + str_line+ " > " + self.BASE_I2C_PATH + "0-000"+str(i) +"/eeprom"
                else:
                    set_drv_cmd = "echo " + str_line+ " > " + self.BASE_I2C_PATH + "0-00"+str(i) +"/eeprom"
                #print(set_drv_cmd)
                log_os_system(set_drv_cmd, 0)
            else:
                ipmi_cmd = "echo 0 > " + self.QSFP_PRESENT_PATH + str(i)
                log_os_system(ipmi_cmd, 0)
                if(i < 10):
                    set_drv_cmd = "echo 0 > " + self.BASE_I2C_PATH + "0-000"+str(i) +"/eeprom"
                else:
                    set_drv_cmd = "echo 0 > " + self.BASE_I2C_PATH + "0-00"+str(i) +"/eeprom"
                #print(set_drv_cmd)
                log_os_system(set_drv_cmd, 0)
                
            time.sleep(0.01) 
        return True
        
    def manage_ipmi_thermal(self):
        logging.debug ("drv hanlder-manage_ipmi_thermal")
        #Handle thermal case
        #ipmitool raw 0x34 0x12 
        ipmi_cmd = self.IPMI_CMD_THERMAL + " > " +self.THERMAL_FILE  
        log_os_system(ipmi_cmd, 0)
        file_path = self.THERMAL_FILE
        check_file = open(file_path)
        try:
            check_file = open(file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
        line = check_file.readline()
        str_line= line.rstrip().replace(" ","")
        while line:
            line = check_file.readline()            
            str_line+=line.rstrip().replace(" ","")
        check_file.close()       
        val_str= "0x" + str(str_line[4])+str(str_line[5])
        val_int=int(val_str, 16)*1000
        check_file.close()
        set_drv_cmd = "echo "+str(val_int) + " > " + self.BASE_I2C_PATH + "0-0048/temp1_input"
        log_os_system(set_drv_cmd,0)
        val_str= "0x" + str(str_line[10])+str(str_line[11])
        val_int=int(val_str, 16) * 1000
        set_drv_cmd = "echo "+str(val_int) + " > " + self.BASE_I2C_PATH + "0-0049/temp1_input"
        log_os_system(set_drv_cmd,0)
        val_str= "0x" + str(str_line[16])+str(str_line[17])
        val_int=int(val_str, 16) * 1000
        set_drv_cmd = "echo "+str(val_int) + " > " + self.BASE_I2C_PATH + "0-004a/temp1_input"
        log_os_system(set_drv_cmd, 0)
        
        return True
         
    def manage_ipmi_fan(self):
        logging.debug ("drv hanlder-manage_ipmi_fan")
        #Handle fan case
        #ipmitool raw  0x34 0x14
        ipmi_cmd = self.IPMI_CMD_FAN + " > " +self.FAN_FILE  
        log_os_system(ipmi_cmd, 0)
        file_path = self.FAN_FILE
        #print(file_path)
        check_file = open(file_path)
        try:
            check_file = open(file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))   
        line = check_file.readline()
        str_line= line.rstrip().replace(" ","")
        while line:
            line = check_file.readline()            
            str_line+=line.rstrip().replace(" ","")
        check_file.close()        
        #print (str_line)
        k=0
        for i in range(self.FAN_ID_START, self.FAN_ID_END+1, 1):
            #print "k=%d"%k
            if str_line[k+1]=='0':
                set_drv_cmd = "echo 1 > " + self.FAN_PATH + str(i) + "_present"
            else:
                set_drv_cmd = "echo 0 > " + self.FAN_PATH + str(i) + "_present"
            log_os_system(set_drv_cmd, 0)
        
            val_str= "0x" + str(str_line[k+6])+str(str_line[k+7]) + str(str_line[k+4])+str(str_line[k+5])
            val_int=int(val_str, 16)        
            set_drv_cmd = "echo " + str(val_int) + " > " + self.FAN_PATH + str(i) + "_front_speed_rpm"
            log_os_system(set_drv_cmd, 0)
            val_str= "0x" + str(str_line[k+54])+str(str_line[k+55]) + str(str_line[k+52])+str(str_line[k+53])
            val_int=int(val_str, 16)
            set_drv_cmd = "echo " + str(val_int) + " > " + self.FAN_PATH + str(i) + "_rear_speed_rpm"
            log_os_system(set_drv_cmd, 0)
            k+=8;
        return True
        
        
    def manage_ipmi_psu(self):
        logging.debug ("drv hanlder-manage_ipmi_psu")
        #Handle psu case
        #present: ipmitool raw  0x34 0x16 '0x1' .   Param-1 is psu id(id_1:0x1, id_2:0x2)
       
        #cpld access psu
        for i in range(self.PSU_ID_START, self.PSU_ID_END+1, 1):
            #present case
            ipmi_cmd = self.IPMI_CMD_PSU + str(i) + " > " +self.PSU_FILE + str(i)  
            log_os_system(ipmi_cmd, 0)
            if i==1:
               psu_sysfs_path = self.PSU1_PATH
            else:
               psu_sysfs_path = self.PSU2_PATH
            file_path = self.PSU_FILE + str(i)
            #print(file_path)
            check_file = open(file_path)
            try:
                check_file = open(file_path)
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))   
            line = check_file.readline()
            str_line= line.rstrip().replace(" ","")
            while line:
                line = check_file.readline()            
                str_line+=line.rstrip().replace(" ","")
            check_file.close()  
            #print (line)            
            if str_line[1]=='0': 
                int_val=1 #psu insert
                #print "psu_%d present"%i
                set_drv_cmd = "echo " +str(int_val) + " > " + psu_sysfs_path + "psu_present"
                log_os_system(set_drv_cmd, 0) 
                set_drv_cmd = "echo " + str(str_line[5]) + " > " + psu_sysfs_path + "psu_power_good"
                log_os_system(set_drv_cmd, 0)                 
            else:
                int_val=0
                set_drv_cmd = "echo " +str(int_val) + " > " + psu_sysfs_path + "psu_present"
                log_os_system(set_drv_cmd, 0)
                set_drv_cmd = "echo " + str(int_val) + " > " + psu_sysfs_path + "psu_power_good"
                log_os_system(set_drv_cmd, 0)      
               
        #pmbus
            if i==1:
               psu_sysfs_path = self.PSU1_PMBUS_PATH
            else:
               psu_sysfs_path = self.PSU2_PMBUS_PATH            
            
            if str_line[5]=='1': #power_on
                val_str= "0x" + str(str_line[28])+str(str_line[29]) + str(str_line[26])+str(str_line[27])
                val_int=int(val_str, 16) * 1000                
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_temp1_input"
                log_os_system(set_drv_cmd, 0)
                #print "val_int=%d"%val_int               
                val_str= "0x" + str(str_line[32])+str(str_line[33]) + str(str_line[30])+str(str_line[31])
                val_int=int(val_str, 16)
                #print "fan:val_int=%d"%val_int               
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_fan1_speed_rpm"
                log_os_system(set_drv_cmd, 0)
                val_str= "0x" + str(str_line[36])+str(str_line[37]) + str(str_line[34])+str(str_line[35])
                val_int=int(val_str, 16)
                #print "pout val_int=%d"%val_int               
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_p_out"
                log_os_system(set_drv_cmd, 0)
            else: #power_off
                val_int=0
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_temp1_input"
                log_os_system(set_drv_cmd, 0)
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_fan1_speed_rpm"
                log_os_system(set_drv_cmd, 0)
                set_drv_cmd = "echo " + str(val_int) + " > " + psu_sysfs_path + "psu_p_out"
                log_os_system(set_drv_cmd, 0)
         
        time.sleep(2)     
        return True
        
    def manage_ipmi_sys(self):
        logging.debug ("drv hanlder-manage_ipmi_sys")
        #Handle sys case
        #ipmitool -raw 0x34 0x18 0x00 0x80
        #ipmitool -raw 0x34 0x18 0x80 0x80
        
        ipmi_cmd = self.IPMI_CMD_SYS_EEPROM_1 + " > " + self.SYS_EEPROM_FILE_1
        log_os_system(ipmi_cmd, 0)        
        ipmi_cmd = self.IPMI_CMD_SYS_EEPROM_2 + " > " + self.SYS_EEPROM_FILE_2
        log_os_system(ipmi_cmd, 0)
        
        #Read SYS EEPROM
        file_path = self.SYS_EEPROM_FILE_1
        check_file = open(file_path)
        try:
            check_file = open(file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
        line = check_file.readline()
        str_line= line.rstrip().replace(" ","")
        while line:
            line = check_file.readline()            
            str_line+=line.rstrip().replace(" ","")
        check_file.close()
        
        file_path = self.SYS_EEPROM_FILE_2
        check_file = open(file_path)
        try:
            check_file = open(file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
        line = check_file.readline()
        str_line+= line.rstrip().replace(" ","")
        while line:
            line = check_file.readline()
            str_line+=line.rstrip().replace(" ","")
        check_file.close()
        #print(len(str_line))
        #print(str_line)
        set_drv_cmd = "echo " + str_line+ " > " + self.SYS_EEPROM_PATH
        #print(set_drv_cmd)
        log_os_system(set_drv_cmd, 0)
        
        return True  

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.WARNING
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdl:',['lfile='])
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg
                
    set_drv_cmd = "echo 100 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us"
    log_os_system(set_drv_cmd, 0) 
    monitor = accton_as7716xb_drv_handler(log_file, log_level)
   
    set_sys_eeprom=0
    thermal_chk_time=0
    psu_chk_time=0
    # Loop forever, doing something useful hopefully:
    while True:
        logging.debug ("monitor.manage_ipmi")
        if set_sys_eeprom==0:
            monitor.manage_ipmi_sys()
            set_sys_eeprom=1
        monitor.manage_ipmi_qsfp()
        time.sleep(0.1)        
        monitor.manage_ipmi_thermal()
        monitor.manage_ipmi_psu()
        time.sleep(0.1)        
        monitor.manage_ipmi_fan()
        time.sleep(0.05)        

if __name__ == '__main__':
    main(sys.argv[1:])
