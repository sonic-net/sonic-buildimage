#!/usr/bin/env python3

import argparse
import pexpect
import sys
import time

class OnieInterface:
    """ONiE Telnet Interface"""

    #KEY_UP = '\x1b[A'
    KEY_DOWN = '\x1b[B'
    #KEY_RIGHT = '\x1b[C'
    #KEY_LEFT = '\x1b[D'

    ONIE_INSTALL_OS = 'ONIE: Install OS'
    ONIE_CONSOLE = 'Please press Enter to activate this console'
    ONIE_RESCUE = 'Rescue Mode Enabled'
    ONIE_SHELL = 'ONIE:/ #'
    ONIE_PROMPT = 'Do you still wish to install this image?'

    GRUB_SELECTION = 'The highlighted entry will be executed'

    INSTALL_CMD = [
        'tmpdir=$(mktemp -d)',
        'mount LABEL=INSTALLER $tmpdir',
        'onie-nos-install $tmpdir/onie-installer.bin'
    ]

    def __init__(self, telnet_session, arg_list):
        self.args = arg_list
        self.onie = telnet_session

    def wait_grub(self):
        self.onie.expect([self.GRUB_SELECTION])

    def embed_onie(self):
        self.wait_grub()
        self.onie.sendline(self.KEY_DOWN)

    def install_os(self):
        self.onie.expect([self.ONIE_INSTALL_OS])
        self.wait_grub()
        if self.args.f: # manual installation
            # enable rescue mode
            self.onie.sendline(self.KEY_DOWN)
            self.onie.expect([self.ONIE_CONSOLE])
            self.onie.sendline()
            self.onie.expect([self.ONIE_RESCUE])
            self.onie.expect([self.ONIE_SHELL])
            # install image
            self.onie.sendline(' ; '.join(self.INSTALL_CMD))
            # handle unsupported platform
            self.onie.expect([self.ONIE_PROMPT])
            self.onie.sendline('y')
        else: # automatic discovery installation
            self.onie.sendline()


def main():

    parser = argparse.ArgumentParser(description='test_login cmdline parser')
    parser.add_argument('-p', type=int, default=9000, help='local port')
    parser.add_argument('-f', action='store_true', help='force image installation')

    args = parser.parse_args()

    i = 0
    while True:
        try:
            p = pexpect.spawn("telnet 127.0.0.1 {}".format(args.p), timeout=1200, logfile=sys.stdout, encoding='utf-8')
            break
        except Exception as e:
            print(str(e))
            i += 1
            if i == 10:
                raise
            time.sleep(1)

    onie = OnieInterface(p, args)

    # select ONIE embed
    onie.embed_onie()

    # select ONIE install
    onie.install_os()

    # wait for grub, and exit
    onie.wait_grub()


if __name__ == '__main__':
    main()
