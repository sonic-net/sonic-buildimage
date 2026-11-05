#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import os
import re
try:
    from sonic_platform import get_machine_info
    from sonic_platform import get_platform_info
except ImportError :
    try:
        from sonic_device_util import get_machine_info
        from sonic_device_util import get_platform_info
    except ImportError:
        from sonic_py_common import device_info
        def get_machine_info():
            print("get_machine_info is null")
            return False
        def get_platform_info(x):
            return device_info.get_platform()

def start():
    x = get_platform_info(get_machine_info())
    print (x)
    str = re.findall(r"-(.+?)_",x)
    print (str[0])
    if str[0] == 'micas':
        print ("Start privatenetwork.sh")
        os.system("/usr/local/bin/privatenetwork.sh start")
    else:
        print ("Not set private network.")
def stop():
    x = get_platform_info(get_machine_info())
    print (x)
    str = re.findall(r"-(.+?)_",x)
    print (str[0])
    if str[0] == 'micas':
        print ("Stop privatenetwork.sh")
        os.system("/usr/local/bin/privatenetwork.sh stop")
    else:
        print ("Not stop private network.")
def main():
    print (sys.argv[1])
    if sys.argv[1]=='start':
        start()
    elif sys.argv[1]=='stop':
        stop()
    else:
        print ("Error parameter!\nRequired parameters : start or stop.")
if __name__ == '__main__':
    main()
