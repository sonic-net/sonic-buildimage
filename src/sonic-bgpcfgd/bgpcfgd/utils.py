import subprocess
import yaml

from .log import log_crit, log_debug, log_err


def run_command(command, shell=False, hide_errors=False):
    """
    Run a linux command. The command is defined as a list. See subprocess.Popen documentation on format
    :param command: command to execute. Type: List of strings
    :param shell: execute the command through shell when True. Type: Boolean
    :param hide_errors: don't report errors to syslog when True. Type: Boolean
    :return: Tuple: integer exit code from the command, stdout as a string, stderr as a string
    """
    log_debug("execute command '%s'." % str(command))
    p = subprocess.Popen(command, shell=shell, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        if not hide_errors:
            print_tuple = p.returncode, str(command), stdout, stderr
            log_err("command execution returned %d. Command: '%s', stdout: '%s', stderr: '%s'" % print_tuple)

    return p.returncode, stdout, stderr


def read_constants():
    """ Read file with constants values from /etc/sonic/constants.yml """
    with open('/etc/sonic/constants.yml') as fp:
        content = yaml.safe_load(fp)
        if "constants" not in content:
            log_crit("/etc/sonic/constants.yml doesn't have 'constants' key")
            raise Exception("/etc/sonic/constants.yml doesn't have 'constants' key")
        return content["constants"]
