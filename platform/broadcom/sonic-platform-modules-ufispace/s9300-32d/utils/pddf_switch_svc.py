#!/usr/bin/env python
# Script to stop and start the respective platforms default services. 
# This will be used while switching the pddf->non-pddf mode and vice versa
import commands

def check_pddf_support():
    return True

def stop_platform_svc():
    
    '''
    status, output = commands.getstatusoutput("systemctl stop s9300-32d-platform-monitor-fan.service")
    if status:
        print "Stop s9300-32d-platform-fan.service failed %d"%status
        return False
    
    status, output = commands.getstatusoutput("systemctl stop s9300-32d-platform-monitor-psu.service")
    if status:
        print "Stop s9300-32d-platform-psu.service failed %d"%status
        return False
    
    status, output = commands.getstatusoutput("systemctl stop s9300-32d-platform-monitor.service")
    if status:
        print "Stop s9300-32d-platform-init.service failed %d"%status
        return False
    status, output = commands.getstatusoutput("systemctl disable s9300-32d-platform-monitor.service")
    if status:
        print "Disable s9300-32d-platform-monitor.service failed %d"%status
        return False
    '''
    
    status, output = commands.getstatusoutput("/usr/local/bin/platform_utility.py deinit")
    if status:
        print "platform_utility.py deinit command failed %d"%status
        return False

    # HACK , stop the pddf-platform-init service if it is active
    status, output = commands.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print "Stop pddf-platform-init.service along with other platform serives failed %d"%status
        return False

    return True
    
def start_platform_svc():
    
    status, output = commands.getstatusoutput("/usr/local/bin/platform_utility.py init")
    if status:
        print "platform_utility.py init command failed %d"%status
        return False

    '''
    status, output = commands.getstatusoutput("systemctl enable s9300-32d-platform-monitor.service")
    if status:
        print "Enable s9300-32d-platform-monitor.service failed %d"%status
        return False
    status, output = commands.getstatusoutput("systemctl start s9300-32d-platform-monitor-fan.service")
    if status:
        print "Start s9300-32d-platform-monitor-fan.service failed %d"%status
        return False
        
    status, output = commands.getstatusoutput("systemctl start s9300-32d-platform-monitor-psu.service")
    if status:
        print "Start s9300-32d-platform-monitor-psu.service failed %d"%status
        return False
    '''
    return True

def start_platform_pddf():   

    status, output = commands.getstatusoutput("systemctl start pddf-platform-init.service")
    if status:
        print "Start pddf-platform-init.service failed %d"%status
        return False
    
    return True

def stop_platform_pddf():   

    status, output = commands.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print "Stop pddf-platform-init.service failed %d"%status
        return False

    return True

