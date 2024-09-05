from unittest import TestCase, mock
import acms_monitor

class TestACMSMonitor(TestCase):

    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_cmd):
        '''
        Restart acms process if acms is running
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            (0, "", ""),
            expect_err
        ]
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('supervisorctl restart acms'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)


    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_cmd):
        '''
        Do nothing if acms is not running
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        mock_cmd.side_effect = [
            (0, "acms   STOPPED   Not started", ""),
            expect_err
        ]
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)
