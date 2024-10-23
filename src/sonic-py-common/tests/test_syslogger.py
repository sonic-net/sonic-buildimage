import logging
import os
import sys
from io import StringIO
from contextlib import redirect_stdout

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

modules_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(modules_path, 'sonic_py_common'))
from sonic_py_common import syslogger


class TestSysLogger:
    def test_notice_log(self, capsys):
        """NOTICE is a customize log level, added a test case here to make sure it works in future
        """
        log = syslogger.SysLogger()
        # remove syslog handler, unit test environment has no rsyslogd
        for handler in log.logger.handlers[:]:
            log.logger.removeHandler(handler)
        
        # put a stdout log handler 
        handler = logging.StreamHandler(sys.stdout)
        formatter = logging.Formatter('%(name)s[%(process)d] - %(levelname)s : %(message)s')
        handler.setFormatter(formatter)
        log.logger.addHandler(handler)
        
        log.log_notice('this is a message')
        captured = capsys.readouterr() 
        assert 'NOTICE' in captured.out
