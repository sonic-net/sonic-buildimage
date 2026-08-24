import os
import signal
import logging
import subprocess

from logging.handlers import SysLogHandler
from sonic_py_common.syslogger import SysLogger


logger = SysLogger(
    log_identifier='healthd#utils',
    log_facility=SysLogHandler.LOG_DAEMON,
    log_level=logging.INFO,
    enable_runtime_config=False
)


def run_command(command, timeout=None):
    """
    Utility function to run an shell command and return the output.
    :param command: Shell command string.
    :return: Output of the shell command.
    """
    try:
        process = subprocess.Popen(
            command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            start_new_session=True
        )
        return process.communicate(timeout=timeout)[0]
    except subprocess.TimeoutExpired as err:
        logger.log_warning("Failed to run command: {}".format(str(err)))

        # The child process is not killed if the timeout expires,
        # so in order to cleanup properly a well-behaved application
        # should kill the child process and finish communication

        logger.log_notice("Initiate stuck process cleanup: pid={}".format(process.pid))

        try:
            os.killpg(process.pid, signal.SIGKILL)
        except Exception as e:
            logger.log_error("Failed to kill process group: {}".format(str(e)))

        try:
            process.communicate(timeout=1)
        except Exception as e:
            logger.log_error("Failed to wait for process: {}".format(str(e)))

        logger.log_notice("Cleanup is done: rc={}".format(process.returncode))

        return None
    except Exception:
        return None


def get_uptime():
    """
    Utility to get the system up time.
    :return: System up time in seconds.
    """
    with open('/proc/uptime', 'r') as f:
        uptime_seconds = float(f.readline().split()[0])

    return uptime_seconds
