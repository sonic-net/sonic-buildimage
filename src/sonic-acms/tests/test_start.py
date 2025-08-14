from unittest import TestCase, mock
import subprocess
import start

class TestStart(TestCase):

    @mock.patch("start.exec_cmd")
    @mock.patch("os.makedirs")
    @mock.patch("shutil.copy2")
    @mock.patch("os.path.isfile")
    @mock.patch("start.get_bootstrap_status")
    def test_main_01(self, mock_bootstrap_status, mock_isfile, mock_copy, mock_makedirs, mock_cmd):
        '''
        Skip bootstrap and start
        Verify exec_cmd
        '''
        mock_isfile.return_value = True
        mock_bootstrap_status.return_value = True
        mock_cmd.return_value = (0, "", "")
        start.main()
        expected = []
        expected.append(mock.call('supervisorctl start rsyslogd'))
        expected.append(mock.call('supervisorctl start acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)

    @mock.patch("start.exec_cmd")
    @mock.patch("os.makedirs")
    @mock.patch("shutil.copy2")
    @mock.patch("os.path.isfile")
    @mock.patch("start.get_bootstrap_status")
    @mock.patch("os.remove")
    @mock.patch("start.get_bootstrap_cert_paths")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_cert_paths, mock_rm, mock_bootstrap_status, mock_isfile, mock_copy, mock_makedirs, mock_cmd):
        '''
        Bootstrap for the first time and start, no bootstrap cert
        Verify exec_cmd
        '''
        mock_isfile.return_value = True
        mock_bootstrap_status.side_effect = [False, True]
        mock_cmd.return_value = (0, "", "")
        mock_cert_paths.return_value = []
        start.main()
        expected = []
        expected.append(mock.call('supervisorctl start rsyslogd'))
        expected.append(mock.call('supervisorctl start acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)

    @mock.patch("start.exec_cmd")
    @mock.patch("os.makedirs")
    @mock.patch("shutil.copy2")
    @mock.patch("os.path.isfile")
    @mock.patch("start.get_bootstrap_status")
    @mock.patch("os.remove")
    @mock.patch("start.get_bootstrap_cert_paths")
    @mock.patch("time.sleep")
    @mock.patch("start.update_acms_config")
    def test_main_03(self, mock_acms_config, mock_sleep, mock_cert_paths, mock_rm, mock_bootstrap_status, mock_isfile, mock_copy, mock_makedirs, mock_cmd):
        '''
        Bootstrap for multiple times and start, valid bootstrap cert
        Verify exec_cmd and back-off delay
        '''
        count = 100
        mock_isfile.return_value = True
        result_list = []
        for i in range(count):
            result_list.append(False)
        result_list.append(True)
        mock_bootstrap_status.side_effect = result_list
        mock_cmd.return_value = (0, "", "")
        mock_cert_paths.return_value = [('sonic_acms_bootstrap-abc.pfx', 0)]
        mock_acms_config.return_value = True
        start.main()
        expected = []
        expected.append(mock.call('supervisorctl start rsyslogd'))
        for i in range(count):
            expected.append(mock.call('/usr/bin/acms -Bootstrap -Dependant client -BaseDirPath /var/opt/msft/', timeout=3600))
        expected.append(mock.call('supervisorctl start acms'))
        self.assertEqual(mock_cmd.call_args_list, expected)
        expected_delay = []
        curr = 60
        for i in range(count):
            expected_delay.append(mock.call(curr))
            curr = curr * 2
            if curr > 3600:
                curr = 3600
        self.assertEqual(mock_sleep.call_args_list, expected_delay)

    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    def test_acms_config_01(self, mock_cloud_type):
        '''
        Update acms config when cloud type is empty
        Should return False
        '''
        mock_cloud_type.return_value = ''
        ret = start.update_acms_config("test.cert")
        self.assertEqual(ret, False)

    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    def test_acms_config_02(self, mock_cloud_type):
        '''
        Update acms config when cloud type is None
        Should return False
        '''
        mock_cloud_type.return_value = 'None'
        ret = start.update_acms_config("test.cert")
        self.assertEqual(ret, False)

    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    @mock.patch("dSMS_config_modifier.update_config")
    def test_acms_config_03(self, mock_update_config, mock_cloud_type):
        '''
        Update acms config when cloud type is public
        Should return True
        '''
        mock_cloud_type.return_value = 'Public'
        ret = start.update_acms_config("test.cert")
        self.assertEqual(ret, True)

    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    @mock.patch("dSMS_config_modifier.update_config")
    @mock.patch("dSMS_config_modifier.fix_endpoint_for_cloud")
    def test_acms_config_04(self, mock_fix_endpoint, mock_update_config, mock_cloud_type):
        '''
        Update acms config when cloud type is mooncake
        Should return True
        '''
        mock_fix_endpoint.return_value = True
        mock_cloud_type.return_value = 'Mooncake'
        ret = start.update_acms_config("test.cert")
        self.assertEqual(ret, True)

    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    @mock.patch("dSMS_config_modifier.update_config")
    @mock.patch("dSMS_config_modifier.fix_endpoint_for_cloud")
    def test_acms_config_05(self, mock_fix_endpoint, mock_update_config, mock_cloud_type):
        '''
        Update acms config when cloud type is mooncake, and fix_endpoint failed
        Should return True to support unknown cloud type
        '''
        mock_fix_endpoint.return_value = False
        mock_cloud_type.return_value = 'Mooncake'
        ret = start.update_acms_config("test.cert")
        self.assertEqual(ret, True)

    def test_cmd_01(self):
        '''
        Verify exec_cmd
        '''
        rc, stdoutdata, stderrdata = start.exec_cmd("echo abc")
        self.assertEqual(rc, 0)
        output = stdoutdata.strip()
        self.assertEqual(output, "abc")

    @mock.patch("subprocess.run")
    def test_cmd_02(self, mock_run):
        '''
        Verify exec_cmd
        '''
        expect_err = subprocess.TimeoutExpired("dummy timeout", timeout=0)
        mock_run.side_effect = [expect_err]
        rc, stdoutdata, stderrdata = start.exec_cmd("echo abc")
        self.assertEqual(rc, -1)
