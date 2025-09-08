from unittest import TestCase, mock
import cert_converter

class TestCertConverter(TestCase):

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("cert_converter.convert_all_certs")
    @mock.patch("os.path.isfile")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_isfile, mock_convert, mock_table, mock_db):
        '''
        Run cert_converter, wait for notify file, then break the polling loop
        Compare exception and check sleep
        '''
        expect_error = Exception("Break the loop")
        # First call returns False (no notify file), second call returns True (notify file found)
        mock_isfile.side_effect = [False, True]
        # Convert calls: first after notify succeeds, second in polling raises exception
        mock_convert.side_effect = [None, expect_error]
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        try:
            cert_converter.main()
        except Exception as e:
            self.assertEqual(e, expect_error)
        # Should have sleep(60) when waiting for notify file, but no polling sleep since exception occurs
        expected_calls = [mock.call(60)]
        self.assertEqual(mock_sleep.call_args_list, expected_calls)

    @mock.patch("os.path.exists")
    @mock.patch("os.path.isfile")
    @mock.patch("os.path.islink")
    @mock.patch("os.listdir")
    def test_cert_list_01(self, mock_listdir, mock_islink, mock_isfile, mock_exists):
        '''
        Generate list of certs
        Verify cert list
        '''
        mock_exists.return_value = True
        mock_isfile.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            'sonic_acms_bootstrap_uswest.pfx',
            'restapiserver.crt.1',
            'restapiserver.key.1',
            'restapiserver.crt.2',
            'restapiserver.key.2',
            'private.key',
            'temp.crt.3',
            'temp.key'
            ]
        result = sorted(cert_converter.get_list_of_certs(""))
        self.assertEqual(result, ['restapiserver.1', 'restapiserver.2'])

    @mock.patch("os.path.exists")
    @mock.patch("os.path.isfile")
    @mock.patch("os.path.islink")
    @mock.patch("os.listdir")
    def test_cert_list_02(self, mock_listdir, mock_islink, mock_isfile, mock_exists):
        '''
        Generate list of certs
        Verify cert list
        '''
        mock_exists.return_value = True
        mock_isfile.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            'gnmiserver.crt.1',
            'gnmiserver.key.1',
            'gnmiserver.crt.2',
            'gnmiserver.key.2'
            ]
        result = sorted(cert_converter.get_list_of_certs(""))
        self.assertEqual(result, ['gnmiserver.1', 'gnmiserver.2'])

    def test_cert_list_03(self):
        '''
        Generate list of certs for invalid path
        Verify cert list
        '''
        result = cert_converter.get_list_of_certs("/invalid/path/")
        self.assertEqual(result, [])

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.execute_cmd")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_converter_01(self, mock_link, mock_cmd, mock_list):
        '''
        Convert cert list with prefix matching
        Check command
        '''
        mock_cmd.return_value = (True, "")
        mock_link.return_value = True
        mock_list.side_effect = [[], ['restapiserver.1', 'restapiserver.6']]
        result = cert_converter.convert_certs_with_prefix('abc/', 'restapiserver', 'xyz/', 10)
        self.assertTrue(result)
        self.assertEqual(len(mock_cmd.call_args_list), 6)
        self.assertEqual(mock_cmd.call_args_list[0], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/restapiserver.pfx.1", "-out", "xyz/restapiserver.crt.1", "-password", "pass:", "-passin", "pass:"]))
        self.assertEqual(mock_cmd.call_args_list[3], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/restapiserver.pfx.6", "-out", "xyz/restapiserver.crt.6", "-password", "pass:", "-passin", "pass:"]))

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.execute_cmd")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_converter_02(self, mock_link, mock_cmd, mock_list):
        '''
        Convert cert list with gnmi prefix
        Check command
        '''
        mock_cmd.return_value = (True, "")
        mock_link.return_value = True
        mock_list.side_effect = [[], ['gnmiserver.1', 'gnmiserver.6']]
        result = cert_converter.convert_certs_with_prefix('abc/', 'gnmiserver', 'xyz/', 10)
        self.assertTrue(result)
        self.assertEqual(len(mock_cmd.call_args_list), 6)
        self.assertEqual(mock_cmd.call_args_list[0], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/gnmiserver.pfx.1", "-out", "xyz/gnmiserver.crt.1", "-password", "pass:", "-passin", "pass:"]))
        self.assertEqual(mock_cmd.call_args_list[3], mock.call(["openssl", "pkcs12", "-clcerts", "-nokeys", "-in", "abc/gnmiserver.pfx.6", "-out", "xyz/gnmiserver.crt.6", "-password", "pass:", "-passin", "pass:"]))

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

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("os.remove")
    @mock.patch("os.path.exists")
    @mock.patch("os.path.islink")
    @mock.patch("os.path.isfile")
    @mock.patch("os.listdir")
    def test_clean_cert_01(self, mock_listdir, mock_isfile, mock_islink, mock_exists, mock_remove, mock_table, mock_db):
        '''
        clean link file in certs path
        Compare removed file
        '''
        mock_exists.return_value = True
        mock_islink.return_value = True
        mock_listdir.return_value = ["restapiserver.crt", "restapiserver.key"]
        mock_isfile.return_value = True
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        cert_converter.clean_current_certs("/dummy/")
        expect_remove = [mock.call("/dummy/restapiserver.crt"), mock.call("/dummy/restapiserver.key")]
        self.assertEqual(mock_remove.call_args_list, expect_remove)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("os.remove")
    @mock.patch("os.path.exists")
    @mock.patch("os.path.islink")
    @mock.patch("os.path.isfile")
    @mock.patch("os.listdir")
    def test_clean_cert_02(self, mock_listdir, mock_isfile, mock_islink, mock_exists, mock_remove, mock_table, mock_db):
        '''
        clean crt file and key file in certs path
        Compare removed file
        '''
        mock_exists.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            "restapiserver.crt.11",
            "restapiserver.crt.12",
            "restapiserver.key.11",
            "restapiserver.key.12"
            ]
        mock_isfile.return_value = True
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        cert_converter.clean_current_certs("/dummy/")
        expect_remove = [
            mock.call("/dummy/restapiserver.crt.11"),
            mock.call("/dummy/restapiserver.key.11"),
            mock.call("/dummy/restapiserver.crt.12"),
            mock.call("/dummy/restapiserver.key.12")
            ]
        expect_remove.sort()
        remove_list = mock_remove.call_args_list
        remove_list.sort()
        self.assertEqual(remove_list, expect_remove)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("os.remove")
    @mock.patch("os.path.exists")
    @mock.patch("os.path.islink")
    @mock.patch("os.path.isfile")
    @mock.patch("os.listdir")
    def test_clean_cert_03(self, mock_listdir, mock_isfile, mock_islink, mock_exists, mock_remove, mock_table, mock_db):
        '''
        clean crt file and key file in certs path
        Compare removed file
        '''
        mock_exists.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            "gnmiserver.crt.11",
            "gnmiserver.crt.12",
            "gnmiserver.key.11",
            "gnmiserver.key.12"
            ]
        mock_isfile.return_value = True
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        cert_converter.clean_current_certs("/dummy/")
        expect_remove = [
            mock.call("/dummy/gnmiserver.crt.11"),
            mock.call("/dummy/gnmiserver.key.11"),
            mock.call("/dummy/gnmiserver.crt.12"),
            mock.call("/dummy/gnmiserver.key.12")
            ]
        expect_remove.sort()
        remove_list = mock_remove.call_args_list
        remove_list.sort()
        self.assertEqual(remove_list, expect_remove)

    @mock.patch("cert_converter.execute_cmd")
    def test_convert_single_cert_success(self, mock_cmd):
        '''
        Test successful single certificate conversion
        '''
        mock_cmd.return_value = (True, "private_key_data")
        result = cert_converter.convert_single_cert("testcert.1", "/source/", "/dest/")
        self.assertTrue(result)
        self.assertEqual(len(mock_cmd.call_args_list), 3)
        
        # Verify the commands called
        calls = mock_cmd.call_args_list
        # First call: extract certificate
        self.assertIn("-clcerts", calls[0][0][0])
        # Second call: extract private key  
        self.assertIn("-nocerts", calls[1][0][0])
        # Third call: decrypt private key
        self.assertIn("rsa", calls[2][0][0])

    @mock.patch("cert_converter.execute_cmd")
    def test_convert_single_cert_fail_crt(self, mock_cmd):
        '''
        Test single certificate conversion failure at crt extraction
        '''
        mock_cmd.return_value = (False, "")
        result = cert_converter.convert_single_cert("testcert.1", "/source/", "/dest/")
        self.assertFalse(result)
        self.assertEqual(len(mock_cmd.call_args_list), 1)

    @mock.patch("cert_converter.execute_cmd")
    def test_convert_single_cert_fail_key_extract(self, mock_cmd):
        '''
        Test single certificate conversion failure at key extraction
        '''
        mock_cmd.side_effect = [(True, ""), (False, "")]
        result = cert_converter.convert_single_cert("testcert.1", "/source/", "/dest/")
        self.assertFalse(result)
        self.assertEqual(len(mock_cmd.call_args_list), 2)

    @mock.patch("cert_converter.execute_cmd")
    def test_convert_single_cert_fail_key_decrypt(self, mock_cmd):
        '''
        Test single certificate conversion failure at key decryption
        '''
        mock_cmd.side_effect = [(True, ""), (True, "private_key"), (False, "")]
        result = cert_converter.convert_single_cert("testcert.1", "/source/", "/dest/")
        self.assertFalse(result)
        self.assertEqual(len(mock_cmd.call_args_list), 3)

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_with_prefix_no_certs(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_with_prefix with no certificates
        '''
        mock_list.side_effect = [[], []]
        result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
        self.assertTrue(result)
        mock_convert.assert_not_called()
        mock_link.assert_not_called()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_with_prefix_already_converted(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_with_prefix with already converted certificates
        '''
        mock_list.side_effect = [["restapiserver.1"], ["restapiserver.1"]]
        result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
        self.assertTrue(result)
        mock_convert.assert_not_called()
        mock_link.assert_not_called()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_with_prefix_conversion_failure(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_with_prefix with conversion failure
        '''
        mock_list.side_effect = [[], ["restapiserver.1"]]
        mock_convert.return_value = False
        result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
        self.assertFalse(result)
        mock_link.assert_not_called()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_with_prefix_link_failure(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_with_prefix with linking failure
        '''
        mock_list.side_effect = [[], ["restapiserver.1"]]
        mock_convert.return_value = True
        mock_link.return_value = False
        result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
        self.assertFalse(result)

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_with_prefix_success(self, mock_link, mock_convert, mock_list):
        '''
        Test successful convert_certs_with_prefix
        '''
        mock_list.side_effect = [[], ["restapiserver.1", "othercert.2"]]
        mock_convert.return_value = True
        mock_link.return_value = True
        result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
        self.assertTrue(result)
        mock_convert.assert_called_once_with("restapiserver.1", "/source/", "/dest/")
        mock_link.assert_called_once()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_without_known_prefix_no_unknown_certs(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_without_known_prefix with no unknown certificates
        '''
        mock_list.side_effect = [["restapiserver.1", "gnmiserver.2"], []]
        cert_converter.convert_certs_without_known_prefix("/source/", ["restapiserver", "gnmiserver"], "/dest/")
        mock_convert.assert_not_called()
        mock_link.assert_not_called()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_without_known_prefix_with_unknown_certs(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_without_known_prefix with unknown certificates
        '''
        mock_list.side_effect = [["unknowncert.1", "restapiserver.2"], []]
        mock_convert.return_value = True
        mock_link.return_value = True
        cert_converter.convert_certs_without_known_prefix("/source/", ["restapiserver", "gnmiserver"], "/dest/")
        mock_convert.assert_called_once_with("unknowncert.1", "/source/", "/dest/")
        mock_link.assert_called_once()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_without_known_prefix_already_converted(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_without_known_prefix with already converted unknown certificates
        '''
        mock_list.side_effect = [["unknowncert.1"], ["unknowncert.1"]]
        cert_converter.convert_certs_without_known_prefix("/source/", ["restapiserver", "gnmiserver"], "/dest/")
        mock_convert.assert_not_called()
        mock_link.assert_not_called()

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    @mock.patch("cert_converter.link_to_latest_cert")
    def test_convert_certs_without_known_prefix_conversion_fails(self, mock_link, mock_convert, mock_list):
        '''
        Test convert_certs_without_known_prefix when conversion fails (should continue)
        '''
        mock_list.side_effect = [["unknowncert.1", "anothercert.2"], []]
        mock_convert.side_effect = [False, True]  # First fails, second succeeds
        mock_link.return_value = True
        cert_converter.convert_certs_without_known_prefix("/source/", ["restapiserver", "gnmiserver"], "/dest/")
        self.assertEqual(mock_convert.call_count, 2)
        mock_link.assert_called_once()

    @mock.patch("cert_converter.convert_certs_with_prefix")
    @mock.patch("cert_converter.convert_certs_without_known_prefix")
    def test_convert_all_certs_success(self, mock_without_prefix, mock_with_prefix):
        '''
        Test successful convert_all_certs
        '''
        mock_with_prefix.return_value = True
        cert_converter.convert_all_certs()
        # Should be called twice (once for restapiserver, once for gnmiserver)
        self.assertEqual(mock_with_prefix.call_count, 2)
        mock_without_prefix.assert_called_once()

    @mock.patch("cert_converter.convert_certs_with_prefix")
    @mock.patch("cert_converter.convert_certs_without_known_prefix")
    def test_convert_all_certs_with_failure(self, mock_without_prefix, mock_with_prefix):
        '''
        Test convert_all_certs with some failures (should continue)
        '''
        mock_with_prefix.side_effect = [False, True]  # First fails, second succeeds
        cert_converter.convert_all_certs()
        self.assertEqual(mock_with_prefix.call_count, 2)
        mock_without_prefix.assert_called_once()

    @mock.patch("subprocess.run")
    def test_execute_cmd_success(self, mock_subprocess):
        '''
        Test successful execute_cmd
        '''
        mock_process = mock.Mock()
        mock_process.returncode = 0
        mock_process.stdout = b"output"
        mock_process.stderr = b""
        mock_subprocess.return_value = mock_process
        
        success, output = cert_converter.execute_cmd(["test", "command"])
        self.assertTrue(success)
        self.assertEqual(output, "output")

    @mock.patch("subprocess.run")
    def test_execute_cmd_failure(self, mock_subprocess):
        '''
        Test failed execute_cmd
        '''
        mock_process = mock.Mock()
        mock_process.returncode = 1
        mock_process.stdout = b""
        mock_process.stderr = b"error message"
        mock_subprocess.return_value = mock_process
        
        success, output = cert_converter.execute_cmd(["test", "command"])
        self.assertFalse(success)
        self.assertEqual(output, "")

    @mock.patch("cert_converter.get_list_of_certs")
    def test_convert_certs_with_prefix_wrong_prefix(self, mock_list):
        '''
        Test convert_certs_with_prefix ignores certs with wrong prefix
        '''
        mock_list.side_effect = [[], ["gnmiserver.1", "othercert.2"]]
        with mock.patch("cert_converter.convert_single_cert") as mock_convert:
            result = cert_converter.convert_certs_with_prefix("/source/", "restapi", "/dest/", 10)
            self.assertTrue(result)
            mock_convert.assert_not_called()  # No certs should be converted

    def test_cert_list_with_metadata_files(self):
        '''
        Test get_list_of_certs ignores metadata and notify files
        '''
        with mock.patch("os.path.exists", return_value=True), \
             mock.patch("os.path.isfile", return_value=True), \
             mock.patch("os.path.islink", return_value=False), \
             mock.patch("os.listdir", return_value=[
                 "cert.metadata",
                 "cert.notify", 
                 "restapiserver.crt.1",
                 "restapiserver.key.1"
             ]):
            result = cert_converter.get_list_of_certs("/test/")
            self.assertEqual(result, ["restapiserver.1"])

    def test_cert_list_with_bootstrap_files(self):
        '''
        Test get_list_of_certs ignores bootstrap, temp, and test files
        '''
        with mock.patch("os.path.exists", return_value=True), \
             mock.patch("os.path.isfile", return_value=True), \
             mock.patch("os.path.islink", return_value=False), \
             mock.patch("os.listdir", return_value=[
                 "sonic_acms_bootstrap.crt.1",
                 "temp.crt.2",
                 "test.key.3",
                 "restapiserver.crt.1"
             ]):
            result = cert_converter.get_list_of_certs("/test/")
            self.assertEqual(result, ["restapiserver.1"])

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("cert_converter.convert_all_certs")
    @mock.patch("cert_converter.clean_current_certs")
    @mock.patch("os.path.isfile")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_isfile, mock_clean, mock_convert, mock_table, mock_db):
        '''
        Test main function with uber notify file present
        '''
        mock_isfile.return_value = True
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        
        # Mock convert_all_certs to raise exception after first call to break polling loop
        mock_convert.side_effect = [None, Exception("Break polling loop")]
        
        try:
            cert_converter.main()
        except Exception as e:
            self.assertEqual(str(e), "Break polling loop")
        
        # Should clean all cert paths
        self.assertEqual(mock_clean.call_count, 3)  # 2 from certs_path_map + 1 default
        # Should call convert_all_certs twice (once after notify, once in polling)
        self.assertEqual(mock_convert.call_count, 2)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("cert_converter.convert_all_certs")
    @mock.patch("cert_converter.clean_current_certs")
    @mock.patch("os.path.isfile")
    @mock.patch("time.sleep")
    def test_main_03_immediate_notify(self, mock_sleep, mock_isfile, mock_clean, mock_convert, mock_table, mock_db):
        '''
        Test main function when notify file is found immediately
        '''
        mock_isfile.return_value = True  # Notify file found immediately
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        
        # Convert calls: first succeeds, second in polling raises exception
        mock_convert.side_effect = [None, Exception("Break polling loop")]
        
        try:
            cert_converter.main()
        except Exception as e:
            self.assertEqual(str(e), "Break polling loop")
        
        # Should clean all cert paths (2 from certs_path_map + 1 default)
        self.assertEqual(mock_clean.call_count, 3)
        # Should call convert_all_certs twice (once after notify, once in polling)
        self.assertEqual(mock_convert.call_count, 2)
        # No sleep calls since notify file found immediately and exception occurs before polling sleep
        self.assertEqual(mock_sleep.call_count, 0)

    @mock.patch("os.path.exists")
    def test_clean_current_certs_nonexistent_path(self, mock_exists):
        '''
        Test clean_current_certs with nonexistent path
        '''
        mock_exists.return_value = False
        # Should not raise exception
        cert_converter.clean_current_certs("/nonexistent/")
        mock_exists.assert_called_once_with("/nonexistent/")

    @mock.patch("os.path.exists")
    @mock.patch("os.listdir")
    def test_clean_current_certs_empty_directory(self, mock_listdir, mock_exists):
        '''
        Test clean_current_certs with empty directory
        '''
        mock_exists.return_value = True
        mock_listdir.return_value = []
        # Should not raise exception
        cert_converter.clean_current_certs("/empty/")

    @mock.patch("os.path.exists")
    @mock.patch("os.path.isfile") 
    @mock.patch("os.path.islink")
    @mock.patch("os.listdir")
    @mock.patch("os.remove")
    def test_clean_current_certs_malformed_filenames(self, mock_remove, mock_listdir, mock_islink, mock_isfile, mock_exists):
        '''
        Test clean_current_certs with malformed filenames (should skip them)
        '''
        mock_exists.return_value = True
        mock_isfile.return_value = True
        mock_islink.return_value = False
        mock_listdir.return_value = [
            "malformed",  # No extension
            "normal.crt.1"  # Should be removed
        ]
        cert_converter.clean_current_certs("/test/")
        mock_remove.assert_called_once_with("/test/normal.crt.1")

    def test_cert_list_malformed_filenames(self):
        '''
        Test get_list_of_certs with malformed filenames
        '''
        with mock.patch("os.path.exists", return_value=True), \
             mock.patch("os.path.isfile", return_value=True), \
             mock.patch("os.path.islink", return_value=False), \
             mock.patch("os.listdir", return_value=[
                 "malformed",       # No dots at all
                 "file.crt",        # Only 2 parts when split by '.' (should be ignored)
                 "good.crt.1"       # 3 parts (should be included)
             ]):
            result = cert_converter.get_list_of_certs("/test/")
            self.assertEqual(result, ["good.1"])

    @mock.patch("cert_converter.get_list_of_certs")
    @mock.patch("cert_converter.convert_single_cert")
    def test_convert_certs_without_known_prefix_mixed_results(self, mock_convert, mock_list):
        '''
        Test convert_certs_without_known_prefix with mixed conversion results
        '''
        mock_list.side_effect = [["unknown1.1", "unknown2.2", "restapiserver.1"], []]
        # First cert fails, second succeeds
        mock_convert.side_effect = [False, True]
        
        with mock.patch("cert_converter.link_to_latest_cert") as mock_link:
            mock_link.return_value = True
            cert_converter.convert_certs_without_known_prefix("/source/", ["restapiserver"], "/dest/")
            
            # Should try to convert both unknown certs
            self.assertEqual(mock_convert.call_count, 2)
            # Should still link because at least one succeeded
            mock_link.assert_called_once()

    def test_password_length_usage(self):
        '''
        Test that password_length global variable is accessible
        '''
        self.assertEqual(cert_converter.password_length, 64)
        
    @mock.patch("cert_converter.random.choice")
    @mock.patch("cert_converter.execute_cmd")
    def test_convert_single_cert_password_generation(self, mock_cmd, mock_choice):
        '''
        Test that convert_single_cert generates password of correct length
        '''
        mock_cmd.side_effect = [(True, ""), (True, "private_key"), (True, "")]
        mock_choice.side_effect = ['A'] * cert_converter.password_length  # Simulate consistent random choice
        
        result = cert_converter.convert_single_cert("testcert.1", "/source/", "/dest/")
        self.assertTrue(result)
        
        # Verify password generation was called correct number of times
        self.assertEqual(mock_choice.call_count, cert_converter.password_length)

    @mock.patch("swsscommon.swsscommon.DBConnector")
    @mock.patch("swsscommon.swsscommon.Table")
    @mock.patch("cert_converter.convert_all_certs")
    @mock.patch("cert_converter.clean_current_certs")
    @mock.patch("os.path.isfile")
    @mock.patch("time.sleep")
    def test_main_04_with_polling_sleep(self, mock_sleep, mock_isfile, mock_clean, mock_convert, mock_table, mock_db):
        '''
        Test main function with successful polling sleep
        '''
        mock_isfile.return_value = True  # Notify file found immediately
        mock_table.return_value = {"localhost": (True, (('', ''),))}
        mock_db.return_value = None
        
        # First convert succeeds, second convert succeeds, sleep raises exception to break
        mock_convert.return_value = None
        mock_sleep.side_effect = [Exception("Break after sleep")]
        
        try:
            cert_converter.main()
        except Exception as e:
            self.assertEqual(str(e), "Break after sleep")
        
        # Should clean all cert paths (2 from certs_path_map + 1 default)  
        self.assertEqual(mock_clean.call_count, 3)
        # Should call convert_all_certs twice (once after notify, once in polling)
        self.assertEqual(mock_convert.call_count, 2)
        # Should have one sleep call that raises exception
        self.assertEqual(mock_sleep.call_count, 1)
        mock_sleep.assert_called_with(cert_converter.polling_frequency)
