#!/usr/bin/env python
#
# Copyright (C) 2018 Inventec, Inc.
# 
# Editor: James Huang ( Huang.James@inventec.com )
#  
# 

"""
Usage: %(scriptName)s [options] command object

Auto detecting the transceiver and set the correct if_type value

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
   
"""

try:
    import os
    import commands
    import sys, getopt
    import logging
    import re
    import time
    import datetime
    import syslog
    from sfputil import SfpUtil
    from sonic_sfp.bcmshell import bcmshell
    from sonic_py_common.general import getstatusoutput_noshell
    
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

DEBUG = False
args = []
INV_REDWOOD_PLATFORM = "SONiC-Inventec-d7032-100"
INV_CYPRESS_PLATFORM = "SONiC-Inventec-d7054"
INV_SEQUOIA_PLATFORM = "SONiC-Inventec-d7264"
INV_MAPLE_PLATFORM = "SONiC-Inventec-d6556"
INV_MAGNOLIA_PLATFORM = "SONiC-Inventec-d6254qs"

transceiver_type_dict = { 
                          "FCBG110SD1C03": "SR",
                          "FCBG110SD1C05": "SR",
                          "FTLX8571D3BCL": "SR",
                          "FTLX8574D3BCL": "SR",
                          "AFBR-709DMZ": "SR",
                          "AFBR-709SMZ": "SR",
                          "FTLX8571D3BCV": "SR",
                          "FTLX1471D3BCL": "SR",
                          "FTLX1871M3BCL": "SR",
                          "FTLF8536P4BCL": "SR",
                          "FCBG125SD1C05": "SR",
                          "FCBG125SD1C30": "SR",
                          "FCBG125SD1C03": "SR",
                          "FCBG410QB1C03-1E": "SR4",
                          "FCBG4100QB1C030-1E": "SR4",
                          "885350163": "SR4",
                          "88535017": "SR4",
                          "FTL410QE2C": "SR4",
                          "FTL410QD3C": "SR4",
                          "FTL410QD2C": "SR4",
                          "AFBR-79E3PZ": "SR4",
                          "AFBR-79Q4Z": "SR4",
                          "FTL4C1QE1C": "SR4",
                          "FTLC9551REPM": "SR4",
                          "FTLC1151RDPL": "SR4",
                          "DAC-010SS-X50" : "KR",
                          "DAC-010QQ-X50": "KR4",
                          "DAC-040QS-007": "KR4",
                          "DAC-040QQ-007": "KR4",
                          "DAC-040QQ-005": "KR4",
                          "DAC-040QS-005": "KR4",
                          "NDAAFF-0001": "KR4",
                          "L0HQF001-SD-R": "KR4",
                          "DAC-Q28/Q28-28-01": "KR4",
                          "NDAAFF-0003": "KR4",
                          "NDAQGF0001": "KR4",
                          "L0HQF003-SD-R": "KR4",
                          "NDAQGJ-0003": "KR4",
                          "L0HQF004-SD-R": "KR4",
                          "L0HSF006-SD-R": "KR",
                          "L0HSF007-SD-R": "KR",
                          "L0HSF008-SD-R": "KR",
                          "L0HQF009-SD-R": "KR4",
                          "FSPP-H7-M85-X3D": "SR",   
                          "PT0-M3-4D33K-C2": "SR",
                          "RTXM228-551": "SR",
                          "RTXM330-003": "SR",
                          "RTXM330-030": "SR",   
                          "MFA2P10-A005": "SR",
                          "QAB-OA03MC": "SR4",
                          "QAB-OA05MC": "SR4",
                          "RTXM320-571": "SR4",
                          "AFBR-89CDDZ": "SR4",
                          "RTXM420-550": "SR4",
                          "MMA1B00-C100D": "SR4",
                          "RTXM420-551": "SR4",
                          "E04025QTXA000": "SR4",
                          "LQ210PR-Oxxx": "SR4",
                          "TR-FC13L-N00": "SR4",  
                          "SPQ-CE-LR-CDFL": "SR4",
                          "FIM37700/170": "SR4",
                          "FCBN425QE1C03": "SR4",
                          "TQS-Q14H8-XCAXX": "SR4",
                          "FPD-203R008-10/3": "SR4",
                          "LTA8531-PC+": "SR4"                       
                        }
 
initial_command = []

def show_help():
    print __doc__ % {'scriptName' : sys.argv[0].split("/")[-1]}
    sys.exit(0)

def log_message( string ):
    syslog.openlog("transceiver_monitor", syslog.LOG_PID, facility=syslog.LOG_DAEMON)
    syslog.syslog(syslog.LOG_NOTICE, string)

class BCMUtil(bcmshell):

    port_to_bcm_mapping = dict()         
    sal_config_list = dict()
    eagle_list = []
    platform = None
    
    def get_platform(self):
        if self.platform is None:
            _, self.platform = getstatusoutput_noshell(["uname", "-n"])
        return self.platform
    
    def get_port_to_bcm_mapping(self):  
        if self.port_to_bcm_mapping is None:
            return dict()
        else:
            return self.port_to_bcm_mapping     
    
    def show_port_to_bcm_mapping(self): 
        for key,value in self.port_to_bcm_mapping.iteritems():
            print "{0}---{1}".format(key, value)    
    
    def get_eagle_port(self):
        return self.eagle_list
        
    def parsing_eagle_port(self):
        name = self.get_platform()
        if name is not None:
            if name == INV_REDWOOD_PLATFORM:
                self.eagle_list = [66,100]
            elif name == INV_CYPRESS_PLATFORM:
                self.eagle_list = [66,100]
            elif name == INV_SEQUOIA_PLATFORM:
                self.eagle_list = [66,100]
            elif name == INV_MAPLE_PLATFORM:
                self.eagle_list = [66,130]
            else:
                self.eagle_list = []
                
    def get_sal_config_list(self):
        return self.sal_config_list

    def show_sal_config_list(self):
        for key,value in self.sal_config_list.iteritems():
            print "{0}---{1}".format(key, value)
        
    def initial_sal_config_list( self ):
        content = self.run("config")  
        for line in content.split("\n"):
            ConfigObject = re.search(r"portmap\_(?P<bcm_id>\d+)\=(?P<lane_id>\d+)\:\d+",line)
            if ConfigObject is not None:   
                if int(ConfigObject.group("bcm_id")) not in self.get_eagle_port():
                    self.get_sal_config_list()[int(ConfigObject.group("bcm_id"))]={"lane": int(ConfigObject.group("lane_id")), "speed": None, "portname": None} 
                
    def parsing_port_list(self):
        content = self.run("ps")
        count = 0
        for line in content.split("\n"):
            PSObject = re.search(r"(?P<port_name>(xe|ce)\d+)\(\s*(?P<bcm_id>\d+)\).+\s+(?P<speed>\d+)G",line)
            if PSObject is not None:
                if int(PSObject.group("bcm_id")) not in self.get_eagle_port():                    
                    if self.get_sal_config_list().has_key(int(PSObject.group("bcm_id"))):
                        self.get_sal_config_list()[int(PSObject.group("bcm_id"))]["portname"] = PSObject.group("port_name")
                        self.get_sal_config_list()[int(PSObject.group("bcm_id"))]["speed"] = int(PSObject.group("speed"))*1000
                        self.port_to_bcm_mapping[count] = int(PSObject.group("bcm_id"))
                        count = count +1
                
    
    def execute_command(self, cmd):
        self.cmd(cmd)

class TransceiverUtil(SfpUtil):     
    
    transceiver_port_mapping = dict()
    
    def get_transceiver_port_mapping(self):
        return self.transceiver_port_mapping
        
    def show_transceiver_port_mapping(self):
        for key,value in self.transceiver_port_mapping.iteritems():
            print "{0}---{1}".format(key, value)     
       
    def get_bcm_port_name(self, index):
        if self.transceiver_port_mapping.has_key(index) and bcm_obj.get_sal_config_list().has_key(self.transceiver_port_mapping[index]["bcm"]):
            return bcm_obj.get_sal_config_list()[self.transceiver_port_mapping[index]["bcm"]]["portname"]            
        else:
            return ""
                
    def get_port_to_i2c_mapping(self):
        if self.port_to_i2c_mapping is None:
            return dict()
        else:
            return self.port_to_i2c_mapping
    
    def show_port_to_i2c_mapping(self):
        for key,value in self.port_to_i2c_mapping.iteritems():
            print "{0}---{1}".format(key, value)
        
    def get_eeprom_partNum(self, portNum):
        tempdict = dict()
        tempdict = self.get_eeprom_dict(portNum)
        self.get_eeprom_partNum_from_parser_eeprom_dict(tempdict)
    
    def get_eeprom_dict_info(self, portNum): 
        return self.get_eeprom_dict(portNum) 
                
    def get_eeprom_partNum_from_parser_eeprom_dict(self, tempdict ):
        if tempdict is not None:
            if tempdict["interface"]["data"].has_key("VendorPN"):
               return tempdict["interface"]["data"]["VendorPN"]
            elif tempdict["interface"]["data"].has_key("Vendor PN"):
                return tempdict["interface"]["data"]["Vendor PN"]
            else:
                return None
        else:
            return None
            
    def get_transceiver_type(self, pn ):
        if pn is not None:
            if transceiver_type_dict.has_key(pn.upper()):
                return transceiver_type_dict[pn.upper()]
            else:
                return None    

    def set_transceiver_type( self, portNum, pn ):
        type = self.get_transceiver_type( pn )
        if type is not None:             
            if bcm_obj.get_platform() == INV_SEQUOIA_PLATFORM or bcm_obj.get_platform() == INV_MAPLE_PLATFORM :
                speed = bcm_obj.get_sal_config_list()[self.transceiver_port_mapping[portNum]["bcm"]]["speed"]
                bcm_obj.execute_command( "port %s if=%s speed=%d" % ( self.get_bcm_port_name(portNum), type, speed ) )
            else:
                bcm_obj.execute_command( "port %s if=%s" % ( self.get_bcm_port_name(portNum), type ) )
            print "Detecting port {0}({1})  need to change interface type {2} ({3})".format( self.get_bcm_port_name(portNum), portNum, type, self.get_transceiver_port_mapping()[portNum]["pn"])
            log_message("Detecting port {0} need to change interface type {1} ({2})".format(self.get_bcm_port_name(portNum), type, self.get_transceiver_port_mapping()[portNum]["pn"]) )
    
    def initial_transceiver_port_mapping(self):
        for index in self.get_port_to_i2c_mapping().keys():
            if self.transceiver_port_mapping.has_key(index) is False :
                i2cValue = self.get_port_to_i2c_mapping()[index]
                bcmValue = bcm_obj.get_port_to_bcm_mapping()[index]
                self.transceiver_port_mapping[index]={"i2c": i2cValue, "bcm": bcmValue , "pn": None}
            
    def set_power_mode_for_QSFP(self):
        for index in self.get_port_to_i2c_mapping().keys():
            if index >= self.qsfp_port_start and index <= self.qsfp_port_end :
                self.set_low_power_mode(index, False)
            else:
                # To set tx_disable
                self.set_tx_disable(index)

    def set_tx_disable(self, port_num):
        if port_num >= self.qsfp_port_start and port_num <= self.qsfp_port_end :
            pass
        else:
            try:
                tx_file = open("/sys/class/swps/port"+str(port_num)+"/tx_disable", "r+")
            except IOError as e:
                print "Error: unable to open file: %s" % str(e)
                return False

            reg_value = int(tx_file.readline().rstrip())

            # always set 0 to tx_disable field
            if reg_value == 1 :
                reg_value = 0        
                tx_file.write(hex(reg_value))
                tx_file.close()

        
def main():

    global DEBUG  
    global transceiver_obj
    global bcm_obj
    
    initalNotOK = True
    retestCount = 0 
    while initalNotOK :
        try:                
            transceiver_obj = TransceiverUtil()
            bcm_obj = BCMUtil()
            initalNotOK = False
        except Exception, e:               
            log_message("Exception. The warning is {0}, Retry again ({1})".format(str(e),retestCount) )                    
            retestCount = retestCount + 1
        time.sleep(5)
     
    log_message( "Object initialed successfully" )  
    options, args = getopt.getopt(sys.argv[1:], 'hd', ['help',
                                                       'debug'
                                                          ])
    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):            
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        else:
            logging.info("no option")
    
    initalNotOK = True
    while initalNotOK :
        try :
            # Before loop, You could execute specific command to initial chip
            for cmd_index in initial_command :
                bcm_obj.execute_command(cmd_index)
            
            # Initial the sal config list
            bcm_obj.parsing_eagle_port()
            bcm_obj.initial_sal_config_list()
            # bcm_obj.show_sal_config_list()
            bcm_obj.parsing_port_list()                 
            #bcm_obj.show_port_to_bcm_mapping()                 
            #bcm_obj.show_sal_config_list()
            # transceiver_obj.show_port_to_i2c_mapping()
            
            # Initial the transceiver_obj 
            transceiver_obj.initial_transceiver_port_mapping()       
            # transceiver_obj.show_transceiver_port_mapping()
             
            initalNotOK = False
        except Exception, e:               
            log_message("Exception. The warning is {0}".format(str(e)) )
        time.sleep(5)            
    
    # Improve the power mode for QSFP ports
    transceiver_obj.set_power_mode_for_QSFP()

    while 1 :
        try:
            if bcm_obj.get_platform() == INV_SEQUOIA_PLATFORM:
                bcm_obj.parsing_port_list()  
            for index in transceiver_obj.get_port_to_i2c_mapping().keys():
                info = transceiver_obj.get_eeprom_dict_info(index)
                value = transceiver_obj.get_eeprom_partNum_from_parser_eeprom_dict(info)
                if transceiver_obj.get_transceiver_port_mapping().has_key(index) is not False and transceiver_obj.get_transceiver_port_mapping()[index]["pn"] <> value:
                    transceiver_obj.get_transceiver_port_mapping()[index]["pn"] = value
                    transceiver_obj.set_transceiver_type(index,value) 
                    transceiver_obj.set_tx_disable(index)
                    #transceiver_obj.show_transceiver_port_mapping()     
            # transceiver_obj.show_transceiver_port_mapping()       
        except Exception, e:
            log_message("Exception. The warning is {0}".format(str(e)) )            
        time.sleep(1)

    syslog.closelog()
    del transceiver_obj
    del bcm_obj

if __name__ == "__main__":
    main()







