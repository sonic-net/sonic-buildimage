import logging
import os
import runpy
import sys
from unittest import mock

import pytest
from supervisor.compat import xmlrpclib
from supervisor.xmlrpc import Faults


SCRIPT = os.path.join(os.path.dirname(__file__), '..', 'scripts', 'supervisor-dependent-startup')


@pytest.fixture
def startup(monkeypatch):
    # The upstream engine is installed by the container, not this utility package.
    engine = mock.Mock(log=logging.getLogger('supervisord_dependent_startup'))
    package = mock.Mock(supervisord_dependent_startup=engine)
    monkeypatch.setitem(sys.modules, 'supervisord_dependent_startup', package)
    return engine


@pytest.mark.parametrize('error, exit_code', [
    (xmlrpclib.Fault(Faults.SHUTDOWN_STATE, 'SHUTDOWN_STATE'), 0),
    (xmlrpclib.Fault(Faults.FAILED, 'FAILED'), 3),
    (RuntimeError('startup failed'), 3),
    (SystemExit(0), 3),
    (SystemExit(2), 3),
    (KeyboardInterrupt(), 3),
    (BaseException('interrupted'), 3),
])
def test_exception(startup, caplog, error, exit_code):
    startup.main.side_effect = error
    with pytest.raises(SystemExit) as result:
        runpy.run_path(SCRIPT, run_name='__main__')
    assert result.value.code == exit_code
    startup.main.assert_called_once_with()
    if exit_code == 0:
        assert not caplog.records
    else:
        record, = caplog.records
        assert record.levelno == logging.ERROR
        assert record.getMessage() == 'Error occured:'
        assert record.exc_info[1] is error
        assert 'Traceback (most recent call last)' in caplog.text


@pytest.mark.parametrize('return_value', [None, 0, 2, 4])
def test_normal_return_is_ignored(startup, caplog, return_value):
    startup.main.return_value = return_value
    with pytest.raises(SystemExit) as result:
        runpy.run_path(SCRIPT, run_name='__main__')
    assert result.value.code == 0
    startup.main.assert_called_once_with()
    assert not caplog.records
