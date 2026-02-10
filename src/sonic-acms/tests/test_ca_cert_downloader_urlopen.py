#!/usr/bin/env python3

"""
Unit tests for CA_cert_downloader.main() with mocked urlopen using test data
"""

import os
import json
import tempfile
import shutil
from unittest import TestCase, mock
from unittest.mock import MagicMock, mock_open, patch
import CA_cert_downloader


class MockHTTPResponse:
    """Mock HTTP response object"""
    def __init__(self, data, status_code=200):
        self.data = data
        self.status_code = status_code
    
    def read(self):
        return self.data.encode('utf-8')
    
    def getcode(self):
        return self.status_code


class TestCACertDownloaderWithMockedUrlopen(TestCase):
    """Test CA_cert_downloader.main() with mocked urlopen"""
    
    @classmethod
    def setUpClass(cls):
        """Generate fake certificate data for testing"""
        from cryptography import x509
        from cryptography.x509.oid import NameOID
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import rsa
        from cryptography.hazmat.backends import default_backend
        from datetime import datetime, timedelta
        
        # Generate Fake Root CA 1
        root1_key = rsa.generate_private_key(public_exponent=65537, key_size=2048, backend=default_backend())
        root1_name = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Corp"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Fake Root CA 1"),
        ])
        root1_cert = x509.CertificateBuilder().subject_name(
            root1_name
        ).issuer_name(
            root1_name  # Self-signed
        ).public_key(
            root1_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=3650)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True,
        ).sign(root1_key, hashes.SHA256(), backend=default_backend())
        
        # Generate Fake Root CA 2
        root2_key = rsa.generate_private_key(public_exponent=65537, key_size=2048, backend=default_backend())
        root2_name = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Corp"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Fake Root CA 2"),
        ])
        root2_cert = x509.CertificateBuilder().subject_name(
            root2_name
        ).issuer_name(
            root2_name  # Self-signed
        ).public_key(
            root2_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=3650)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True,
        ).sign(root2_key, hashes.SHA256(), backend=default_backend())
        
        # Generate Intermediate CA signed by Root CA 1
        intermediate1_key = rsa.generate_private_key(public_exponent=65537, key_size=2048, backend=default_backend())
        intermediate1_name = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Corp"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Fake Intermediate CA 1"),
        ])
        intermediate1_cert = x509.CertificateBuilder().subject_name(
            intermediate1_name
        ).issuer_name(
            root1_name  # Signed by Root CA 1
        ).public_key(
            intermediate1_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=1825)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=0),
            critical=True,
        ).sign(root1_key, hashes.SHA256(), backend=default_backend())
        
        # Generate a second Intermediate CA also signed by Root CA 1
        intermediate1b_key = rsa.generate_private_key(public_exponent=65537, key_size=2048, backend=default_backend())
        intermediate1b_name = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Corp"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Fake Intermediate CA 1B"),
        ])
        intermediate1b_cert = x509.CertificateBuilder().subject_name(
            intermediate1b_name
        ).issuer_name(
            root1_name  # Also signed by Root CA 1
        ).public_key(
            intermediate1b_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=1825)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=0),
            critical=True,
        ).sign(root1_key, hashes.SHA256(), backend=default_backend())
        
        # Generate Intermediate CA signed by Root CA 2
        intermediate2_key = rsa.generate_private_key(public_exponent=65537, key_size=2048, backend=default_backend())
        intermediate2_name = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Corp"),
            x509.NameAttribute(NameOID.COMMON_NAME, "Fake Intermediate CA 2"),
        ])
        intermediate2_cert = x509.CertificateBuilder().subject_name(
            intermediate2_name
        ).issuer_name(
            root2_name  # Signed by Root CA 2
        ).public_key(
            intermediate2_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.utcnow()
        ).not_valid_after(
            datetime.utcnow() + timedelta(days=1825)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=0),
            critical=True,
        ).sign(root2_key, hashes.SHA256(), backend=default_backend())
        
        # Convert certificates to PEM format
        root1_pem = root1_cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')
        root2_pem = root2_cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')
        intermediate1_pem = intermediate1_cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')
        intermediate1b_pem = intermediate1b_cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')
        intermediate2_pem = intermediate2_cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')
        
        # Create fake JSON response matching dSMS API format
        cls.test_json_obj = {
            "RootsInfos": [
                {
                    "rootName": "/certificates/test/fake_root_ca_1",
                    "CaName": "FakeCA1",
                    "PEM": root1_pem,
                    "Intermediates": [
                        {
                            "IntermediateName": "/certificates/test/fake_intermediate_1",
                            "PEM": intermediate1_pem
                        },
                        {
                            "IntermediateName": "/certificates/test/fake_intermediate_1b",
                            "PEM": intermediate1b_pem
                        }
                    ]
                },
                {
                    "rootName": "/certificates/test/fake_root_ca_2",
                    "CaName": "FakeCA2",
                    "PEM": root2_pem,
                    "Intermediates": [
                        {
                            "IntermediateName": "/certificates/test/fake_intermediate_2",
                            "PEM": intermediate2_pem
                        }
                    ]
                }
            ],
            "RootCertificates": [
                {
                    "rootName": "/certificates/test/fake_root_ca_1",
                    "PEM": root1_pem
                },
                {
                    "rootName": "/certificates/test/fake_root_ca_2",
                    "PEM": root2_pem
                }
            ]
        }
        cls.test_json_data = json.dumps(cls.test_json_obj)
    
    def setUp(self):
        """Set up test fixtures"""
        # Create temporary directories for testing
        self.temp_dir = tempfile.mkdtemp()
        self.acms_dir = os.path.join(self.temp_dir, "acms")
        self.creds_dir = os.path.join(self.temp_dir, "credentials")
        os.makedirs(self.acms_dir)
        os.makedirs(self.creds_dir)
        
        # Patch the paths used in CA_cert_downloader
        self.root_cert_path = os.path.join(self.acms_dir, "AME_ROOT_CERTIFICATE.pem")
        self.certs_path = self.creds_dir + "/"
    
    def tearDown(self):
        """Clean up test fixtures"""
        # if os.path.exists(self.temp_dir):
        #     shutil.rmtree(self.temp_dir)
    
    @patch("CA_cert_downloader.sonic_logger")
    @patch("CA_cert_downloader.acms_conf")
    @patch("CA_cert_downloader.ROOT_CERT")
    @patch("CA_cert_downloader.CERTS_PATH")
    @patch("urllib.request.urlopen")
    @patch("dSMS_config_modifier.get_device_cloudtype")
    @patch("time.sleep")
    def test_main_successful_download(self, mock_sleep, mock_cloudtype, mock_urlopen, 
                                      mock_certs_path, mock_root_cert, mock_acms_conf,
                                      mock_logger):
        """
        Test main() successfully downloads and extracts certificates
        """
        # Set up mocks
        mock_acms_conf.return_value = os.path.join(self.temp_dir, "acms_secrets.ini")
        mock_root_cert.return_value = self.root_cert_path
        mock_certs_path.return_value = self.certs_path
        
        # Create mock config file
        config_content = "FullHttpsDsmsUrl=https://test-dsms.microsoft.com\n"
        with open(mock_acms_conf.return_value, 'w') as f:
            f.write(config_content)
        
        # Mock cloudtype
        mock_cloudtype.return_value = "public"
        
        # Mock urlopen to return test data
        mock_response = MockHTTPResponse(self.test_json_data, 200)
        mock_urlopen.return_value = mock_response
        
        # Mock sleep to raise exception after first iteration (to break the infinite loop)
        mock_sleep.side_effect = [Exception("Test complete")]
        
        # Patch the module-level variables
        with patch.object(CA_cert_downloader, 'ROOT_CERT', self.root_cert_path), \
             patch.object(CA_cert_downloader, 'CERTS_PATH', self.certs_path), \
             patch.object(CA_cert_downloader, 'acms_conf', mock_acms_conf.return_value):
            
            try:
                CA_cert_downloader.main()
            except Exception as e:
                # Expected exception to break the loop
                self.assertEqual(str(e), "Test complete")
        
        # Verify urlopen was called with correct URL
        self.assertTrue(mock_urlopen.called)
        # Get the Request object that was passed to urlopen
        request_obj = mock_urlopen.call_args[0][0]
        request_url = request_obj.full_url
        self.assertIn("https://test-dsms.microsoft.com", request_url)
        self.assertIn("getissuersv3", request_url)
        
        # Verify certificate file was created in /acms directory
        self.assertTrue(os.path.exists(self.root_cert_path))
        
        # Verify certificate content
        with open(self.root_cert_path, 'r') as f:
            cert_content = f.read()
            # Check that intermediates and root certs are present
            self.assertIn("BEGIN CERTIFICATE", cert_content)
            self.assertIn("END CERTIFICATE", cert_content)
        
        # Verify certificate was copied to credentials directory
        root_cert_filename = os.path.basename(self.root_cert_path)
        dest_cert = os.path.join(self.creds_dir, root_cert_filename)
        self.assertTrue(os.path.exists(dest_cert))
        
        # Verify multiple distinct issuer CNs using openssl
        import subprocess
        import re
        cmd = f'openssl crl2pkcs7 -nocrl -certfile {dest_cert} | openssl pkcs7 -print_certs -text -noout'
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        # Extract only the CN values from Issuer lines using regex
        issuer_pattern = re.compile(r'Issuer:.*CN=([^,\n]+)', re.IGNORECASE)
        issuer_lines = issuer_pattern.findall(result.stdout)
        # Should have at least 2 distinct Issuer CNs (intermediates + root)
        unique_issuers = set(issuer_lines)
        self.assertGreaterEqual(len(unique_issuers), 2, 
                               f"Expected at least 2 distinct Issuer CNs, found {len(unique_issuers)}: {unique_issuers}")
        print(f"Found {len(issuer_lines)} Issuer CNs ({len(unique_issuers)} unique):")
        for issuer in unique_issuers:
            print(f"  {issuer}")
        
        # Verify logging - check that log_info was called with expected messages
        print("\n=== Log Info Calls ===")
        log_messages = [call[0][0] for call in mock_logger.log_info.call_args_list]
        for msg in log_messages:
            print(f"  {msg}")
        
        # Verify specific log messages exist
        assert any("Processing root #0" in msg for msg in log_messages), "Missing log for processing root #1"
        assert any("Processing root #1" in msg for msg in log_messages), "Missing log for processing root #2"
        assert any("RootName:" in msg for msg in log_messages), "Missing RootName in logs"
        assert any("CaName:" in msg for msg in log_messages), "Missing CaName in logs"
        assert any("Wrote" in msg and "intermediate certificates" in msg for msg in log_messages), "Missing intermediate cert count log"
        assert any("Successfully wrote" in msg and "root certificates" in msg for msg in log_messages), "Missing final success log"
        assert any("Cert extraction completed" in msg for msg in log_messages), "Missing cert extraction completed log"
    
    @patch("CA_cert_downloader.acms_conf")
    @patch("CA_cert_downloader.ROOT_CERT")
    @patch("CA_cert_downloader.CERTS_PATH")
    @patch("urllib.request.urlopen")
    @patch("dSMS_config_modifier.get_device_cloudtype")
    @patch("time.sleep")
    def test_main_with_retry_on_network_error(self, mock_sleep, mock_cloudtype, mock_urlopen,
                                               mock_certs_path, mock_root_cert, mock_acms_conf):
        """
        Test main() retries on network errors (URLError)
        """
        # Set up mocks
        mock_acms_conf.return_value = os.path.join(self.temp_dir, "acms_secrets.ini")
        mock_root_cert.return_value = self.root_cert_path
        mock_certs_path.return_value = self.certs_path
        
        # Create mock config file
        config_content = "FullHttpsDsmsUrl=https://test-dsms.microsoft.com\n"
        with open(mock_acms_conf.return_value, 'w') as f:
            f.write(config_content)
        
        # Mock cloudtype
        mock_cloudtype.return_value = "public"
        
        # First 2 calls raise URLError, 3rd succeeds, 4th raises exception to break loop
        from urllib.error import URLError
        mock_urlopen.side_effect = [
            URLError("Network unreachable"),
            URLError("Connection timeout"),
            MockHTTPResponse(self.test_json_data, 200),
            Exception("Test complete")
        ]
        
        # Track sleep calls for retry delays
        sleep_calls = []
        def track_sleep(seconds):
            sleep_calls.append(seconds)
            if len(sleep_calls) >= 3:  # After 2 retries + success + wait for next poll
                raise Exception("Test complete")
        
        mock_sleep.side_effect = track_sleep
        
        # Patch the module-level variables
        with patch.object(CA_cert_downloader, 'ROOT_CERT', self.root_cert_path), \
             patch.object(CA_cert_downloader, 'CERTS_PATH', self.certs_path), \
             patch.object(CA_cert_downloader, 'acms_conf', mock_acms_conf.return_value):
            
            try:
                CA_cert_downloader.main()
            except Exception as e:
                self.assertEqual(str(e), "Test complete")
        
        # Verify urlopen was called 3 times (2 failures + 1 success)
        self.assertEqual(mock_urlopen.call_count, 3)
        
        # Verify retry delays with exponential backoff
        self.assertGreater(len(sleep_calls), 0)
        print(f"Sleep times between attempts: {[int(s) for s in sleep_calls]}")
        
        # First two should be retry delays (with jitter around base values)
        # Attempt 1: ~300s (5 min), Attempt 2: ~600s (10 min)
        self.assertGreater(sleep_calls[0], 270)  # ~300s with jitter
        self.assertLess(sleep_calls[0], 360)
        self.assertGreater(sleep_calls[1], 540)  # ~600s with jitter
        self.assertLess(sleep_calls[1], 720)
        
        # Verify certificate was eventually created
        self.assertTrue(os.path.exists(self.root_cert_path))
    
    @patch("CA_cert_downloader.acms_conf")
    @patch("CA_cert_downloader.ROOT_CERT")
    @patch("CA_cert_downloader.CERTS_PATH")
    @patch("urllib.request.urlopen")
    @patch("dSMS_config_modifier.get_device_cloudtype")
    @patch("time.sleep")
    def test_main_with_many_retries_then_success(self, mock_sleep, mock_cloudtype, mock_urlopen,
                                                  mock_certs_path, mock_root_cert, mock_acms_conf):
        """
        Test main() with many retries (exceeding max_attempts) before success
        """
        # Set up mocks
        mock_acms_conf.return_value = os.path.join(self.temp_dir, "acms_secrets.ini")
        mock_root_cert.return_value = self.root_cert_path
        mock_certs_path.return_value = self.certs_path
        
        # Create mock config file
        config_content = "FullHttpsDsmsUrl=https://test-dsms.microsoft.com\n"
        with open(mock_acms_conf.return_value, 'w') as f:
            f.write(config_content)
        
        # Mock cloudtype
        mock_cloudtype.return_value = "public"
        
        # First 8 calls raise URLError, 9th succeeds
        from urllib.error import URLError
        mock_urlopen.side_effect = [
            URLError("Network unreachable"),
            URLError("Connection timeout"),
            URLError("DNS resolution failed"),
            URLError("Connection refused"),
            URLError("Timeout"),
            URLError("SSL error"),
            URLError("Connection reset"),
            URLError("Service unavailable"),
            MockHTTPResponse(self.test_json_data, 200),
            Exception("Test complete")
        ]
        
        # Track sleep calls for retry delays
        sleep_calls = []
        def track_sleep(seconds):
            sleep_calls.append(seconds)
            if len(sleep_calls) >= 9:  # After 8 retries + success + wait for next poll
                raise Exception("Test complete")
        
        mock_sleep.side_effect = track_sleep
        
        # Patch the module-level variables
        with patch.object(CA_cert_downloader, 'ROOT_CERT', self.root_cert_path), \
             patch.object(CA_cert_downloader, 'CERTS_PATH', self.certs_path), \
             patch.object(CA_cert_downloader, 'acms_conf', mock_acms_conf.return_value):
            
            try:
                CA_cert_downloader.main()
            except Exception as e:
                self.assertEqual(str(e), "Test complete")
        
        # Verify urlopen was called 9 times (8 failures + 1 success)
        self.assertEqual(mock_urlopen.call_count, 9)
        
        # Verify exponential backoff with cap at max_delay
        print(f"Sleep times for many retries: {[int(s) for s in sleep_calls]}")
        
        # First 5 attempts should show exponential backoff
        # Attempt 1: ~300s, Attempt 2: ~600s, Attempt 3: ~1200s, Attempt 4: ~2400s, Attempt 5: ~4800s
        # Attempts 6-8: should be capped at ~7200s (max_delay)
        
        # Verify first few follow exponential pattern
        self.assertGreater(sleep_calls[0], 270)  # ~300s with jitter
        self.assertLess(sleep_calls[0], 360)
        self.assertGreater(sleep_calls[1], 540)  # ~600s with jitter
        self.assertLess(sleep_calls[1], 720)
        
        # Verify later attempts are capped at max_delay
        for i in range(5, 8):  # Attempts 6-8 should be at max
            self.assertGreater(sleep_calls[i], 6480)  # ~7200s with jitter
            self.assertLess(sleep_calls[i], 7920)
        
        # Last sleep should be 12 hours after success
        self.assertEqual(sleep_calls[8], 60 * 60 * 12)
        
        # Verify certificate was eventually created
        self.assertTrue(os.path.exists(self.root_cert_path))
    
    @patch("CA_cert_downloader.acms_conf")
    @patch("CA_cert_downloader.ROOT_CERT")
    @patch("CA_cert_downloader.CERTS_PATH")
    @patch("urllib.request.urlopen")
    @patch("dSMS_config_modifier.get_device_cloudtype")
    @patch("time.sleep")
    def test_main_fails_on_non_200_status(self, mock_sleep, mock_cloudtype, mock_urlopen,
                                          mock_certs_path, mock_root_cert, mock_acms_conf):
        """
        Test main() handles non-200 HTTP status codes
        """
        # Set up mocks
        mock_acms_conf.return_value = os.path.join(self.temp_dir, "acms_secrets.ini")
        mock_root_cert.return_value = self.root_cert_path
        mock_certs_path.return_value = self.certs_path
        
        # Create mock config file
        config_content = "FullHttpsDsmsUrl=https://test-dsms.microsoft.com\n"
        with open(mock_acms_conf.return_value, 'w') as f:
            f.write(config_content)
        
        # Mock cloudtype
        mock_cloudtype.return_value = "public"
        
        # Return 404 status code
        mock_response = MockHTTPResponse("Not Found", 404)
        mock_urlopen.return_value = mock_response
        
        # Mock sleep to raise exception after retries
        call_count = [0]
        def track_calls(seconds):
            call_count[0] += 1
            if call_count[0] >= 5:  # After retries, should poll again in 12 hours
                raise Exception("Test complete")
        
        mock_sleep.side_effect = track_calls
        
        # Patch the module-level variables
        with patch.object(CA_cert_downloader, 'ROOT_CERT', self.root_cert_path), \
             patch.object(CA_cert_downloader, 'CERTS_PATH', self.certs_path), \
             patch.object(CA_cert_downloader, 'acms_conf', mock_acms_conf.return_value):
            
            try:
                CA_cert_downloader.main()
            except Exception as e:
                self.assertEqual(str(e), "Test complete")
        
        # Verify certificate was NOT created (because of 404)
        self.assertFalse(os.path.exists(self.root_cert_path))
    
    @patch("CA_cert_downloader.acms_conf")
    @patch("CA_cert_downloader.ROOT_CERT")
    @patch("CA_cert_downloader.CERTS_PATH")
    @patch("urllib.request.urlopen")
    @patch("dSMS_config_modifier.get_device_cloudtype")
    @patch("time.sleep")
    def test_main_different_cloud_types(self, mock_sleep, mock_cloudtype, mock_urlopen,
                                        mock_certs_path, mock_root_cert, mock_acms_conf):
        """
        Test main() uses correct URL path for different cloud types
        """
        for cloud_type in ["public", "fairfax", "mooncake", "usnat", "ussec"]:
            with self.subTest(cloud_type=cloud_type):
                # Set up mocks
                mock_acms_conf.return_value = os.path.join(self.temp_dir, "acms_secrets.ini")
                mock_root_cert.return_value = self.root_cert_path
                mock_certs_path.return_value = self.certs_path
                
                # Create mock config file
                config_content = "FullHttpsDsmsUrl=https://test-dsms.microsoft.com\n"
                with open(mock_acms_conf.return_value, 'w') as f:
                    f.write(config_content)
                
                # Mock cloudtype
                mock_cloudtype.return_value = cloud_type
                
                # Mock urlopen
                mock_response = MockHTTPResponse(self.test_json_data, 200)
                mock_urlopen.return_value = mock_response
                mock_urlopen.reset_mock()
                
                # Mock sleep to break after first iteration
                mock_sleep.side_effect = [Exception("Test complete")]
                
                # Patch the module-level variables
                with patch.object(CA_cert_downloader, 'ROOT_CERT', self.root_cert_path), \
                     patch.object(CA_cert_downloader, 'CERTS_PATH', self.certs_path), \
                     patch.object(CA_cert_downloader, 'acms_conf', mock_acms_conf.return_value):
                    
                    try:
                        CA_cert_downloader.main()
                    except Exception:
                        pass
                
                # Verify correct URL path was used
                self.assertTrue(mock_urlopen.called)
                # Get the Request object that was passed to urlopen
                request_obj = mock_urlopen.call_args[0][0]
                request_url = request_obj.full_url
                self.assertIn("getissuersv3", request_url)
                self.assertIn("clientauth", request_url)
                
                # Clean up for next iteration
                if os.path.exists(self.root_cert_path):
                    os.remove(self.root_cert_path)


if __name__ == '__main__':
    import unittest
    unittest.main()
