import os
from dateutil import parser
from unittest import TestCase, mock

import bootstrap_monitor

class TestBootstrapMonitor(TestCase):

    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_check_status, mock_get_path):
        '''
        Exit the main loop in sleep
        check_bootstrap_status failed
        '''
        mock_check_status.side_effect = [
            bootstrap_monitor.BootstrapMonitorError("Bootstrap not ready")
        ]
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 0)

    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_check_status, mock_get_path, mock_get_date):
        '''
        Exit the main loop in sleep
        mock_get_path failed
        '''
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        mock_get_path.side_effect = [
            bootstrap_monitor.BootstrapMonitorError("Failed to find bootstrap cert path")
        ]
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 0)

    @mock.patch("bootstrap_monitor.update_state_db")
    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("time.sleep")
    def test_main_03(self, mock_sleep, mock_check_status, mock_get_path, mock_get_date, mock_update_db):
        '''
        Exit the main loop in sleep
        Failed to find region
        '''
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        mock_get_path.return_value = "abc"
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 1)
        self.assertEqual(mock_update_db.call_count, 0)

    @mock.patch("bootstrap_monitor.update_state_db")
    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("time.sleep")
    def test_main_04(self, mock_sleep, mock_check_status, mock_get_path, mock_get_date, mock_update_db):
        '''
        Exit the main loop in mock_update_db
        update_state_db failed
        '''
        expect_err = Exception("unknown error")
        region = "uswest"
        target = "Jan 01 01:01:01 2024 GMT"
        target_time = parser.parse(target, fuzzy=True)
        mock_get_path.return_value = "/tmp/sonic_acms_bootstrap-%s.pfx" % region
        mock_get_date.return_value = target_time
        mock_update_db.side_effect = [
            expect_err
        ]
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 1)
        self.assertEqual(mock_update_db.call_count, 1)
        mock_update_db.assert_called_with(region, str(target_time))

    @mock.patch("bootstrap_monitor.convert_new_cert")
    @mock.patch("bootstrap_monitor.update_state_db")
    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("bootstrap_monitor.execute_cmd")
    @mock.patch("time.sleep")
    def test_main_05(self, mock_sleep, mock_cmd, mock_check_status, mock_get_path, mock_get_date, mock_update_db, mock_con_cert):
        '''
        Exit the main loop in mock_sleep
        Invalid openssl version
        '''
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        region = "uswest"
        target = "Jan 01 01:01:01 2024 GMT"
        target_time = parser.parse(target, fuzzy=True)
        mock_get_path.return_value = "/tmp/sonic_acms_bootstrap-%s.pfx" % region
        mock_get_date.return_value = target_time
        mock_cmd.return_value = (True, "OpenSSL 666")
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 1)
        self.assertEqual(mock_update_db.call_count, 1)
        self.assertEqual(mock_cmd.call_count, 1)
        self.assertEqual(mock_con_cert.call_count, 0)

    @mock.patch("shutil.copy")
    @mock.patch("bootstrap_monitor.convert_new_cert")
    @mock.patch("bootstrap_monitor.update_state_db")
    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("bootstrap_monitor.execute_cmd")
    @mock.patch("time.sleep")
    def test_main_06(self, mock_sleep, mock_cmd, mock_check_status, mock_get_path, mock_get_date, mock_update_db, mock_con_cert, mock_copy):
        '''
        Exit the main loop in mock_sleep
        Replace bootstrap certificate
        '''
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        region = "uswest"
        mock_get_path.return_value = "/tmp/sonic_acms_bootstrap-%s.pfx" % region
        bootstrap_str = "Jan 01 01:01:01 2024 GMT"
        bootstrap_time = parser.parse(bootstrap_str, fuzzy=True)
        new_str = "Jan 02 01:01:01 2024 GMT"
        new_time = parser.parse(new_str, fuzzy=True)
        mock_get_date.side_effect = [
            bootstrap_time,
            new_time
        ]
        mock_cmd.return_value = (True, "OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)")
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 2)
        self.assertEqual(mock_update_db.call_count, 2)
        self.assertEqual(mock_cmd.call_count, 1)
        self.assertEqual(mock_con_cert.call_count, 1)
        self.assertEqual(mock_copy.call_count, 1)

    @mock.patch("shutil.copy")
    @mock.patch("bootstrap_monitor.convert_new_cert")
    @mock.patch("bootstrap_monitor.update_state_db")
    @mock.patch("bootstrap_monitor.get_expire_date")
    @mock.patch("bootstrap_monitor.get_bootstrap_path")
    @mock.patch("bootstrap_monitor.check_bootstrap_status")
    @mock.patch("bootstrap_monitor.execute_cmd")
    @mock.patch("time.sleep")
    def test_main_07(self, mock_sleep, mock_cmd, mock_check_status, mock_get_path, mock_get_date, mock_update_db, mock_con_cert, mock_copy):
        '''
        Exit the main loop in mock_sleep
        Do not replace bootstrap certificate
        '''
        expect_err = Exception("unknown error")
        mock_sleep.side_effect = [
            expect_err
        ]
        region = "uswest"
        mock_get_path.return_value = "/tmp/sonic_acms_bootstrap-%s.pfx" % region
        bootstrap_str = "Jan 02 01:01:01 2024 GMT"
        bootstrap_time = parser.parse(bootstrap_str, fuzzy=True)
        new_str = "Jan 01 01:01:01 2024 GMT"
        new_time = parser.parse(new_str, fuzzy=True)
        mock_get_date.side_effect = [
            bootstrap_time,
            new_time
        ]
        mock_cmd.return_value = (True, "OpenSSL 3.0.15 3 Sep 2024 (Library: OpenSSL 3.0.15 3 Sep 2024)")
        try:
            bootstrap_monitor.main()
        except Exception as e:
            self.assertEqual(e, expect_err)
        self.assertEqual(mock_check_status.call_count, 1)
        self.assertEqual(mock_get_path.call_count, 1)
        self.assertEqual(mock_get_date.call_count, 2)
        self.assertEqual(mock_update_db.call_count, 1)
        self.assertEqual(mock_cmd.call_count, 1)
        self.assertEqual(mock_con_cert.call_count, 1)
        self.assertEqual(mock_copy.call_count, 0)
   
    def test_check_bootstrap_status_01(self):
        '''
        Verify check_bootstrap_status
        DSMS_CONF does not exist
        '''
        bootstrap_monitor.DSMS_CONF = "/tmp/dsms.conf"
        if os.path.exists(bootstrap_monitor.DSMS_CONF):
            os.remove(bootstrap_monitor.DSMS_CONF)
        try:
            bootstrap_monitor.check_bootstrap_status()
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Bootstrap not ready" in str(e)

    def test_check_bootstrap_status_02(self):
        '''
        Verify check_bootstrap_status
        DSMS_CONF exists and HasBootstrapped=no
        '''
        bootstrap_monitor.DSMS_CONF = "/tmp/dsms.conf"
        with open(bootstrap_monitor.DSMS_CONF, "w") as fp:
            fp.write("deaddead\nHasBootstrapped=no\ndeaddead\n")
        try:
            bootstrap_monitor.check_bootstrap_status()
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Bootstrap not ready" in str(e)

    def test_check_bootstrap_status_03(self):
        '''
        Verify check_bootstrap_status
        DSMS_CONF exists and HasBootstrapped=yes
        '''
        bootstrap_monitor.DSMS_CONF = "/tmp/dsms.conf"
        with open(bootstrap_monitor.DSMS_CONF, "w") as fp:
            fp.write("deaddead\nHasBootstrapped=yes\ndeaddead\n")
        bootstrap_monitor.check_bootstrap_status()

    def test_get_bootstrap_path_01(self):
        '''
        Verify get_bootstrap_path
        ACMS_SEC_CONF does not exist
        '''
        bootstrap_monitor.ACMS_SEC_CONF = "/tmp/acms.conf"
        if os.path.exists(bootstrap_monitor.ACMS_SEC_CONF):
            os.remove(bootstrap_monitor.ACMS_SEC_CONF)
        try:
            bootstrap_monitor.get_bootstrap_path()
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to find bootstrap cert path" in str(e)

    def test_get_bootstrap_path_02(self):
        '''
        Verify get_bootstrap_status
        ACMS_SEC_CONF exists and BootstrapCert=
        '''
        bootstrap_monitor.ACMS_SEC_CONF = "/tmp/acms.conf"
        with open(bootstrap_monitor.ACMS_SEC_CONF, "w") as fp:
            fp.write("deaddead\nBootstrapCert=\ndeaddead\n")
        try:
            bootstrap_monitor.get_bootstrap_path()
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to find bootstrap cert path" in str(e)

    def test_get_bootstrap_path_03(self):
        '''
        Verify get_bootstrap_status
        ACMS_SEC_CONF exists and BootstrapCert=/tmp/target.pfx
        '''
        bootstrap_monitor.ACMS_SEC_CONF = "/tmp/acms.conf"
        target = "/tmp/target.pfx"
        with open(bootstrap_monitor.ACMS_SEC_CONF, "w") as fp:
            fp.write("deaddead\nBootstrapCert=%s\ndeaddead\n" % target)
        ret = bootstrap_monitor.get_bootstrap_path()
        self.assertEqual(ret, target)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_get_expire_date_01(self, mock_cmd):
        '''
        Verify get_expire_date
        Raise exception in the first command
        '''
        mock_cmd.side_effect = [
            (False, ""),
            (False, "")
        ]
        try:
            ret = bootstrap_monitor.get_expire_date("dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to decode" in str(e)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_get_expire_date_02(self, mock_cmd):
        '''
        Verify get_expire_date
        Raise exception in the second command
        '''
        mock_cmd.side_effect = [
            (True, "abc"),
            (False, "")
        ]
        try:
            ret = bootstrap_monitor.get_expire_date("dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to read" in str(e)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_get_expire_date_03(self, mock_cmd):
        '''
        Verify get_expire_date
        Raise exception for invalid date
        '''
        mock_cmd.side_effect = [
            (True, "abc"),
            (True, "notAfter=0123456789 GMT")
        ]
        try:
            ret = bootstrap_monitor.get_expire_date("dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to parse" in str(e)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_get_expire_date_04(self, mock_cmd):
        '''
        Verify get_expire_date
        '''
        target = "Jan 01 01:01:01 2024 GMT"
        target_time = parser.parse(target, fuzzy=True)
        mock_cmd.side_effect = [
            (True, "abc"),
            (True, "notAfter=" + target)
        ]
        ret = bootstrap_monitor.get_expire_date("dummy")
        self.assertEqual(ret, target_time)

    def test_convert_new_cert_01(self):
        '''
        Verify convert_new_cert
        Notify file does not exist
        '''
        notify_file = "/tmp/dummy.notify"
        if os.path.exists(notify_file):
            os.remove(notify_file)
        try:
            bootstrap_monitor.convert_new_cert(notify_file, "dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Notify file does not exist" in str(e)

    def test_convert_new_cert_02(self):
        '''
        Verify convert_new_cert
        Secret file does not exist
        '''
        notify_file = "/tmp/dummy.notify"
        secret_file = "/tmp/dummy.1"
        with open(notify_file, "w") as fp:
            fp.write(secret_file)
        if os.path.exists(secret_file):
            os.remove(secret_file)
        try:
            bootstrap_monitor.convert_new_cert(notify_file, "dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Secret file does not exist" in str(e)

    def test_convert_new_cert_03(self):
        '''
        Verify convert_new_cert
        Failed to decode base64
        '''
        notify_file = "/tmp/dummy.notify"
        secret_file = "/tmp/dummy.1"
        with open(notify_file, "w") as fp:
            fp.write(secret_file)
        with open(secret_file, "w") as fp:
            fp.write("invalid")
        try:
            bootstrap_monitor.convert_new_cert(notify_file, "dummy")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Decode error" in str(e)

    def test_convert_new_cert_04(self):
        '''
        Verify convert_new_cert
        '''
        notify_file = "/tmp/dummy.notify"
        secret_file = "/tmp/dummy.1"
        target_file = "/tmp/target.pfx"
        with open(notify_file, "w") as fp:
            fp.write(secret_file)
        with open(secret_file, "w") as fp:
            fp.write("SGVsbG8gV29ybGQh")
        bootstrap_monitor.convert_new_cert(notify_file, target_file)
        self.assertEqual(os.path.exists(target_file), True)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_update_state_db_01(self, mock_cmd):
        '''
        Verify update_state_db
        Raise exception in the first command
        '''
        mock_cmd.side_effect = [
            (False, ""),
            (False, "")
        ]
        try:
            bootstrap_monitor.update_state_db("dummy", "123")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to update region" in str(e)

    @mock.patch("bootstrap_monitor.execute_cmd")
    def test_update_state_db_02(self, mock_cmd):
        '''
        Verify update_state_db
        Raise exception in the second command
        '''
        mock_cmd.side_effect = [
            (True, ""),
            (False, "")
        ]
        try:
            bootstrap_monitor.update_state_db("dummy", "123")
        except bootstrap_monitor.BootstrapMonitorError as e:
            assert "Failed to update date" in str(e)
