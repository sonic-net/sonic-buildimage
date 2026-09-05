#!/usr/bin/python3
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#Script to read/write the portio based registers

import sys
import os
import getopt
import struct

resource = '/dev/port'

def usage():
    ''' This is the Usage Method '''

    print('\t\t portiocfg.py  --default')
    print('\t\t portiocfg.py  --get --offset <offset>')
    print('\t\t portiocfg.py --set --val <val>  --offset <offset>')
    sys.exit(1)

def portio_reg_read(resource, offset):
    fd = os.open(resource, os.O_RDONLY)
    if fd < 0:
        print('file open failed %s"%resource')
        return
    if os.lseek(fd, offset, os.SEEK_SET) != offset:
        print('lseek failed on %s'%resource)
        return
    buf = os.read(fd, 1)
    reg_val1 = ord(buf)
    print('reg value %x'%reg_val1)
    os.close(fd)

def portio_reg_write(resource, offset, val):
    fd = os.open(resource, os.O_RDWR)
    if fd < 0:
        print('file open failed %s"%resource')
        return
    if os.lseek(fd, offset, os.SEEK_SET) != offset:
        print('lseek failed on %s'%resource)
        return
    ret = os.write(fd, struct.pack('B', val))
    if ret != 1:
        print('write failed %d'%ret)
        return
    os.close(fd)

def main(argv):

    ''' The main function will read the user input from the
    command line argument and  process the request  '''

    opts = ''
    val = ''
    choice = ''
    resource = ''
    offset = ''

    try:
        opts, args = getopt.getopt(argv, "hgs:", \
        ["val=", "offset=", "help", "get", "set"])

    except getopt.GetoptError:
        usage()

    for opt, arg in opts:

        if opt in ('-h', '--help'):
            choice = 'help'

        elif opt in ('-g', '--get'):
            choice = 'get'

        elif opt in ('-s', '--set'):
            choice = 'set'

        elif opt == '--offset':
            offset = int(arg, 16)

        elif opt == '--val':
            val = int(arg, 16)

    if choice == 'get' and offset != '':
        portio_reg_read(resource, offset)

    elif choice == 'set' and offset != '' and val != '':
        portio_reg_write(resource, offset, val)

    else:
        usage()

#Calling the main method
if __name__ == "__main__":
    main(sys.argv[1:])
