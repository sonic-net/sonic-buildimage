#!/usr/bin/python
import pexpect
import re

def subprocess_exec(cmd):
    child = pexpect.spawn(cmd)
    ret_string=""
    try:
        while True:
            index = child.expect(["\n",pexpect.EOF],timeout=90)
            if index == 0:
                string = child.before.decode()
                print(string)
                string = re.sub(r'[\r\n]', '', string)
                ret_string = ret_string + string + '\n'
            else:
                child.close()
                return child.exitstatus,ret_string
    except pexpect.exceptions.TIMEOUT:
        return 1,ret_string + "\nTimeout exceeded"
    except :
        return 1,ret_string


class BaudrateUtil():
    def set_baudrate(self, baudrate):
        """
        Set machine baudrate

        @param: baudrate
        @return (True,log) for scuess, (False,log) for failure
        """
        status,log = subprocess_exec("baudrate.sh -s {}".format(baudrate))
        if status:
            return False,log
        return True,log

    def get_baudrate(self):
        """
        Get machine baudrate

        @param: None
        @return (True,baudrate) for scuess, (False,error_log) for failure
        """
        status,log = subprocess_exec("baudrate.sh -a")

        baudrate,sub_times=re.subn("[\s\S]*sonic running on (\d+)[\s\S]*","\\1",log)
        if status or sub_times == 0:
            return False,log

        return True,baudrate