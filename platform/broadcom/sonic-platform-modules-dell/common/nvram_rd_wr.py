#!/usr/bin/python3
# Script to read/write the nvram

import sys
import os
import getopt
import struct

nvram_resource = '/dev/nvram'

def usage():
    ''' This is the Usage Method '''

    print('Utility for NVRAM read/write')
    print('\t\t nvram_rd_wr.py --get --offset <offset>')
    print('\t\t nvram_rd_wr.py --set --val <val> --offset <offset>')
    sys.exit(1)

def nvram_reg_read(nvram_resource, offset):
    fd = os.open(nvram_resource, os.O_RDONLY)
    if(fd < 0):
        print('file open failed %s' % nvram_resource)
        return
    if(os.lseek(fd, offset, os.SEEK_SET) != offset):
        print('lseek failed on %s' % nvram_resource)
        return
    buf = os.read(fd, 1)
    reg_val1 = ord(buf)
    print('value %x' % reg_val1)
    os.close(fd)

def nvram_reg_write(nvram_resource, offset, val):
    fd = os.open(nvram_resource, os.O_RDWR)
    if(fd < 0):
        print('file open failed %s' % nvram_resource)
        return
    if(os.lseek(fd, offset, os.SEEK_SET) != offset):
        print('lseek failed on %s' % nvram_resource)
        return
    ret = os.write(fd, struct.pack('B', val))
    if(ret != 1):
        print('write failed %d' % ret)
        return
    os.close(fd)

def main(argv):

    ''' The main function will read the user input from the
    command line argument and  process the request  '''

    opts = ''
    val = ''
    choice = ''
    offset = ''

    try:
        opts, args = getopt.getopt(argv, "hgs:",
                                   ["val=", "offset=", "help", "get", "set"])
    except getopt.GetoptError:
        usage()

    if not os.path.exists(nvram_resource):
        print('NVRAM is not initialized')
        sys.exit(1)

    for opt, arg in opts:

        if opt in ('-h', '--help'):
            choice = 'help'

        elif opt in ('-g', '--get'):
            choice = 'get'

        elif opt in ('-s', '--set'):
            choice = 'set'

        elif opt == '--offset':
            offset = int(arg, 16) - 0xE

        elif opt == '--val':
            val = int(arg, 16)

    if choice == 'get' and offset != '':
        nvram_reg_read(nvram_resource, offset)

    elif choice == 'set' and offset != '' and val != '':
        nvram_reg_write(nvram_resource, offset, val)

    else:
        usage()

# Calling the main method
if __name__ == "__main__":
    main(sys.argv[1:])
