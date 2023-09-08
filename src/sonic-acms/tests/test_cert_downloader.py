from unittest import TestCase, mock
import CA_cert_downloader


class TestCertDownloader(TestCase):

    @mock.patch("CA_cert_downloader.get_url")
    @mock.patch("CA_cert_downloader.url_validator")
    @mock.patch("CA_cert_downloader.get_cert")
    @mock.patch("CA_cert_downloader.copy_cert")
    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    @mock.patch("time.sleep")
    def test_main_01(self, mock_sleep, mock_cloud, mock_copy, mock_get, mock_validator, mock_url):
        '''
        Run cert downloader with single loop
        '''
        expect_error = Exception("Break the loop")
        mock_url.return_value = "https://dummy-dsms.net"
        mock_validator.return_value = True
        mock_get.return_value = True
        mock_copy.side_effect = [expect_error]
        try:
            CA_cert_downloader.main()
        except Exception as e:
            self.assertEqual(e, expect_error)
        self.assertFalse(mock_sleep.called)

    @mock.patch("CA_cert_downloader.get_url")
    @mock.patch("CA_cert_downloader.url_validator")
    @mock.patch("CA_cert_downloader.get_cert")
    @mock.patch("CA_cert_downloader.copy_cert")
    @mock.patch("dSMS_config_modifier.get_device_cloudtype")
    @mock.patch("time.sleep")
    def test_main_02(self, mock_sleep, mock_cloud, mock_copy, mock_get, mock_validator, mock_url):
        '''
        Run cert downloader with multiple loop
        Check sleep delay
        '''
        count = 100
        expect_error = Exception("Break the loop")
        mock_url.return_value = "https://dummy-dsms.net"
        result_list = []
        for i in range(count):
            result_list.append(False)
        result_list.append(True)
        mock_validator.side_effect = result_list
        mock_get.return_value = True
        mock_copy.side_effect = [expect_error]
        try:
            CA_cert_downloader.main()
        except Exception as e:
            self.assertEqual(e, expect_error)
        expect_delay = []
        curr = 60
        for i in range(count):
            expect_delay.append(mock.call(curr))
            curr = curr * 2
            if curr > 3600:
                curr = 3600
        self.assertEqual(mock_sleep.call_args_list, expect_delay)

    @mock.patch('CA_cert_downloader.extract_cert')
    def test_get_cert_01(self, mock_extract):
        '''
        Download certificate
        Check extract_cert
        '''
        with mock.patch('urllib.request.urlopen', mock.mock_open(read_data='{}')) as mock_url:
            mock_url.return_value.getcode = mock.MagicMock()
            mock_url.return_value.getcode.return_value = 200
            CA_cert_downloader.get_cert("https://dummy")
            self.assertEqual(mock_extract.call_args_list, [mock.call({})])

    @mock.patch('time.sleep')
    def test_get_cert_02(self, mock_sleep):
        '''
        Download certificate and simulate exception
        Check exception
        '''
        with mock.patch('urllib.request.urlopen') as mock_url:
            download_err = Exception("Download failed")
            mock_url.side_effect = [download_err]
            expect_err = Exception("Exit function")
            mock_sleep.side_effect = [expect_err]
            try:
                CA_cert_downloader.get_cert("https://dummy")
            except Exception as e:
                self.assertEqual(e, expect_err)
