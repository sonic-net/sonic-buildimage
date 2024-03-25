import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    # Expect the 'mock' package for python 2
    # https://pypi.python.org/pypi/mock
    import mock

from sonic_py_common import logger


class TestLogger:
    def test_log(self):
        log = logger.Logger()
        mock_log = mock.MagicMock()
        mock_log.write = mock.MagicMock()
        mock_log.getMinPrio.return_value = logger.Logger.LOG_PRIORITY_DEBUG

        log._logger = mock_log
        log.set_min_log_priority(logger.Logger.LOG_PRIORITY_DEBUG)

        log.log_debug('debug message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_DEBUG, 'debug message')

        log.log_info('info message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_INFO, 'info message')

        log.log_notice('notice message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_NOTICE, 'notice message')

        log.log_error('error message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_ERROR, 'error message')

        log.log_warning('warning message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_WARNING, 'warning message')

        mock_log.getMinPrio.return_value = logger.Logger.LOG_PRIORITY_INFO
        mock_log.write.reset_mock()
        log.log_debug('debug message')
        mock_log.write.assert_not_called()

        log.log_info('info message', also_print_to_console=True)
        log.log_info('info message')
        mock_log.write.assert_called_with(logger.Logger.LOG_PRIORITY_INFO, 'info message')

    def test_set_log_level(self):
        log = logger.Logger()
        log.set_min_log_priority_error()
        log.set_min_log_priority_warning()
        log.set_min_log_priority_notice()
        log.set_min_log_priority_info()
        log.set_min_log_priority_debug()
