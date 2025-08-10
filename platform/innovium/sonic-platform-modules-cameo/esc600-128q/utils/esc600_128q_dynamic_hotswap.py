#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
import sys
import logging
from multiprocessing import Process, Lock
import time
import subprocess
# import BaldEagleSdk_v2_18 as BE218

from importlib import import_module
BE218 = import_module('esc600-128q.BaldEagleSdk_v2_18')

gMaxThreadNum=4
lock1 = Lock()
lock2 = Lock()

LOGLEVEL = os.environ.get('LOGLEVEL', 'INFO').upper()
LOGGING_FORMAT = '%(asctime)s %(levelname)s: %(message)s'
DATE_FORMAT = '%Y%m%d %H:%M:%S'
logging.basicConfig(level=LOGLEVEL, format=LOGGING_FORMAT, datefmt=DATE_FORMAT)
version = '1.0'
    
def run_command(command):  
    p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    out, err = p.communicate()
    return out.rstrip('\n')
    
def set_attr_value(attr_path,input_val):
    cmd=''
    if not os.path.isfile(attr_path):
        return
    cmd = "echo %d > %s" %(input_val,attr_path)
    logging.debug(cmd)
    run_command(cmd)
	
def get_attr_value(attr_path):
    retval = 'ERR'
    if not os.path.isfile(attr_path):
        return retval
    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", attr_path, " file !")

    retval = retval.rstrip('\r\n')
    fd.close()
    return retval
	
def card_type_detect(card):
    
    sup_card_typed = ['Inphi 100G','Credo 100G','Inphi 400G','Credo 400G']  
    
    filepath = "/sys/bus/i2c/devices/%d-0032/model" %(card)
    logging.debug(filepath)
    if os.path.exists(filepath):
        with open(filepath) as fh:
            for line in fh:
                get_str = line.strip()                    
                for i in range(0,4):
                    val = get_str.find(sup_card_typed[i]);                        
                    if val != -1:
                        return sup_card_typed[i]

                    
    else:
        return 'NA'
    return 'NA'

def card_reset(card):
    lock1.acquire()
    filepath = "/sys/bus/i2c/devices/%d-0032/phy_reset" % card
    
    try:
        if os.path.exists(filepath):
            set_attr_value(filepath,0)
            time.sleep(1) 
            set_attr_value(filepath,1)
            time.sleep(1) 
        else:
            return False;
    finally:        
        lock1.release()            
    return True;
    
def card_power_isgood(card):
    filepath = "/sys/class/hwmon/hwmon2/device/ESC600_SYS/module_%d_power" % card
    logging.debug(filepath)
    if os.path.exists(filepath):
        val = get_attr_value(filepath)
        if val == '1':
            return True, True
        else:
            return True, False
    return False, False

def card_12v_status(card):
    filepath = "/sys/class/hwmon/hwmon2/device/ESC600_SYS/module_%d_power" % card
    logging.debug(filepath)
    if os.path.exists(filepath):
        val = get_attr_value(filepath)
        if val == '1':
            return True, True
        else:
            return True, False
    return False, False   

def light_led(act,card):    
    led_loc = ['led_4_1', 'led_4_2', 'led_4_3', 'led_4_4', 'led_5_1','led_5_2','led_5_3','led_5_4']    
    value = 0
    lock2.acquire()
    filepath = '/sys/class/hwmon/hwmon2/device/ESC600_LED/%s'%led_loc[card-1]
    logging.debug(filepath)    
    try:
        if os.path.exists(filepath):
            if act=='OFF':
                logging.info("Setting LED to %s card [%d]" %(act,card))
                value = 0                
            elif act=='GREEN_SOLID':
                logging.info("Setting LED to %s card [%d]" %(act,card))
                value = 3                         
            elif act=='GREEN_BLINK':
                logging.info("Setting LED to %s card [%d]" %(act,card))
                value = 4                
            elif act=='AMBER_SOLID':
                logging.info("Setting LED to %s card [%d]" %(act,card))
                value = 1                
            elif act=='AMBER_BLINK':
                logging.info("Setting LED to %s card [%d]" %(act,card))
                value = 2                
            
            set_attr_value(filepath,value)
    finally:        
        lock2.release()
        
def card_inserted(card):            
    filepath = "/sys/class/hwmon/hwmon2/device/ESC600_SYS/module_%d_present" % card
    logging.debug(filepath)
    if os.path.exists(filepath):
        val = get_attr_value(filepath)
        if val == '1':
            return True, True
        else:
            return True, False
    return False, False
        
def job(mdio):           
    if mdio < 0 or mdio > 3:
        logging.error("MDIO [%d] out of range" % mdio) 
        return False
    CardStatus = ['NA','NA']
    FirstInitFlag = True
    while(1):   
        for i in range(2):
            card = mdio*2+i+1  
            status_12v = False
            status_powergood = False
            status_insert = False
            get_12v = False
            get_powergood = False
            get_insert = False   
            get_12v,status_12v = card_12v_status(card)
            get_powergood,status_powergood = card_power_isgood(card)
            get_insert,status_insert = card_inserted(card)
                        
            if get_12v == False:
                logging.error("Card [%d] Fail to get 12v status " % card)                
                CardStatus[i] = 'Driver Get Failed'
                continue
            if get_powergood == False:
                logging.error("Card [%d] Fail to get power good status "  % card)
                CardStatus[i] = 'Driver Get Failed'
                continue
            if get_insert == False: 
                logging.error("Card [%d] Fail to get insert status "  % card)            
                CardStatus[i] = 'Driver Get Failed' 
                continue
                
            if status_12v == True:
                if status_powergood == True and status_insert == True:                
                    if CardStatus[i]=='Initialize Complete' or CardStatus[i]=='Initialize Failed':
                        logging.info("Card [%d] status keep [%s] " % (card,CardStatus[i]))
                        time.sleep(1)
                        continue
                    else:                   
                        if CardStatus[i] != 'Initializing':
                            light_led('AMBER_BLINK',card)                        
                            logging.info( "Card [%d] status changed [%s] -> [Initializing] " % (card,CardStatus[i]))
                            CardStatus[i]='Initializing'                
                else: #Power on but no card inserted, set LED off
                    if CardStatus[i] != 'Power on Only':
                        logging.info( "Card [%d] status changed [%s] -> [Power on Only] " % (card,CardStatus[i]))
                        CardStatus[i]='Power on Only'
                        light_led('OFF',card)
                    
            else: #Power off, remove/install card availible
                if CardStatus[i] != 'Power Off':
                    light_led('GREEN_BLINK',card)                    
                    logging.info( "Card [%d] status changed [%s] -> [Power Off] " % (card,CardStatus[i]))
                    CardStatus[i]='Power Off'

        for i in range(2):
            card = mdio*2+i+1
            if CardStatus[i]=='Initializing' :
                if FirstInitFlag:
                    BE218.cameo_fw_load_2_card(card)
                    FirstInitFlag = False
                init_status = False                
                card_type = card_type_detect(card)
                if card_type == 'Credo 100G':                    
                    card_reset(card)
                    init_status = BE218.cameo_main_100G(card)
                    time.sleep(1)                     
                elif card_type == 'Credo 400G':                    
                    card_reset(card)
                    init_status = BE218.cameo_main_400G(card)
                    time.sleep(1)                     
                logging.info( "Card [%d] [%s] %s" %(card,CardStatus[i],card_type) )
                if init_status==True:                            
                    logging.info( "Card [%d] status changed [%s] -> [Initialize Complete] " % (card,CardStatus[i]))
                    CardStatus[i]='Initialize Complete'
                    light_led('GREEN_SOLID',card)
                else:
                    logging.info( "Card [%d] status changed [%s] -> [Initialize Failed] " % (card,CardStatus[i]))
                    CardStatus[i]='Initialize Failed'
                    light_led('AMBER_SOLID',card)
        time.sleep(2) 
        #break;
def module_loaded(module_name):
    """Checks if module is loaded"""
    lsmod_proc = subprocess.Popen(['lsmod'], stdout=subprocess.PIPE)
    grep_proc = subprocess.Popen(['grep', module_name], stdin=lsmod_proc.stdout)
    grep_proc.communicate()  # Block until finished
    return grep_proc.returncode == 0

def main():
    logging.info( "Cameo dynamic hotswap main task (%s) start. " % version ) 
    
    """Checks if lscpcie2 module is loaded"""
    module_name = 'lscpcie2'
    loaded = module_loaded(module_name)
    logging.error ('Module {} {} loaded'.format(module_name, "is" if loaded else "isn't"))    
    if not loaded:
        logging.error( "Cameo dynamic hotswap main task (%s) exit. " % version )
        sys.exit()    
    
    """Ready to create processes"""
    numList = []
    for i in range(gMaxThreadNum):
        p = Process(target=job, args=(i,))
        numList.append(p)
        logging.info( "Prcess (%d) start ." % i )
        p.start()
    for i in range(gMaxThreadNum):
        numList[i].join()
        logging.info( "Cameo dynamic hotswap main task (%s) exit(process %d) " % (version,i ))
        

if __name__ == '__main__':    
    main()