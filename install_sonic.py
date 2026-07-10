#!/usr/bin/env python3

import argparse
import pexpect
import sys
import time


def main():

    parser = argparse.ArgumentParser(description='test_login cmdline parser')
    parser.add_argument('-p', type=int, default=9000, help='local port')

    args = parser.parse_args()

    #KEY_UP = '\x1b[A'
    KEY_DOWN = '\x1b[B'
    #KEY_RIGHT = '\x1b[C'
    #KEY_LEFT = '\x1b[D'

    grub_selection = "The highlighted entry will be executed"

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

    # select ONIE embed
    p.expect(grub_selection)
    p.sendline(KEY_DOWN)

    # select ONIE install
    p.expect(['ONIE: Install OS'])
    p.expect([grub_selection])
    p.sendline()

    # wait for grub, and exit
    p.expect([grub_selection])


if __name__ == '__main__':
    main()
