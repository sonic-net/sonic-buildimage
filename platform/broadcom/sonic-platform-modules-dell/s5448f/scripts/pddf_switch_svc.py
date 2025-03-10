#!/usr/bin/env python
# Script to stop and start the respective platforms default services. 
# This will be used while switching the pddf->non-pddf mode and vice versa
import os
import sys
import subprocess

def check_pddf_support():
    return True

def stop_platform_svc():
    status, output = subprocess.getstatusoutput("systemctl stop platform-modules-s5448f.service")
    if status:
        print("Stop platform-modules-s5448f.service failed %d"%status)
        return False
    status, output = subprocess.getstatusoutput("systemctl disable platform-modules-s5448f.service")
    if status:
        print("Disable platform-modules-s5448f.service failed %d"%status)
        return False

    # HACK , stop the pddf-platform-init service if it is active
    status, output = subprocess.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print("Stop pddf-platform-init.service along with other platform serives failed %d"%status)
        return False

    return True

def start_platform_svc():
    status, output = subprocess.getstatusoutput("systemctl enable platform-modules-s5448f.service")
    if status:
        print("Enable platform-modules-s5448f.service failed %d"%status)
        return False
    status, output = subprocess.getstatusoutput("systemctl start platform-modules-s5448f.service")
    if status:
        print("Start platform-modules-s5448f.service failed %d"%status)
        return False

    return True

def start_platform_pddf():
    status, output = subprocess.getstatusoutput("systemctl enable pddf-platform-init.service")
    if status:
        print("Enable pddf-platform-init.service failed %d"%status)
        return False
    status, output = subprocess.getstatusoutput("systemctl start pddf-platform-init.service")
    if status:
        print("Start pddf-platform-init.service failed %d"%status)
        return False

    return True

def stop_platform_pddf():

    status, output = subprocess.getstatusoutput("systemctl stop pddf-platform-init.service")
    if status:
        print("Stop pddf-platform-init.service failed %d"%status)
        return False
    status, output = subprocess.getstatusoutput("systemctl disable pddf-platform-init.service")
    if status:
        print("Disable pddf-platform-init.service failed %d"%status)
        return False

    return True

def main():
    pass

if __name__ == "__main__":
    main()
