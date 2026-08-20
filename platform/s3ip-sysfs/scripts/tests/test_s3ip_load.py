"""
Unit tests for s3ip_load.py - verifies os.system is not used
and all system calls go through subprocess with argument lists.
"""
import json
import os
import subprocess
import unittest
from unittest.mock import patch, MagicMock


SCRIPT_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 's3ip_load.py')

with open(SCRIPT_PATH, 'r') as _f:
    _SCRIPT_SOURCE = _f.read()
_SCRIPT_CODE = compile(_SCRIPT_SOURCE, SCRIPT_PATH, 'exec')


def run_s3ip_load():
    exec(_SCRIPT_CODE, {'__name__': '__main__', '__file__': SCRIPT_PATH})


class TestNoOsSystem(unittest.TestCase):

    def test_no_os_system_in_source(self):
        self.assertNotIn('os.system', _SCRIPT_SOURCE)

    def test_no_shell_true_in_source(self):
        self.assertNotIn('shell=True', _SCRIPT_SOURCE)


class TestS3ipLoadStringType(unittest.TestCase):

    MOCK_CONFIG = {
        "s3ip_syfs_paths": [
            {"path": "/sys_switch/test/file", "type": "string", "value": "hello"}
        ]
    }

    @patch('subprocess.call')
    def test_string_type_calls(self, mock_call):
        file_handles = {}

        def open_side_effect(path, *args, **kwargs):
            if path == '/etc/s3ip/s3ip_sysfs_conf.json':
                handle = MagicMock()
                handle.__enter__ = MagicMock(return_value=handle)
                handle.__exit__ = MagicMock(return_value=False)
                return handle
            else:
                handle = MagicMock()
                handle.__enter__ = MagicMock(return_value=handle)
                handle.__exit__ = MagicMock(return_value=False)
                file_handles[path] = handle
                return handle

        with patch('builtins.open', side_effect=open_side_effect):
            with patch('json.load', return_value=self.MOCK_CONFIG):
                run_s3ip_load()

        expected_calls = [
            (["sudo", "rm", "-rf", "/sys_switch"],),
            (["sudo", "mkdir", "-p", "-m", "777", "/sys_switch"],),
            (["sudo", "mkdir", "-p", "-m", "777", "/sys_switch/test"],),
            (["tree", "-l", "/sys_switch"],),
        ]
        for exp in expected_calls:
            self.assertIn(exp, [c.args for c in mock_call.call_args_list])

        self.assertIn('/sys_switch/test/file', file_handles)
        file_handles['/sys_switch/test/file'].write.assert_called_once_with('hello')

        for c in mock_call.call_args_list:
            self.assertIsInstance(c[0][0], list)


class TestS3ipLoadPathType(unittest.TestCase):

    MOCK_CONFIG = {
        "s3ip_syfs_paths": [
            {"path": "/sys_switch/link", "type": "path", "value": "/some/target"}
        ]
    }

    @patch('subprocess.call')
    def test_path_type_symlink(self, mock_call):
        def open_side_effect(path, *args, **kwargs):
            handle = MagicMock()
            handle.__enter__ = MagicMock(return_value=handle)
            handle.__exit__ = MagicMock(return_value=False)
            return handle

        with patch('builtins.open', side_effect=open_side_effect):
            with patch('json.load', return_value=self.MOCK_CONFIG):
                run_s3ip_load()

        ln_calls = [c for c in mock_call.call_args_list if 'ln' in c[0][0]]
        self.assertEqual(len(ln_calls), 1)
        self.assertEqual(ln_calls[0][0][0], ["sudo", "ln", "-s", "/some/target", "/sys_switch/link"])


class TestS3ipLoadErrorType(unittest.TestCase):

    MOCK_CONFIG = {
        "s3ip_syfs_paths": [
            {"path": "/sys_switch/bad", "type": "unknown", "value": "x"}
        ]
    }

    @patch('subprocess.call')
    @patch('builtins.print')
    def test_error_type_prints(self, mock_print, mock_call):
        def open_side_effect(path, *args, **kwargs):
            handle = MagicMock()
            handle.__enter__ = MagicMock(return_value=handle)
            handle.__exit__ = MagicMock(return_value=False)
            return handle

        with patch('builtins.open', side_effect=open_side_effect):
            with patch('json.load', return_value=self.MOCK_CONFIG):
                run_s3ip_load()

        mock_print.assert_any_call('error type:unknown')


if __name__ == '__main__':
    unittest.main()
