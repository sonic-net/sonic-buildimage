from unittest import TestCase, mock
import datetime

import acms_monitor

class TestACMSMonitor(TestCase):

    @mock.patch("acms_monitor.get_now_time")
    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_cmd, mock_now):
        '''
        Restart acms process if acms is running and next poll time expired
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        acms_monitor.DSMS_CONFIG = "/tmp/dsms.conf"
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=2024-01-01 21:26:11\nSecureBlobEtag=DEADDEAD\n"
        with open(acms_monitor.DSMS_CONFIG, "w") as fp:
            fp.write(dsms_conf)
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            (0, "", ""),
            expect_err
        ]
        mock_now.return_value = datetime.datetime(2024, 1, 2, 22, 0, 0)
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
        Restart acms process if acms is running and next poll time expired
        Do not mock datetime
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        acms_monitor.DSMS_CONFIG = "/tmp/dsms.conf"
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=2024-01-01 21:26:11\nSecureBlobEtag=DEADDEAD\n"
        with open(acms_monitor.DSMS_CONFIG, "w") as fp:
            fp.write(dsms_conf)
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


    @mock.patch("acms_monitor.get_now_time")
    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_03(self, mock_sleep, mock_cmd, mock_now):
        '''
        Do nothing if acms is running and next poll time not expired
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        acms_monitor.DSMS_CONFIG = "/tmp/dsms.conf"
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=2024-01-01 21:26:11\nSecureBlobEtag=DEADDEAD\n"
        with open(acms_monitor.DSMS_CONFIG, "w") as fp:
            fp.write(dsms_conf)
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            expect_err
        ]
        mock_now.return_value = datetime.datetime(2024, 1, 2, 21, 0, 0)
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)


    @mock.patch("acms_monitor.get_now_time")
    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_04(self, mock_sleep, mock_cmd, mock_now):
        '''
        Do nothing if acms is running and next poll time not valid
        Verify exec_cmd input
        '''
        expect_err = Exception("unknown error")
        acms_monitor.DSMS_CONFIG = "/tmp/dsms.conf"
        dsms_conf = "LastPollUtc=2024-01-01 04:47:40\nNextPollUtc=DEADDEAD\nSecureBlobEtag=DEADDEAD\n"
        with open(acms_monitor.DSMS_CONFIG, "w") as fp:
            fp.write(dsms_conf)
        mock_cmd.side_effect = [
            (0, "acms   RUNNING   pid 88, uptime 8 days", ""),
            expect_err
        ]
        mock_now.return_value = datetime.datetime(2024, 1, 2, 21, 0, 0)
        try:
            acms_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        expected = []
        expected.append(mock.call('supervisorctl status acms'))
        expected.append(mock.call('supervisorctl status acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)


    @mock.patch("acms_monitor.exec_cmd")
    @mock.patch("time.sleep")
    def test_main_05(self, mock_sleep, mock_cmd):
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
