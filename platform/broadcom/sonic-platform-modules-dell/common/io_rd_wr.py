#!/usr/bin/python3
# Script to read/write the io based registers

import sys
import os
import getopt
import struct

io_resource = '/dev/port'

def usage():
    ''' This is the Usage Method '''

    print('Utility for IO read/write')
    print('\t\t io_rd_wr.py --get --offset <offset>')
    print('\t\t io_rd_wr.py --set --val <val> --offset <offset>')
    sys.exit(1)

def io_reg_read(io_resource, offset):
    fd = os.open(io_resource, os.O_RDONLY)
    if(fd < 0):
        print('file open failed %s' % io_resource)
        return
    if(os.lseek(fd, offset, os.SEEK_SET) != offset):
        print('lseek failed on %s' % io_resource)
        return
    buf = os.read(fd, 1)
    reg_val1 = ord(buf)
    print('reg value %x' % reg_val1)
    os.close(fd)

def io_reg_write(io_resource, offset, val):
    fd = os.open(io_resource, os.O_RDWR)
    if(fd < 0):
        print('file open failed %s' % io_resource)
        return
    if(os.lseek(fd, offset, os.SEEK_SET) != offset):
        print('lseek failed on %s' % io_resource)
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
        io_reg_read(io_resource, offset)

    elif choice == 'set' and offset != '' and val != '':
        io_reg_write(io_resource, offset, val)

    else:
        usage()

# Calling the main method
if __name__ == "__main__":
    main(sys.argv[1:])
