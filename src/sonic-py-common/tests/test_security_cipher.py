import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

import pytest
from sonic_py_common.security_cipher import master_key_mgr 
from .mock_swsscommon import ConfigDBConnector

# TODO: Remove this if/else block once we no longer support Python 2
if sys.version_info.major == 3:
    BUILTINS = "builtins"
else:
    BUILTINS = "__builtin__"

DEFAULT_FILE = [
        "#Auto generated file for storing the encryption passwords",
        "TACPLUS : ",
        "RADIUS : ", 
        "LDAP :"
        ]

UPDATED_FILE = [
        "#Auto generated file for storing the encryption passwords",
        "TACPLUS : ",
        "RADIUS : TEST2",
        "LDAP :"
        ]


class TestSecurityCipher(object):
    def test_passkey_encryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
                mock.patch("os.chmod") as mock_chmod, \
                mock.patch("{}.open".format(BUILTINS),mock.mock_open()) as mock_file:
            temp = master_key_mgr()

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("{}.open".format(BUILTINS), mock.mock_open()) as mock_file, \
                    mock.patch("os.chmod") as mock_chmod:
                mock_fd = mock.MagicMock()
                mock_fd.readlines = mock.MagicMock(return_value=DEFAULT_FILE)
                mock_file.return_value.__enter__.return_value = mock_fd 
                encrypt = temp.encrypt_passkey("TACPLUS", "passkey1", "TEST1")
                assert encrypt !=  "passkey1"

    def test_passkey_decryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
                mock.patch("os.chmod") as mock_chmod, \
                mock.patch("{}.open".format(BUILTINS), mock.mock_open()) as mock_file:
            temp = master_key_mgr()

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("{}.open".format(BUILTINS), mock.mock_open()) as mock_file, \
                    mock.patch("os.chmod") as mock_chmod:
                mock_fd = mock.MagicMock()
                mock_fd.readlines = mock.MagicMock(return_value=DEFAULT_FILE)
                mock_file.return_value.__enter__.return_value = mock_fd
                encrypt = temp.encrypt_passkey("RADIUS", "passkey2", "TEST2")

            # Use patch to replace the built-in 'open' function with a mock
            with mock.patch("{}.open".format(BUILTINS), mock.mock_open()) as mock_file, \
                    mock.patch("os.chmod") as mock_chmod:
                mock_fd = mock.MagicMock()
                mock_fd.readlines = mock.MagicMock(return_value=UPDATED_FILE)
                mock_file.return_value.__enter__.return_value = mock_fd 
                decrypt = temp.decrypt_passkey("RADIUS", encrypt)
                assert decrypt == "passkey2"


