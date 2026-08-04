#!/usr/bin/python

import os
import sys
import logging

FAN_NUM = 5
sensors_path = '/sys/bus/i2c/devices/5-0070/'
sensors_nodes = {'fan_rpm': ['_inner_rpm', '_outer_rpm'],
                 'fan_vol': ['ADC8_vol', 'ADC7_vol','ADC6_vol', 'ADC5_vol','ADC4_vol', 'ADC3_vol'],
                 'temp':['lm75_49_temp', 'lm75_48_temp', 'SA56004_local_temp','SA56004_remote_temp']}
sensors_type = {'fan_rpm': ['Inner RPM', 'Outer RPM'],
                'fan_vol': ['P0.2', 'P0.6','P0.1', 'P1.5','P0.7', 'P1.6'],
                'temp':['lm75_49_temp', 'lm75_48_temp', 'SA56004_local_temp','SA56004_remote_temp']}

# Get sysfs attribute
def get_attr_value(attr_path):
    retval = 'ERR'        
    if (not os.path.isfile(attr_path)):
        return retval

    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", attr_path, " file !")

    retval = retval.rstrip('\r\n')
    fd.close()
    return retval

def get_fan_status(number):
    attr_value = get_attr_value(sensors_path + "fan" + str(number+1) + "_present")
    if (attr_value != 'ERR'):
        attr_value = int(attr_value, 16)

        if(attr_value == 0):
            string = "Connect"
        else:
            string = "Disconnect"
    return string

def get_fan_alert(number):
    attr_value = get_attr_value(sensors_path + "fan" + str(number+1) + "_status_alert")
    if (attr_value != 'ERR'):
        attr_value = int(attr_value, 16)

        if(attr_value == 0):
            string = "Normal"
        else:
            string = "Abnormal"
    return string

def get_fan_inner_rpm(number):
    return get_attr_value(sensors_path + "fan" + str(number+1) + "_inner_rpm")

def get_fan_outer_rpm(number):
    return get_attr_value(sensors_path + "fan" + str(number+1) + "_outer_rpm")

def get_fan():
    for i in range(0,FAN_NUM):
        print " "
        #status
        string = get_fan_status(i)
        print "FAN " + str(i+1) + ":" + ' ' + string
        if string=='Disconnect':
            continue
            
        #alert
        string = get_fan_alert(i)
        print "     Status:"+ ' ' + string

        #inner rpm
        string = get_fan_inner_rpm(i)
        print "  Inner RPM:"+ string.rjust(10) + ' RPM'

        #outer rpm
        string = get_fan_outer_rpm(i)
        print "  Outer RPM:"+ string.rjust(10) + ' RPM'

    return

def get_hwmon():
    print " "
    string = get_attr_value(sensors_path + "lm75_48_temp")
    print "Sensor A: " + string + " C"

    string = get_attr_value(sensors_path + "lm75_49_temp")
    print "Sensor B: " + string + " C"

    return

def get_voltage():
    print " "
    nodes = sensors_nodes['fan_vol']
    types = sensors_type['fan_vol']
    for i in range(0,len(nodes)):
        string = get_attr_value(sensors_path + nodes[i])
        print types[i] + ': ' + string + " V"

    return

def init_fan():
    return

def main():
    """
    Usage: %(scriptName)s command object

    command:
        install     : install drivers and generate related sysfs nodes
        clean       : uninstall drivers and remove related sysfs nodes  
        show        : show all systen status
        set         : change board setting with fan|led|sfp    
    """

    if len(sys.argv)<2:
        print main.__doc__

    for arg in sys.argv[1:]:           
        if arg == 'fan_init':
            init_fan()
        elif arg == 'get_sensors':
            ver = get_attr_value(sensors_path + "fb_hw_version")
            print 'HW Version: ' + ver
            ver = get_attr_value(sensors_path + "fb_fw_version")
            print 'SW Version: ' + ver
            get_fan()
            get_hwmon()
            get_voltage()                      
        elif arg == 'fan_set':
            if len(sys.argv[1:])<1:
                print main.__doc__
            else:
                set_fan(sys.argv[1:])                
            return            
        else:
            print main.__doc__

if __name__ == "__main__":
    main()
