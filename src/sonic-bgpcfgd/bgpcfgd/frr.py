import os
import datetime
import time
import tempfile

from bgpcfgd.log import log_err, log_info, log_warn, log_crit
from .vars import g_debug
from .utils import run_command


class FRR(object):
    """Proxy object with FRR"""
    def __init__(self, daemons):
        self.daemons = daemons

    def wait_for_daemons(self, seconds):
        """
        Wait until FRR daemons are ready for requests
        :param seconds: number of seconds to wait, until raise an error
        """
        stop_time = datetime.datetime.now() + datetime.timedelta(seconds=seconds)
        log_info("Start waiting for FRR daemons: %s" % str(datetime.datetime.now()))
        while datetime.datetime.now() < stop_time:
            ret_code, out, err = run_command(["vtysh", "-c", "show daemons"], hide_errors=True)
            if ret_code == 0 and all(daemon in out for daemon in self.daemons):
                log_info("All required daemons have connected to vtysh: %s" % str(datetime.datetime.now()))
                return
            else:
                log_warn("Can't read daemon status from FRR: %s" % str(err))
            time.sleep(0.1)  # sleep 100 ms
        raise RuntimeError("FRR daemons hasn't been started in %d seconds" % seconds)

    @staticmethod
    def get_config():
        ret_code, out, err = run_command(["vtysh", "-c", "show running-config"])
        if ret_code != 0:
            log_crit("can't update running config: rc=%d out='%s' err='%s'" % (ret_code, out, err))
            return ""
        return out

    @staticmethod
    def write(config_text):
        fd, tmp_filename = tempfile.mkstemp(dir='/tmp')
        os.close(fd)
        with open(tmp_filename, 'w') as fp:
            fp.write("%s\n" % config_text)
        command = ["vtysh", "-f", tmp_filename]
        ret_code, out, err = run_command(command)
        if ret_code != 0:
            err_tuple = tmp_filename, ret_code, out, err
            log_err("ConfigMgr::commit(): can't push configuration from file='%s', rc='%d', stdout='%s', stderr='%s'" % err_tuple)
        else:
            if not g_debug:
                os.remove(tmp_filename)
        return ret_code == 0

    @staticmethod
    def restart_peer_groups(peer_groups):
        """ Restart peer-groups which support BBR
        :param peer_groups: List of peer_groups to restart
        :return: True if restart of all peer-groups was successful, False otherwise
        """
        res = True
        for peer_group in sorted(peer_groups):
            rc, out, err = run_command(["vtysh", "-c", "clear bgp peer-group %s soft in" % peer_group])
            if rc != 0:
                log_value = peer_group, rc, out, err
                log_crit("Can't restart bgp peer-group '%s'. rc='%d', out='%s', err='%s'" % log_value)
            res = res and (rc == 0)
        return res
