#!/usr/bin/env python

import os, sys

CAMEO_PORT_CFG = '/usr/share/sonic/device/x86_64-cameo_esc600_128q-r0/esc600-128q/port_config.ini'
# WARNING_FILE is used to notify user in login banner
WARNING_FILE = '/tmp/slotcheckfail'

WARNING_STR1 = "****************************** WARNING *******************************"
WARNING_STR2 = "* Current port_config.ini does not match with physical configuration *"
WARNING_STR3 = "**********************************************************************"

MAX_SLOT_NUM = 8
SLOT_BUS_BASE = 1

def get_attr_value(attr_path):
    retval = 'ERR'
    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        print("Unable to open ", attr_path, " file !")
        return retval
    
    retval = retval.rstrip('\r\n')
    return retval


def main():

    current_portcfg_list = set()
    current_slotphy_list = set()

    # record the current port list
    try:
        with open(CAMEO_PORT_CFG, 'r') as fd:
            lines = fd.readlines()
                
    except Exception as error:
        print("Unable to open ", CAMEO_PORT_CFG, " file !")
        sys.exit(1)
    
    for l in lines[1:]:
        split_l = l.split(' ')
        current_portcfg_list.add(split_l[0])
        
    # record all slot presence
    for slotn in range(0,MAX_SLOT_NUM):
        filepath = "/sys/bus/i2c/devices/{}-0032/portnum".format(SLOT_BUS_BASE+slotn)
        if os.path.exists(filepath):
            portnum = get_attr_value(filepath)
            if portnum == 'ERR':
                print("{} Error: Read {} error".format(__file__, filepath))
                sys.exit(1)
            
            portnum = int(portnum)
            
            for pn in range(0, portnum):
                current_slotphy_list.add("Ethernet{0}-{1}".format(slotn+1, pn+1))
                
    # not match
    if(current_portcfg_list != current_slotphy_list):
        if os.path.exists(WARNING_FILE) is False:
            fd = open(WARNING_FILE, "w")
            fd.close()
        print(WARNING_STR1+'\n'+WARNING_STR2+'\n'+WARNING_STR3)
        print('curret portcfg:\n{}'.format(current_portcfg_list))
        print('physical cfg:\n{}'.format(current_slotphy_list))
        sys.exit(2)
    # match
    else:
        if os.path.exists(WARNING_FILE):
            os.remove(WARNING_FILE)
    
    # match without error
    sys.exit(0)


if __name__ == "__main__":
    main()

