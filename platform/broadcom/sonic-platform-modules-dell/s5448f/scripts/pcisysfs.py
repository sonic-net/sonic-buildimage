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

import struct
import sys
import getopt
from os import *
from mmap import *

def usage():
   ''' This is the Usage Method '''

   print('\t\t pcisysfs.py  --get --offset <offset> --res <resource>')
   print('\t\t pcisysfs.py --set --val <val>  --offset <offset> --res <resource>')
   sys.exit(1)

def pci_mem_read(mm,offset):
    mm.seek(offset)
    read_data_stream=mm.read(4)
    print("")
    reg_val=struct.unpack('I',read_data_stream)
    print("reg_val read:%x"%reg_val)
    return reg_val

def pci_mem_write(mm,offset,data):
    mm.seek(offset)
    print("data to write:%x",data)
    mm.write(struct.pack('I',data))

def pci_set_value(resource,val,offset):
    fd=open(resource,O_RDWR)
    mm=mmap(fd,0)
    pci_mem_write(mm,offset,val)

def pci_get_value(resource,offset):
    fd=open(resource,O_RDWR)
    mm=mmap(fd,0)
    pci_mem_read(mm,offset)

def main(argv):

    ''' The main function will read the user input from the
    command line argument and  process the request  '''

    opts = ''
    val = ''
    choice = ''
    resource = ''
    offset = ''

    try:
        opts, args = getopt.getopt(argv, "hgsv:" , \
        ["val=","res=","offset=","help", "get", "set"])
	
    except getopt.GetoptError:
        usage()

    for opt,arg in opts:

        if opt in ('-h','--help'):
            choice = 'help'

        elif opt in ('-g', '--get'):
            choice = 'get'

        elif opt in ('-s', '--set'):
            choice = 'set'

        elif opt ==  '--res':
            resource = arg

        elif opt ==  '--val':
            val = int(arg,16)

        elif opt ==  '--offset':
            offset = int(arg,16)

    if choice == 'set' and val != '' and offset !='' and resource !='':
        pci_set_value(resource,val,offset)

    elif choice == 'get' and offset != '' and resource !='':
        pci_get_value(resource,offset)

    else:
        usage()

#Calling the main method
if __name__ == "__main__":
    main(sys.argv[1:])

