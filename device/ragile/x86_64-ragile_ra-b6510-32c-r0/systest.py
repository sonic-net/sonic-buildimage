#!/usr/bin/python
# -*- coding: UTF-8 -*-

#   * onboard interval check
#   * FAN trays
#   * PSU
#   * temp
import time
import datetime
from monitor import status

def doWork():
    a=[];
    '''
    return: [{'status': '1', 'hw_version': '1.00', 'errcode': 0, 'fan_type': 'M6510-FAN-F', 'errmsg': 'OK', 'Speed': '9778', 'id': 'fan1', 'present': '0', 'sn': '1000000000014'},
                {'id': 'fan2', 'errmsg': 'not present', 'errcode': -1},
                {'id': 'fan3', 'errmsg': 'not present', 'errcode': -1},
                {'id': 'fan4', 'errmsg': 'not present', 'errcode': -1}
                ]
    description:  1.get id
                  2.errcode     equal 0 : dev normal
                            not equal 0 : get errmsg
                  3.other message add when all check success
    '''
    status.checkFan(a)
    #status.getTemp(a)
    #status.getPsu(a)

    nowTime=datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print nowTime
    print a
def run(interval):
    while True:
        try:
            time_remaining = interval - time.time()%interval
            time.sleep(time_remaining)
            doWork()
        except Exception as e:
            print(e)

if __name__ == '__main__':
    interval = 1
    run(interval)
