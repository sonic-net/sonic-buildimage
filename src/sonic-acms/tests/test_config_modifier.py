from unittest import TestCase, mock
import dSMS_config_modifier

class TestConfigModifier(TestCase):

    def test_fix_endpoint_01(self):
        '''
        Fix endpoint and cloud type is abc
        Should return False
        '''
        ret = dSMS_config_modifier.fix_endpoint_for_cloud("abc")
        self.assertEqual(ret, False)

    @mock.patch("dSMS_config_modifier.update_dsms_url")
    def test_fix_endpoint_02(self, mock_url):
        '''
        Fix endpoint and cloud type is Fairfax
        Should return True
        '''
        ret = dSMS_config_modifier.fix_endpoint_for_cloud("Fairfax")
        self.assertEqual(ret, True)
        expected = [(mock.call("/var/opt/msft/client/acms_secrets.ini", "https://region-dsms.dsms.core.usgovcloudapi.net"))]
        self.assertEqual(mock_url.call_args_list, expected)

    @mock.patch("dSMS_config_modifier.update_dsms_url")
    def test_fix_endpoint_03(self, mock_url):
        '''
        Fix endpoint and cloud type is Mooncake
        Should return True
        '''
        ret = dSMS_config_modifier.fix_endpoint_for_cloud("Mooncake")
        self.assertEqual(ret, True)
        expected = [(mock.call("/var/opt/msft/client/acms_secrets.ini", "https://region-dsms.dsms.core.chinacloudapi.cn"))]
        self.assertEqual(mock_url.call_args_list, expected)

    @mock.patch("dSMS_config_modifier.update_dsms_url")
    def test_fix_endpoint_04(self, mock_url):
        '''
        Fix endpoint and cloud type is USnat
        Should return True
        '''
        ret = dSMS_config_modifier.fix_endpoint_for_cloud("USnat")
        self.assertEqual(ret, True)
        expected = [(mock.call("/var/opt/msft/client/acms_secrets.ini", "https://region-dsms.dsms.core.eaglex.ic.gov"))]
        self.assertEqual(mock_url.call_args_list, expected)

    @mock.patch("dSMS_config_modifier.update_dsms_url")
    def test_fix_endpoint_05(self, mock_url):
        '''
        Fix endpoint and cloud type is USSec
        Should return True
        '''
        ret = dSMS_config_modifier.fix_endpoint_for_cloud("USSec")
        self.assertEqual(ret, True)
        expected = [(mock.call("/var/opt/msft/client/acms_secrets.ini", "https://region-dsms.dsms.core.microsoft.scloud"))]
        self.assertEqual(mock_url.call_args_list, expected)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_cloudtype_01(self, mock_table, mock_conn):
        '''
        Get cloud type
        Should return public
        '''
        mock_table.return_value.get = mock.MagicMock()
        mock_table.return_value.get.return_value = (True, [('cloudtype', 'Public')])
        cloudtype = dSMS_config_modifier.get_device_cloudtype()
        self.assertEqual(cloudtype, 'Public')

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_cloudtype_02(self, mock_table, mock_conn):
        '''
        Get cloud type and raise exception
        Should return emptry string
        '''
        swsscommon_err = RuntimeError("redis error")
        mock_table.side_effect = [swsscommon_err]
        cloudtype = dSMS_config_modifier.get_device_cloudtype()
        self.assertEqual(cloudtype, '')

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_cloudtype_03(self, mock_table, mock_conn):
        '''
        Get cloud type and raise exception
        Should raise exception
        '''
        expect_err = Exception("unknown error")
        mock_table.side_effect = [expect_err]
        try:
            dSMS_config_modifier.get_device_cloudtype()
        except Exception as e:
            self.assertEqual(e, expect_err)
