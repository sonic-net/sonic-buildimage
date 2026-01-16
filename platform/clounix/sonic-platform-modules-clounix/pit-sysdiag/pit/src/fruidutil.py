#!/usr/bin/python
# -*- coding: UTF-8 -*-
import re
import argparse
from function import exec_cmd

# psu1_fru_info = """
# FRU Device Description : PSU1_FRU (ID 34)
#  Product Manufacturer  : Great Wall
#  Product Name          : GW-CRPS1600D2W
#  Product Part Number   :
#  Product Version       : 00.01.01
#  Product Serial        : 2L020080022
# """

usage = """
fruidutil get [NODE+NUM]
          set [NODE+NUM] -f <file>

OPTIONS:    
    option                     <get|set> fru information
    node                       <fan1|fan2|psu1|...>fru node name 
    -f                         Path of xxx.bin file
    -h                         Print use guide
"""


def fruidutil(args):
    """
        FRU Commands:  list read write edit
        fru list                                             - list all fru
        fru list <type num>                                  - list specific fru data
        fru read <type num> <fru file>                       - store fru data to file
        fru write <type num> <fru file>                      - write fru data from file
        fru edit <type num> field <section> <index> <string> - edit FRU string

        You should move fru file to /usr/local/bin first
    """
    cmd = ''
    if args.option in ['get', 'set']:
        cmd += 'fruidutil get' if args.option == 'get' else 'fruidutil set'
        if args.node:
            if 'fan' in args.node:
                cmd = 'fruidutil get fanboard' 
            elif 'psu1' in args.node:
                cmd = 'fruidutil get psu 1'
            elif 'psu2' in args.node:
                cmd = 'fruidutil get psu 2' 

            try:
                fru_info = exec_cmd(cmd)
                print(fru_info)
            except Exception as err:
                print('{} run failed as: {}'.format(cmd, err))

    # if args.option in ['get', 'set']:
    #     cmd += 'fru list' if args.option == 'get' else 'fru edit'
    #     if args.node:
    #         find_num = re.findall('[a-z0]+(\d+)', args.node)
    #         if find_num:
    #             num = find_num[0]
    #             if num <= 0:
    #                 print('error: num is incorrect')
    #             else:
    #                 cmd += ' ' + re.sub('\d', '', args.node) + num
    #                 if args.f:
    #                     cmd += ' ' + args.f
    #                     cmd = cmd.replace('edit', 'write')
    #                 try:
    #                     fru_info = exec_cmd(cmd)
    #                     print(fru_info)
    #                 except Exception as err:
    #                     print('{} run failed as: {}'.format(cmd, err))
    #         else:
    #             print('error: num is missing')
    # else:
    #     print(usage)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Base fru command", add_help=False, usage=usage)
    parser.add_argument('option', type=str, help='get/set')
    parser.add_argument('node', type=str, help='<node[num]> ex:psu1|fan1... num start from 1')
    parser.add_argument('-f', type=str, help='[file] example: /root/fru.bin')
    parser.add_argument('-h', action='help', help='Help Information')
    args = parser.parse_args()
    fruidutil(args)
