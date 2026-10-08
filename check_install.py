#!/usr/bin/env python3

import argparse
import os
import pexpect
import sys
import time


def build_arg_parser():
    parser = argparse.ArgumentParser(description='test_login cmdline parser')
    parser.add_argument('-u', default="admin", help='login user name')
    parser.add_argument('-P', default=None, help='login password (default: $SONIC_PASSWORD)')
    parser.add_argument('-N', default=None, help='new password (default: $SONIC_NEW_PASSWORD)')
    parser.add_argument('-p', type=int, default=9000, help='local port')
    return parser


def resolve_credentials(parser, args):
    """Resolve login/new passwords from CLI args, falling back to environment
    variables. CLI arguments take precedence over environment variables.
    Fails with an argparse error (no hardcoded fallback) if either value is
    missing or empty.
    """
    login_password = args.P if args.P is not None else os.environ.get("SONIC_PASSWORD")
    new_password = args.N if args.N is not None else os.environ.get("SONIC_NEW_PASSWORD")

    if not login_password:
        parser.error("login password must be provided via -P or the SONIC_PASSWORD environment variable")
    if not new_password:
        parser.error("new password must be provided via -N or the SONIC_NEW_PASSWORD environment variable")

    return login_password, new_password


def main():

    parser = build_arg_parser()
    args = parser.parse_args()
    login_password, new_password = resolve_credentials(parser, args)

    login_prompt = 'sonic login:'
    passwd_prompt = 'Password:'
    cmd_prompt = "{}@sonic:~\$ $".format(args.u)
    grub_selection = "The highlighted entry will be executed"
    firsttime_prompt = 'firsttime_exit'
    passwd_change_prompt = ['Current password:', 'New password:', 'Retype new password:']

    i = 0
    while True:
        try:
            p = pexpect.spawn("telnet 127.0.0.1 {}".format(args.p), timeout=600, encoding='utf-8')
            p.logfile_read = sys.stdout
            break
        except Exception as e:
            print(str(e))
            i += 1
            if i == 10:
                raise
            time.sleep(1)

    # select default SONiC Image
    p.expect(grub_selection)
    p.sendline()
    # bootup sonic image
    while True:
        i = p.expect([login_prompt, passwd_prompt, firsttime_prompt, cmd_prompt])
        if i == 0:
            # send user name
            p.sendline(args.u)
        elif i == 1:
            # send password
            p.sendline(login_password)
            # Check for password change prompt
            try:
                p.expect('Current password:', timeout=2)
            except pexpect.TIMEOUT:
                break
            else:
                # send old password for password prompt
                p.sendline(login_password)
                p.expect(passwd_change_prompt[1])
                # send new password
                p.sendline(new_password)
                p.expect(passwd_change_prompt[2])
                # retype new password
                p.sendline(new_password)
                time.sleep(1)
                # Restore default password
                p.sendline('passwd {}'.format(args.u))
                p.expect(passwd_change_prompt[0])
                p.sendline(new_password)
                p.expect(passwd_change_prompt[1])
                p.sendline(login_password)
                p.expect(passwd_change_prompt[2])
                p.sendline(login_password)
                break
        elif i == 2:
            # fix a login timeout issue, caused by the login_prompt message mixed with the output message of the rc.local
            time.sleep(1)
            p.sendline()
        else:
            break

    # check version
    time.sleep(5)
    p.sendline('uptime')
    p.expect([cmd_prompt])
    p.sendline('show version')
    p.expect([cmd_prompt])
    p.sendline('show ip bgp sum')
    p.expect([cmd_prompt])
    p.sendline('sync')
    p.expect([cmd_prompt])


if __name__ == '__main__':
    main()
