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

    def test_basic(self):
        log = syslogger.SysLogger()
        log.logger.log = mock.MagicMock()
        log.log_error('error message')
        log.log_warning('warning message')
        log.log_notice('notice message')
        log.log_info('info message')
        log.log_debug('debug message')
        log.log(logging.ERROR, 'error msg', also_print_to_console=True)

    def test_log_priority(self):
        log = syslogger.SysLogger()
        log.set_min_log_priority(logging.ERROR)
        assert log.logger.level == logging.ERROR

    def test_log_priority_from_str(self):
        log = syslogger.SysLogger()
        assert log.log_priority_from_str('ERROR') == logging.ERROR
        assert log.log_priority_from_str('INFO') == logging.INFO
        assert log.log_priority_from_str('NOTICE') == logging.NOTICE
        assert log.log_priority_from_str('WARN') == logging.WARN
        assert log.log_priority_from_str('DEBUG') == logging.DEBUG
        assert log.log_priority_from_str('invalid') == logging.WARN

    def test_log_priority_to_str(self):
        log = syslogger.SysLogger()
        assert log.log_priority_to_str(logging.NOTICE) == 'NOTICE'
        assert log.log_priority_to_str(logging.INFO) == 'INFO'
        assert log.log_priority_to_str(logging.DEBUG) == 'DEBUG'
        assert log.log_priority_to_str(logging.WARN) == 'WARN'
        assert log.log_priority_to_str(logging.ERROR) == 'ERROR'
        assert log.log_priority_to_str(-1) == 'WARN'

    @mock.patch('swsscommon.swsscommon.SonicV2Connector')
    def test_runtime_config(self, mock_connector):
        mock_db = mock.MagicMock()
        mock_db.get = mock.MagicMock(return_value='DEBUG')
        mock_connector.return_value = mock_db
        log = syslogger.SysLogger(log_identifier='log1', enable_runtime_config=True, log_level=logging.INFO)
        assert log.logger.level == logging.DEBUG

        mock_db.get.return_value = 'ERROR'
        ret, msg = log.update_log_level()
        assert ret
        assert not msg

    @mock.patch('swsscommon.swsscommon.SonicV2Connector')
    def test_runtime_config_negative(self, mock_connector):
        mock_db = mock.MagicMock()
        mock_db.get = mock.MagicMock(side_effect=Exception(''))
        mock_connector.return_value = mock_db
        log = syslogger.SysLogger(log_identifier='log', enable_runtime_config=True)

        ret, msg = log.update_log_level()
        assert not ret
        assert msg
