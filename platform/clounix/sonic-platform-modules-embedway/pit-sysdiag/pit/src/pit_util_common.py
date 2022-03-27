# -*- coding:utf-8 -*-
from __future__ import print_function
from test_case import TCBase, TestCaseCommon
from subprocess import PIPE, Popen
from errcode import E
import subprocess
import threading
import time
import sys
import signal
import syslogger
import pexpect
import imp
import os
import re

# from threading import Lock, Thread
PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
PLATFORM_ROOT_PATH_DOCKER = "/usr/share/sonic/platform"

SONIC_CFGGEN_PATH = "/usr/local/bin/sonic-cfggen"
HWSKU_KEY = "DEVICE_METADATA['localhost']['hwsku']"
PLATFORM_KEY = "DEVICE_METADATA['localhost']['platform']"


def get_platform_and_hwsku():
    try:
        proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-H', '-v', PLATFORM_KEY],
                                stdout=subprocess.PIPE,
                                shell=False,
                                stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0]
        proc.wait()
        stdout = stdout.decode('utf-8')
        platform = stdout.rstrip("\n")

        proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                stdout=subprocess.PIPE,
                                shell=False,
                                stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0]
        proc.wait()
        stdout = stdout.decode('utf-8')
        hwsku = stdout.rstrip("\n")
    except OSError as e:
        raise OSError("Cannot detect platform")

    return (platform, hwsku)


def load_platform_util_module(module_name):
    platform_util_module = None

    (platform, hwsku) = get_platform_and_hwsku()
    platform_path = ''
    if len(platform) != 0:
        platform_path = "/".join([PLATFORM_ROOT_PATH, platform])
    else:
        platform_path = PLATFORM_ROOT_PATH_DOCKER

    hwsku_path = "/".join([platform_path, hwsku])

    try:
        module_file = "/".join([platform_path, "plugins", module_name + ".py"])
        platform_util_module = imp.load_source(module_name, module_file)
    except IOError as e:
        return None

    return platform_util_module

    # try:
    #     platform_util_class = getattr(platform_util_module, class_name)
    #     platform_util = platform_util_class()
    # except AttributeError as e:
    #     return None

    # return platform_util


def run_command(cmd):
    proc = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if err and proc.returncode != 0:
        return proc.returncode, err
    out = out.decode('utf-8')
    return 0, out.rstrip('\n')


def execute_cmd(cmdlist):
    for cmd in cmdlist:
        proc = Popen(
            cmd,
            stdin=PIPE,
            stderr=sys.stderr,
            close_fds=True,
            stdout=sys.stdout,
            universal_newlines=True,
            shell=True,
            bufsize=1)
        proc.communicate()
        signal.signal(signal.SIGINT, quit)
    return proc.returncode


def execute_cmd_return_match(cmd, pattern, linemode=True):
    statu, log = run_command(cmd)
    if statu != 0 or len(log) <= 0:
        print("cmd execute error")
    else:
        if linemode:
            for line in log.splitlines():
                result_list = re.findall(pattern, line)
                if result_list != []:
                    return result_list
        else:
            result_list = re.findall(pattern, log)
            return result_list


def search_pattern_in_content_return_match(output, pattern, linemode=True):
    if linemode:
        for line in output.splitlines():
            result_list = re.findall(pattern, line)
            if result_list != []:
                return result_list
    else:
        result_list = re.findall(pattern, output)
        return result_list


def execute_cmd_return_boolen(cmd, paras, linemode=True):
    flag = 0
    fail_list = []
    if isinstance(paras, dict):
        for key, value in paras.items():
            if linemode:
                match_list = execute_cmd_return_match(cmd, key, True)
                result = ''.join(match_list)
                if value != result:
                    fail_list.append(value)
                    flag += 1
            else:
                match_list = execute_cmd_return_match(cmd, key, False)
                result = ''.join(match_list)
                if value != result:
                    fail_list.append(value)
                    flag += 1

        if flag:
            return False, fail_list
        else:
            return True, fail_list

    elif isinstance(paras, list):
        for pattern in paras:
            if linemode:
                match_list = execute_cmd_return_match(cmd, pattern, True)
                if match_list is None:
                    fail_list.append(pattern)
                    flag += 1
            else:
                match_list = execute_cmd_return_match(cmd, pattern, False)
                print(match_list)
                if match_list == []:
                    fail_list.append(pattern)
                    flag += 1

        if flag:
            return False, fail_list
        else:
            return True, fail_list
    else:
        pass


class waiting():
    def __init__(self, logger, mesg):
        self.logger = logger
        self.mesg = mesg
        self.lock = threading.Lock()
        self.lock.acquire()
        self.thread = threading.Thread(target=self.flush_display)
        self.thread.start()

    def flush_display(self):
        while True:
            for item in ["\\", "|", "/", "—"]:
                print("\r{} {}".format(self.mesg, item), end='')
                sys.stdout.flush()
                time.sleep(0.25)
                if not self.lock.locked():
                    break
            time.sleep(0.25)
            if not self.lock.locked():
                break

    def stop(self, mesg=None):
        if mesg is None:
            mesg = self.mesg
        self.lock.release()
        print("\r{}    ".format(mesg), end="")
        sys.stdout.flush()
        print('\r')
        self.logger.log_info(mesg)


class sort_list():
    def __init__(self):
        pass

    def tryint(self, s):
        try:
            return int(s)
        except ValueError:
            return s

    def str2int(self, v_str):
        return [self.tryint(sub_str)
                for sub_str in re.split('([0-9]+)', v_str)]

    def sort_humanly(self, v_list):
        return sorted(v_list, key=self.str2int, reverse=True)


class remote():
    def __init__(self, logger, fail_reason, username, ip, password):
        self.logger = logger
        self.username = username
        self.ip = ip
        self.password = password
        self.fail_reason = fail_reason

    def connect(self, also_print_console=False):
        try:
            command = 'ssh {}@{}'.format(self.username, self.ip)
            self.obj = pexpect.spawn(command, timeout=10)
            expect_list = [
                'yes/no',
                'password:',
                '~',
                'Permission denied',
                pexpect.TIMEOUT,
                pexpect.EOF,
            ]
            while True:
                index = self.obj.expect(expect_list, timeout=10)
                if index == 0:
                    self.obj.sendline("yes")
                    continue
                if index == 1:
                    self.obj.sendline(self.password)
                    continue
                if index == 2:
                    ret = E.OK
                    return ret
                if index == 3:
                    self.obj.close()
                    ret = E.ESSH35001
                    self.fail_reason.append(
                        "[ remote ] connect fail ,PASSWORD OR HOSTNAME is wrong")
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
                if index == 4:
                    self.obj.close()
                    ret = E.ESSH35002
                    self.fail_reason.append(
                        "[ remote ] connect timeout ,PASSWORD OR HOSTNAME is wrong")
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
                if index == 5:
                    self.obj.close()
                    ret = E.ESSH35003
                    self.fail_reason.append(
                        "[ remote ] connect timeout ,PASSWORD OR HOSTNAME is wrong")
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
        except BaseException as error:
            print(error)
            print('[ remote ] Error !!! ')

    def command(self, cmd, time=5, expect_list=['#', '~']):
        self.obj.sendline(cmd)
        self.obj.expect('\r\n', timeout=time)
        self.obj.expect(expect_list, timeout=time)
        try:
            output = self.obj.before.decode()
            index = output.rfind('\r\n')
            if index == -1:
                output = ''
            else:
                output = output[0:index]
                if output[0] == '|':
                    index = output.find('\r\n')
                    if index == -1:
                        output = ''
                    else:
                        output = output[index + 2:]
            return output
        except BaseException as error:
            print(error)
            print('[ remote ] Error !!! ')

    def disconnect(self):
        self.obj.close()


def ssh_command(
        self,
        cmd,
        username,
        ip,
        password,
        time=10,
        also_print_console=False):
    ssh = pexpect.spawn('ssh {}@{} "{}"'.format(username, ip, cmd))
    try:
        expect_list = [
            'yes/no',
            'password:',
            '# ',
            pexpect.TIMEOUT,
            pexpect.EOF
        ]
        while True:
            index = ssh.expect(expect_list, timeout=time)
            if index == 0:
                ssh.sendline("yes")
                continue
            if index == 1:
                ssh.sendline(password)
                expect_list = ['password:', 'Permission denied', pexpect.EOF]
                index = ssh.expect(expect_list, timeout=time)
                if index == 0 or index == 1:
                    ssh.close()
                    ret = E.ESSH35001
                    output = '[ remote ] connect fail, PASSWORD OR HOSTNAME is wrong'
                    self.fail_reason.append(output)
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret, output
            if index == 2:
                ret = E.OK
                output = ssh.before.decode()
                return ret, output
            if index == 3:
                ret = E.ESSH35002
                output = '[ remote ] connect timeout, PASSWORD OR HOSTNAME is wrong'
                self.fail_reason.append(output)
                self.logger.log_err("FAIL!", also_print_console)
                return ret, output
            if index == 4:
                ret = E.ESSH35003
                output = '[ remote ] connect EOF err, PASSWORD OR HOSTNAME is wrong'
                self.fail_reason.append(output)
                self.logger.log_err("FAIL!", also_print_console)
                return ret, output
    except BaseException as err:
        output = str(err)
        ret = E.EFAIL
        return ret, output


def isIpV4AddrLegal(ipStr):
    if '.' not in ipStr:
        return False
    ip_split_list = ipStr.strip().split('.')
    if len(ip_split_list) != 4:
        return False
    for i in range(4):
        try:
            ip_split_list[i] = int(ip_split_list[i])
        except BaseException:
            print("IP invalid for not number: " + ipStr)
            return False
        if ip_split_list[i] <= 255 and ip_split_list[i] >= 0:
            pass
        else:
            print("IP invalid: " + ipStr)
            return False
    if int(ip_split_list[0] == 0):
        print("ip format wrong")
        return False
    return True
