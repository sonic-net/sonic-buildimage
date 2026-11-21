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
        assert log.logger.level == logging.ERROR
        mock_db.hmset.assert_called_with('STATE_DB', 'LOGGER_TABLE|log1', {
            'LOGLEVEL': 'ERROR',
            'LOGOUTPUT': 'SYSLOG',
            'require_manual_refresh': 'true'
        })
        for call in mock_db.hmset.call_args_list + mock_db.set.call_args_list + mock_db.hset.call_args_list:
            assert call.args[0] != 'CONFIG_DB'

    @mock.patch('swsscommon.swsscommon.SonicV2Connector')
    def test_runtime_config_absent_row_is_default(self, mock_connector):
        """An absent CONFIG_DB row means the logger's own default; CONFIG_DB is never written."""
        mock_db = mock.MagicMock()
        mock_db.get = mock.MagicMock(return_value=None)
        mock_connector.return_value = mock_db
        log = syslogger.SysLogger(log_identifier='log2', enable_runtime_config=True, log_level=logging.INFO)
        assert log.logger.level == logging.INFO
        mock_db.hmset.assert_called_once_with('STATE_DB', 'LOGGER_TABLE|log2', {
            'LOGLEVEL': 'INFO',
            'LOGOUTPUT': 'SYSLOG',
            'require_manual_refresh': 'true'
        })

        # A configured level applies, and removing it again reverts to the default
        mock_db.get.return_value = 'DEBUG'
        assert log.update_log_level() == (True, '')
        assert log.logger.level == logging.DEBUG

        mock_db.get.return_value = None
        assert log.update_log_level() == (True, '')
        assert log.logger.level == logging.INFO
        assert mock_db.hmset.call_args.args == ('STATE_DB', 'LOGGER_TABLE|log2', {
            'LOGLEVEL': 'INFO',
            'LOGOUTPUT': 'SYSLOG',
            'require_manual_refresh': 'true'
        })
        for call in mock_db.method_calls:
            if call[0] in ('hmset', 'set', 'hset', 'delete', 'hdel'):
                assert call.args[0] != 'CONFIG_DB', call

    @mock.patch('swsscommon.swsscommon.SonicV2Connector')
    def test_runtime_config_negative(self, mock_connector):
        mock_db = mock.MagicMock()
        mock_db.get = mock.MagicMock(side_effect=Exception(''))
        mock_connector.return_value = mock_db
        log = syslogger.SysLogger(log_identifier='log', enable_runtime_config=True)

        ret, msg = log.update_log_level()
        assert not ret
        assert msg

    @mock.patch('swsscommon.swsscommon.SonicV2Connector')
    def test_runtime_config_publish_fails(self, mock_connector):
        """A failed STATE_DB publish is reported and logged, and the configured level still applies."""
        mock_db = mock.MagicMock()
        mock_db.get = mock.MagicMock(return_value='DEBUG')
        mock_db.hmset = mock.MagicMock(side_effect=Exception('STATE_DB unavailable'))
        mock_connector.return_value = mock_db
        with mock.patch.object(syslogger.SysLogger, 'log_error') as mock_log_error:
            log = syslogger.SysLogger(log_identifier='log3', enable_runtime_config=True)
            mock_log_error.assert_called_once_with('Failed to refresh log configuration - STATE_DB unavailable')
        assert log.logger.level == logging.DEBUG

        assert log.update_log_level() == (False, 'Failed to refresh log configuration - STATE_DB unavailable')
        assert log.logger.level == logging.DEBUG
