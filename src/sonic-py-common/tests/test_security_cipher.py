import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

import pytest
from sonic_py_common.security_cipher import security_cipher
from .mock_swsscommon import ConfigDBConnector

EXPECTED_PASSWD = "TEST2" 

class TestSecurityCipher(object):
    def test_passkey_encryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("builtins.open", mock_open()) as mock_file:
            temp = security_cipher()

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("builtins.open", mock_open()) as mock_file:
                encrypt, err = temp.encrypt_passkey("TACPLUS", "passkey1", "TEST1")
                assert encrypt !=  "passkey1"
                assert err == None 

    def test_passkey_decryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("builtins.open", mock_open()) as mock_file:
            temp = security_cipher()

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("builtins.open", mock_open()) as mock_file:
                encrypt, err = temp.encrypt_passkey("RADIUS", "passkey2", EXPECTED_PASSWD)
                assert err == None

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("builtins.open", mock_open(read_data=EXPECTED_PASSWD)) as mock_file: 
                decrypt, err = temp.decrypt_passkey("RADIUS", encrypt)
                assert err == None 
                assert decrypt == "passkey2"


