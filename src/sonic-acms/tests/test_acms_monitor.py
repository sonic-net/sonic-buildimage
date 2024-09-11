from unittest import TestCase, mock
import acms_monitor

class TestACMSMonitor(TestCase):

    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_cmd):
        '''
        Restart acms process if acms is running and next poll time expired
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=2024-01-01 21:26:11\nSecureBlobEtag=DEADDEAD\n"
        current_date = "2024-01-02 22:00:00\n"
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            (0, dsms_conf, ""),
            (0, current_date, ""),
            (0, "", ""),
            expect_err
        ]
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('cat /var/opt/msft/client/dsms.conf'))
        expected.append(mock.call('date "+%Y-%m-%d %H:%M:%S"'))
        expected.append(mock.call('supervisorctl restart acms'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)


    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_cmd):
        '''
        Do nothing if acms is running and next poll time not expired
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=2024-01-01 21:26:11\nSecureBlobEtag=DEADDEAD\n"
        current_date = "2024-01-02 21:00:00\n"
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            (0, dsms_conf, ""),
            (0, current_date, ""),
            expect_err
        ]
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('cat /var/opt/msft/client/dsms.conf'))
        expected.append(mock.call('date "+%Y-%m-%d %H:%M:%S"'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)


    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_03(self, mock_sleep, mock_cmd):
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
