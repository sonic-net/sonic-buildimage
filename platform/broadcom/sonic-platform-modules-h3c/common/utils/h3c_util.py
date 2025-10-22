#!/usr/bin/env python
# Copyright (C) 2020 H3C, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Usage: %(scriptName)s [options] command object

options:
    -h | --help     : this help message
    -d | --debug    : run with debug mode
    -f | --force    : ignore error during installation or clean
command:
    install         : install b3010 drivers
    clean           : uninstall b3010 drivers
"""

import os
import commands
import sys
import getopt
import logging
import syslog
from vendor_sonic_platform.devcfg import Devcfg
import optoe

DEBUG = False
ARGS = []
FORCE = 0

if DEBUG:
    print sys.argv[0]
    print 'ARGV: ', sys.argv[1:]


def main():
    global DEBUG
    global ARGS
    global FORCE

    if len(sys.argv) < 2:
        show_help()

    options, ARGS = getopt.getopt(sys.argv[1:], 'hdf', ['help', 'debug', 'force'])

    if DEBUG:
        print options
        print ARGS
        print len(sys.argv)

    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        elif opt in ('-f', '--force'):
            FORCE = 1
        else:
            logging.info('no option')

    for arg in ARGS:
        if arg == 'install':
            install(0)
        elif arg == 'fast-reboot-install':
            install(1)
        elif arg == 'clean':
            uninstall()
        else:
            show_help()

    return 0


def show_help():
    print __doc__ % {'scriptName': sys.argv[0].split("/")[-1]}
    sys.exit(0)


def show_log(txt):
    if DEBUG:
        print "[Demo]" + txt
    return


def exec_cmd(cmd, show):
    logging.info('Run :' + cmd)
    status, output = commands.getstatusoutput(cmd)
    show_log(cmd + " with result:" + str(status))
    show_log("      output:" + output)

    if status:
        logging.info('Failed :' + cmd)
        if show:
            print 'Failed :' + cmd

    return status, output


H3C_DRIVERS = [
    'bsp_base',
    'syseeprom',
    'cpld',
]


def install(boot_option=0):
    """ install h3c driver """
    ''' boot_option: 0 - normal, 1 - fast-reboot '''
    global FORCE

    status, output = exec_cmd('uname -r', 1)
    if status:
        print "Get kernel version failed!"
        sys.exit(0)
    plain_cmd = 'insmod /lib/modules/' + output + '/extra/'
    for drv in H3C_DRIVERS:
        drv = drv + '.ko'
        if 'dom_iic' in drv:
            extra_cmd = ' Portnum={}'.format(Devcfg.PORT_END - Devcfg.PORT_START + 1)
            cmd = plain_cmd + drv + extra_cmd
        else:
            cmd = plain_cmd + drv
        status, output = exec_cmd(cmd, 1)

    os.system('nohup /usr/bin/python /usr/local/bin/h3c_hw_mon.py &')

    if status:
        print output
        if FORCE == 0:
            return status
    optoe.install()

    status, output = exec_cmd('bash /usr/local/bin/set_68127_0v75_low_threshold.sh install', 1)
    if status:
        print output
        print "change 0v75 failed!"
        syslog.syslog(syslog.LOG_ERR, str(output))
        syslog.syslog(syslog.LOG_ERR, str('change 0v75 failed!'))
        sys.exit(0)

    return


def ext_ko_unintall(koname):
    cmd = 'lsmod | grep ' + koname
    status, output = exec_cmd(cmd, 0)
    if status == 0:
        val, output = exec_cmd('rmmod ' + koname, 0)
        if val != 0:
            print(output)
    return


def uninstall():
    """ uninstall h3c driver """
    global FORCE
    os.system('ps -aux | grep h3c_hw_mon | grep -v grep | awk \'{print $2}\' | xargs kill -9')

    ext_ko_unintall('wishbone')
    ext_ko_unintall('dom_support')
    ext_ko_unintall('fpgaspi')

    for i in range(len(H3C_DRIVERS) - 1, -1, -1):
        status, output = exec_cmd('rmmod ' + H3C_DRIVERS[i], 1)

    if status:
        print output
        if FORCE == 0:
            return status
    optoe.uninstall()
    return


if __name__ == "__main__":
    main()
