#!/usr/bin/python3

'''
This script is for healthd user_defined_checkers.
'''

import subprocess
class TemperCheck(object):
    def showLCCStatus(self):
        print('LCC')
        # Get Product name
        ret1, productName = subprocess.getstatusoutput("ipmitool fru print 0| grep 'Product Part Number' |awk '{ print $5 }'")
        # Check the platform support LCC or not
        if 'LCC' in productName:
            ret2, leakStatus = subprocess.getstatusoutput('ipmitool raw 0x04 0x2d 0xEE | cut -c 2-3')
            if leakStatus == '01':
                print("{}:not OK".format('Liquid Leak'))
            elif leakStatus == '00':
                print("{}:OK".format('Liquid Leak'))
            else:
                return

if __name__ == "__main__":
    temperCheck = TemperCheck()
    temperCheck.showLCCStatus()
