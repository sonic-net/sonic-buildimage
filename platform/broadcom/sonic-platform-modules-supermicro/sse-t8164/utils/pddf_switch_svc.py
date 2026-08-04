#!/usr/bin/env python3
# Script to stop and start the respective platforms default services.
# This will be used while switching the pddf->non-pddf mode and vice versa
import os
import sys
import subprocess

def check_pddf_support():
    return True

def stop_platform_svc():
    status, output = subprocess.getstatusoutput("systemctl stop sse-t8164-platform-init.service")
    if status:
        print "Stop sse-t8164-platform-init.service failed %d"%status
        return False
    status, output = subprocess.getstatusoutput("systemctl disable sse-t8164-platform-init.service")
    if status:
        print "Disable sse-t8164-platform-init.service failed %d"%status
        return False

    status, output = subprocess.getstatusoutput("/usr/local/bin/sse-t8164_util.py clean")
    if status:
        print "sse-t8164_util.py clean command failed %d"%status
        return False

    # HACK , stop the pddf-platform-init service if it is active
    status, output = subprocess.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print "Stop pddf-platform-init.service along with other platform serives failed %d"%status
        return False

    return True

def start_platform_svc():
    status, output = subprocess.getstatusoutput("/usr/local/bin/sse-t8164_util.py install")
    if status:
        print "sse-t8164_util.py install command failed %d"%status
        return False

    status, output = subprocess.getstatusoutput("systemctl enable sse-t8164-platform-init.service")
    if status:
        print "Enable sse-t8164-platform-init.service failed %d"%status
        return False
    status, output = subprocess.getstatusoutput("systemctl start sse-t8164-platform-init.service")
    if status:
        print "Start sse-t8164-platform-init.service failed %d"%status
        return False

    return True

def start_platform_pddf():
    # Enable PDDF 2.0 object class for sse-t8164
    status, output = subprocess.getstatusoutput("mkdir /usr/share/sonic/platform/sonic_platform")
    if status:
        print "Unable to create 2.0 object class folder /usr/share/sonic/platform/sonic_platform"
        return False

    status, output = subprocess.getstatusoutput("systemctl start pddf-platform-init.service")
    if status:
        print "Start pddf-platform-init.service failed %d"%status
        return False

    return True

def stop_platform_pddf():
    # Disable PDDF 2.0 object class for sse-t8164
    status, output = subprocess.getstatusoutput("rm -r /usr/share/sonic/platform/sonic_platform")
    if status:
        print "Unable to delete 2.0 object class folder /usr/share/sonic/platform/sonic_platform"
        return False

    status, output = subprocess.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print "Stop pddf-platform-init.service failed %d"%status
        return False

    return True

def main():
    pass

if __name__ == "__main__":
    main()

