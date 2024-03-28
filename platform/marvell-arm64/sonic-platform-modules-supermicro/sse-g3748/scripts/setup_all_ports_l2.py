#!/usr/bin/python

import os

command1 = ""
command2 = "sudo config vlan add 10"
command3 = ""
os.system(command2)
for interface in range(0,54):
    command1 = ('sudo config interface ip remove Ethernet' + str(interface) + " " + '10.0.0.' + str(interface*2) + '/31' + '\n')
    os.system(command1)
    command3 =  "sudo config vlan member add -u 10 Ethernet" + str(interface) + '\n'
    os.system(command3)
