#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import re
import time
import mmap
configfile_pre   =  "/usr/local/bin/"
import sys
sys.path.append(configfile_pre)
from platform_config import *
import subprocess
import pexpect
import shlex

SYSLOG_IDENTIFIER = "UTILTOOL"

def os_system(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    return status, output

def get_machine_info():
    if not os.path.isfile('/host/machine.conf'):
        return None
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars

def get_platform_info(machine_info):
    if machine_info != None:
        if machine_info.__contains__('onie_platform'):
            return  machine_info['onie_platform']
        elif machine_info.__contains__('aboot_platform'):
            return machine_info['aboot_platform']
    return None

'''
def cpld_version_restful(url):
    if url == "" or len(url) <=0:
        print("invalid url")
        return
    bmc = BMCMessage()
    value = bmc.getBmcValue(url)
    json_dicts=json.dumps(value,indent=4)
    return json_dicts
'''

def lpc_cpld_rd(reg_addr):
    try:
        regaddr = 0
        if type(reg_addr) == int:
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        devfile = "/dev/lpc_cpld"
        fd = os.open(devfile, os.O_RDWR|os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        str = os.read(fd, 1)
        os.close(fd)
        return "%02x" % ord(str)
    except ValueError:
        return None
    except Exception as e:
        print (e)
        return None


def my_log(txt):
    if DEBUG == True:
        print ("[PLATFORM]:",)
        print (txt)
    return

def log_os_system(cmd, show):
    my_log ('         Run :'+ cmd)
    status, output = subprocess.getstatusoutput(cmd)
    my_log (" with result :" + str(status))
    my_log ("      output :" + output)
    if status:
        log_error('Failed :%s msg:%s'%(cmd,output))
        if show:
            print ('Failed :'+ cmd)
    return  status, output

def password_command(cmd, password, exec_timeout=30):

    newkey = 'continue connecting'
    log_os_system("rm -rf ~/.ssh", 0)
    msg = ""
    try_times = 3
    try_times_conter = try_times
    while try_times_conter:
        child = pexpect.spawn(cmd)
        if try_times != try_times_conter:
            time.sleep(5)
        try_times_conter -= 1
        try:
            i = child.expect([pexpect.TIMEOUT, newkey, 'password: ',"refused",pexpect.EOF],timeout=30)
            # If the login times out, print an error message and exit.
            if i == 0: # Timeout
                msg = 'connect to BMC timeout'
                continue
            # no public key
            if i == 1:
                child.sendline ('yes')
                i = child.expect([pexpect.TIMEOUT, 'password: '],timeout=30)
                if i == 0: # Timeout
                    msg = 'connect to BMC timeout'
                    continue
                if i == 1:# Go below and enter the logic of the password
                    i = 2
            if i == 2: # Enter the password
                child.sendline (password)
                i = child.expect([pexpect.EOF, pexpect.TIMEOUT], exec_timeout)
                if i == 0:
                    return True,child.before
                if i == 1:
                    msg = str(child.before)+"\nBMC run commands timeout"
                    return False,msg
            if i == 3: # BMC Connection refused
                msg =  'connect to BMC failed'
                continue
            if i == 4:
                msg = child.before
        except Exception as e:
            msg = str(child.before)+"\nconnect to BMC failed"

    return False,msg

def password_command_realtime(ssh_header, ssh_cmd, password,key_words, exec_timeout=30):

    key_word_end = key_words.get("key_word_end")
    key_word_pass = key_words.get("key_word_pass")
    key_word_noshow = key_words.get("key_word_noshow")
    # Prevents waiting caused by BMC restart
    key_word_exit = key_words.get("key_word_exit")

    if None in [key_word_end,key_word_pass]:
        print ("Missing parameters")
        return False

    newkey = 'continue connecting'
    log_os_system("rm -rf ~/.ssh", 0)
    msg = ""
    try_times = 3
    key_word_pass_flag = False
    try_times_conter = try_times
    child = pexpect.spawn(ssh_header)
    try:
        i = child.expect([pexpect.TIMEOUT,newkey, 'password: ',"refused",pexpect.EOF],timeout=30)
        # If the login times out, print an error message and exit.
        if i == 0: # Timeout
            msg = 'connect to BMC timeout'
        # no public key
        if i == 1:
            child.sendline ('yes')
            i = child.expect([pexpect.TIMEOUT, 'password: '],timeout=30)
            if i == 0: # Timeout
                msg = 'connect to BMC timeout'
            if i == 1:# Go below and enter the logic of the password
                i = 2

        if i == 2: # Enter the password
            child.sendline (password)
            i = child.expect([pexpect.EOF, "\r",pexpect.TIMEOUT], exec_timeout)
            if i == 0:
                print (child.before)
                return key_word_pass_flag
            if i == 1:
                child.sendline(ssh_cmd)
                # amount received is similar to root@switch2 in order to avoid misjudgment about the end of execution
                usr_symble_first = True
                bmc_str_tmp=""
                while True:
                    i = child.expect([pexpect.EOF,"\r","\n",key_word_end, pexpect.TIMEOUT], exec_timeout)
                    if i == 0:
                        return key_word_pass_flag
                    elif i in [1,2]:
                        if key_word_noshow == None or key_word_noshow not in child.before:
                             bmc_str, times = re.subn("\r|\n","",child.before)
                             if len(bmc_str) > 1:
                                 print (bmc_str)
                                 bmc_str_tmp=bmc_str_tmp + bmc_str
                                # print bmc_str_tmp
                       # if key_word_pass in child.before:
                        if re.search(key_word_pass,bmc_str_tmp) != None:
                            key_word_pass_flag = True
                        if key_word_exit != None and key_word_exit in child.before:
                            # Give the BMC time to execute the last command
                            time.sleep(3)
                            return key_word_pass_flag
                    elif i == 3 :
                        if usr_symble_first:
                            usr_symble_first = False
                        else:
                            return key_word_pass_flag

        if i == 3: # BMC Connection refused
            msg =  'connect to BMC failed'
        if i == 4:
            msg = child.before
    except Exception as e:
        print (e)
        msg = str(child.before)+"\nconnect to BMC error"
        print (msg)

    return False

def get_sys_execute2(cmd, key_word_pass):
    # key_word_pass_flag1 = False
    key_word_pass_flag = False
    filename = "/tmp/diag_excute_out"
    cmd = cmd + "|tee %s" % filename
    p =  subprocess.Popen(shlex.split(cmd), shell=False)
    p.wait()
    with open(filename, 'r') as f:
        str1 = f.read()
        if key_word_pass in str1:
            key_word_pass_flag = True
       # if key_word_pass_flag1 and "100%" in str1:
          # key_word_pass_flag = True
    log_os_system("rm %s"%filename,0)
    return key_word_pass_flag


BMC_PATH = "PATH=/usr/sbin/:/bin/:/usr/bin/:/sbin"
RETURN_KEY1       = "code"
RETURN_KEY2       = "msg"
DEBUG             = False

def wbi2cget(bus, devno, address):
    command_line = "i2cget -f -y %d 0x%02x 0x%02x " % (bus, devno, address)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = os_system(command_line)
        if ret == 0:
            return True, ret_t
        time.sleep(0.1)
    return False, ret_t

def strtoint(str):  # Hexadecimal string to int,"4040"/"0x4040"/"0X4040" = 16448
    value = 0
    rest_v = str.replace("0X", "").replace("0x", "")
    for index in range(len(rest_v)):
        value |= int(rest_v[index], 16) << ((len(rest_v) - index - 1) * 4)
    return value

def pci_read(pcibus, slot,  fn , bar, offset):
    if offset % 4 != 0:
        return None
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
    file = open(filename, "r+")
    size = os.path.getsize(filename)
    data = mmap.mmap(file.fileno(), size)
    result = data[offset: offset + 4]
    s = result[::-1]
    val = 0
    for i in range(0, len(s)):
        val = val << 8  | ord(s[i])
    data.close()
    file.close()
    return val

###########################################
# Run the DMI command to obtain the BIOS information
###########################################
def getDmiSysByType(type_t):
    ret, log = os_system("which dmidecode ")
    if ret != 0 or len(log) <= 0:
        error = "cmd find dmidecode"
        return False, error
    cmd = log + " -t %s" % type_t
    # Get the total first
    ret1, log1 = os_system(cmd)
    if ret != 0 or len(log1) <= 0:
        return False, "Command error[%s]" % cmd
    its = log1.replace("\t", "").strip().split("\n")
    ret = {}
    for it in its:
        if ":" in it:
            key = it.split(":")[0].lstrip()
            value = it.split(":")[1].lstrip()
            ret[key] = value
    return True, ret
