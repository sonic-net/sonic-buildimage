#!/usr/bin/env python3
"""Exercise Redis startup races without using a host Redis service."""
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('eventd_run_tests', Path(__file__).with_name('run_tests.py'))
run_tests = importlib.util.module_from_spec(spec)
spec.loader.exec_module(run_tests)


class RedisStartupTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.logs = []

    def process(self, message='', running=False):
        process = mock.Mock(pid=1234)
        process.poll.return_value = None if running else 1

        def launch(*args, **kwargs):
            log = kwargs['stdout']
            self.logs.append(log)
            log.write(message.format(port=args[0][4]))
            log.flush()
            return process

        return process, launch

    def client(self, pid=1234):
        body = ('process_id:' + str(pid) + '\r\n').encode()
        client = mock.MagicMock()
        client.__enter__.return_value = client
        client.makefile.return_value = io.BytesIO(b'$' + str(len(body)).encode() + b'\r\n' + body)
        return client

    def start(self, **kwargs):
        return run_tests.start_redis('/declared/redis-server', self.root,
                                     str(self.root / 'redis.sock'), {}, **kwargs)

    def test_collision_retries_another_port_and_returns_owned_server(self):
        failed, collision = self.process('TCP listening socket 127.0.0.1:{port}: bind: Address already in use\n')
        healthy, success = self.process(running=True)
        launches = iter([collision, success])
        with mock.patch.object(run_tests, 'unused_port', side_effect=[10001, 10002]), \
                mock.patch.object(run_tests.subprocess, 'Popen', side_effect=lambda *a, **kw: next(launches)(*a, **kw)) as popen, \
                mock.patch.object(run_tests.socket, 'create_connection', return_value=self.client()):
            process, log, port = self.start()
        self.addCleanup(log.close)
        self.assertIs(process, healthy)
        self.assertEqual(port, 10002)
        self.assertTrue(self.logs[0].closed)
        self.assertFalse(log.closed)
        self.assertEqual([call.args[0][4] for call in popen.call_args_list], ['10001', '10002'])
        failed.terminate.assert_not_called()

    def test_other_startup_failure_is_not_retried(self):
        _, launch = self.process('Fatal configuration error\n')
        with mock.patch.object(run_tests.subprocess, 'Popen', side_effect=launch) as popen:
            with self.assertRaisesRegex(ValueError, 'Fatal configuration error'):
                self.start()
        self.assertEqual(popen.call_count, 1)
        self.assertTrue(self.logs[0].closed)

    def test_collision_retries_are_bounded(self):
        _, launch = self.process('TCP listening socket 127.0.0.1:{port}: bind: Address already in use\n')
        with mock.patch.object(run_tests.subprocess, 'Popen', side_effect=launch) as popen:
            with self.assertRaisesRegex(ValueError, 'Address already in use'):
                self.start(attempts=3)
        self.assertEqual(popen.call_count, 3)
        self.assertTrue(all(log.closed for log in self.logs))

    def test_unix_socket_collision_is_not_retried_with_a_new_tcp_port(self):
        _, launch = self.process('Unix listening socket /tmp/redis.sock: bind: Address already in use\n')
        with mock.patch.object(run_tests.subprocess, 'Popen', side_effect=launch) as popen:
            with self.assertRaisesRegex(ValueError, 'Address already in use'):
                self.start()
        self.assertEqual(popen.call_count, 1)
        self.assertTrue(self.logs[0].closed)

    def test_foreign_server_identity_is_rejected_and_child_stopped(self):
        process, launch = self.process(running=True)
        with mock.patch.object(run_tests.subprocess, 'Popen', side_effect=launch) as popen, \
                mock.patch.object(run_tests.socket, 'create_connection', return_value=self.client(pid=4321)), \
                mock.patch.object(run_tests.time, 'monotonic', side_effect=[0, 1, 11]), \
                mock.patch.object(run_tests.time, 'sleep'):
            with self.assertRaisesRegex(ValueError, 'Private Redis startup failed'):
                self.start()
        self.assertEqual(popen.call_count, 1)
        process.terminate.assert_called_once()
        process.wait.assert_called_once_with(timeout=5)
        self.assertTrue(self.logs[0].closed)

    def test_spawn_error_closes_log(self):
        with mock.patch.object(run_tests.subprocess, 'Popen', side_effect=OSError('cannot execute')) as popen:
            with self.assertRaisesRegex(OSError, 'cannot execute'):
                self.start()
        self.assertTrue(popen.call_args.kwargs['stdout'].closed)


if __name__ == '__main__':
    unittest.main()
