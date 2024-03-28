#!/usr/bin/env python
# Script to stop and start the respective platforms default services. 
# This will be used while switching the pddf->non-pddf mode and vice versa

from sonic_py_common.general import getstatusoutput_noshell

def check_pddf_support():
    return True

def stop_platform_svc():
    
    status, output = getstatusoutput_noshell(["systemctl", "stop", "n8550-platform-monitor-fan.service"])
    if status:
        print("Stop n8550-platform-fan.service failed %d"%status)
        return False
    
    status, output = getstatusoutput_noshell(["systemctl", "stop", "n8550-platform-monitor-psu.service"])
    if status:
        print("Stop n8550-platform-psu.service failed %d"%status)
        return False
    
    status, output = getstatusoutput_noshell(["systemctl", "stop", "n8550-platform-monitor.service"])
    if status:
        print("Stop n8550-platform-init.service failed %d"%status)
        return False
    status, output = getstatusoutput_noshell(["systemctl", "disable", "n8550-platform-monitor.service"])
    if status:
        print("Disable n8550-platform-monitor.service failed %d"%status)
        return False
    
    status, output = getstatusoutput_noshell(["/usr/local/bin/fs_n8550_util.py", "clean"])
    if status:
        print("fs_n8550_util.py clean command failed %d"%status)
        return False

    # HACK , stop the pddf-platform-init service if it is active
    status, output = getstatusoutput_noshell(["systemctl", "stop", "pddf-platform-init.service"])
    if status:
        print("Stop pddf-platform-init.service along with other platform serives failed %d"%status)
        return False

    return True
    
def start_platform_svc():
    status, output = getstatusoutput_noshell(["/usr/local/bin/fs_n8550_util.py", "install"])
    if status:
        print("fs_n8550_util.py install command failed %d"%status)
        return False

    status, output = getstatusoutput_noshell(["systemctl", "enable", "n8550-platform-monitor.service"])
    if status:
        print("Enable n8550-platform-monitor.service failed %d"%status)
        return False
    status, output = getstatusoutput_noshell(["systemctl", "start", "n8550-platform-monitor-fan.service"])
    if status:
        print("Start n8550-platform-monitor-fan.service failed %d"%status)
        return False
        
    status, output = getstatusoutput_noshell(["systemctl", "start", "n8550-platform-monitor-psu.service"])
    if status:
        print("Start n8550-platform-monitor-psu.service failed %d"%status)
        return False

    return True

def start_platform_pddf():
    status, output = getstatusoutput_noshell(["systemctl", "start", "pddf-platform-init.service"])
    if status:
        print("Start pddf-platform-init.service failed %d"%status)
        return False
    
    return True

def stop_platform_pddf():
    status, output = getstatusoutput_noshell(["systemctl", "stop", "pddf-platform-init.service"])
    if status:
        print("Stop pddf-platform-init.service failed %d"%status)
        return False

    return True

def main():
    print("stop_platform_svc")
    stop_platform_svc()
    #print"start_platform_svc"
    #start_platform_svc()
    #print"start_platform_pddf"
    #start_platform_pddf()
    print("stop_platform_pddf")
    stop_platform_pddf()
    #pass

if __name__ == "__main__":
    main()

