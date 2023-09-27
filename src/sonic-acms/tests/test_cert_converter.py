from unittest import TestCase, mock
import cert_converter

class TestCertConverter(TestCase):

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("cert_converter.convert_certs")
    @mock.patch("os.path.isfile")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_isfile, mock_convert, mock_table, mock_db):
        '''
        Run cert_converter, and break the loop
        Compare exception and check sleep
        '''
        expect_error = Exception("Break the loop")
        mock_isfile.side_effect = [False, True]
        mock_convert.side_effect = [False, True, expect_error]
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        try:
            cert_converter.main()
        except Exception as e:
            self.assertEqual(e, expect_error)
        expect_delay = [mock.call(60), mock.call(cert_converter.polling_frequency)]
        self.assertEqual(mock_sleep.call_args_list, expect_delay)

    @mock.patch("os.path.isfile")
    @mock.patch("os.path.islink")
    @mock.patch("os.listdir")
    def test_cert_list_01(self, mock_listdir, mock_islink, mock_isfile):
        '''
        Generate list of certs
        Verify cert list
        '''
        mock_isfile.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            'sonic_acms_bootstrap_uswest.pfx',
            'restapiserver.crt.1',
            'restapiserver.key.1',
            'restapiserver.crt.2',
            'restapiserver.key.2',
            'temp.crt.3',
            'temp.key'
            ]
        result = sorted(cert_converter.get_list_of_certs(""))
        self.assertEqual(result, ['restapiserver.1', 'restapiserver.2'])

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.execute_cmd")
    @mock.patch("cert_converter.link_to_latest_cert")
    @mock.patch("os.remove")
    def test_converter_01(self, mock_rm, mock_link, mock_cmd, mock_list):
        '''
        Convert cert list
        Check command
        '''
        mock_list.side_effect = [[], ['restapiserver.1', 'restapiserver.6']]
        cert_converter.convert_certs('abc/', 'xyz/', 10)
        self.assertEqual(len(mock_cmd.call_args_list), 6)
        self.assertEqual(mock_cmd.call_args_list[0], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/restapiserver.pfx.1", "-out", "xyz/restapiserver.crt.1", "-password", "pass:", "-passin", "pass:"]))
        self.assertEqual(mock_cmd.call_args_list[3], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/restapiserver.pfx.6", "-out", "xyz/restapiserver.crt.6", "-password", "pass:", "-passin", "pass:"]))

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_set_acms_certs_path_from_db_01(self, mock_table, mock_db):
        '''
        Get certs path from db, no path in db
        Check certs path should be as default
        '''
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        cert_converter.set_acms_certs_path_from_db()
        self.assertEqual(cert_converter.acms_certs_path, "/var/opt/msft/client/dsms/sonic-prod/certificates/chained/")

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_set_acms_certs_path_from_db_02(self, mock_table, mock_db):
        '''
        Get certs path from db, redis connection error
        Check certs path should be as default
        '''
        swsscommon_err = RuntimeError("redis error")
        mock_table.side_effect = [swsscommon_err]
        cert_converter.set_acms_certs_path_from_db()
        self.assertEqual(cert_converter.acms_certs_path, "/var/opt/msft/client/dsms/sonic-prod/certificates/chained/")

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_set_acms_certs_path_from_db_03(self, mock_table, mock_db):
        '''
        Get certs path from db, unknown error
        Check exception should be raised
        '''
        expect_err = Exception("unknown error")
        mock_table.side_effect = [expect_err]
        try:
            cert_converter.set_acms_certs_path_from_db()
        except Exception as e:
            self.assertEqual(e, expect_err)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    def test_set_acms_certs_path_from_db_04(self, mock_table, mock_db):
        '''
        Get certs path from db, path in db
        Check certs path should be as in db
        '''
        mock_table.return_value = {"localhost": (True, (('dsms_cert_path', 'sonick8s-test/certificates/chained/sonick8s'),))}
        mock_db.return_value = None
        cert_converter.set_acms_certs_path_from_db()
        self.assertEqual(cert_converter.acms_certs_path, "/var/opt/msft/client/dsms/sonick8s-test/certificates/chained/sonick8s/")
