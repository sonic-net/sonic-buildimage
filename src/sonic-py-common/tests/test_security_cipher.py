import sys
import json
import base64

import pytest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from sonic_py_common.security_cipher import master_key_mgr

# TODO: Remove this if/else block once we no longer support Python 2
if sys.version_info.major == 3:
    BUILTINS = "builtins"
else:
    BUILTINS = "__builtin__"

DEFAULT_JSON = {
    "TACPLUS": {"callbacks": [], "password": None},
    "RADIUS": {"callbacks": [], "password": None},
    "LDAP": {"callbacks": [], "password": None}
}

UPDATED_JSON = {
    "TACPLUS": {"callbacks": [], "password": None},
    "RADIUS": {"callbacks": [], "password": "TEST2"},
    "LDAP": {"callbacks": [], "password": None}
}

def dummy_cb(table_info): pass
def cb1(table_info): pass
def cb2(table_info): pass

class TestSecurityCipher(object):
    def setup_method(self):
        # Reset singleton for isolation
        master_key_mgr._instance = None

    def test_set_feature_password_sets_password(self):
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(DEFAULT_JSON))) as mock_file, \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr()
            # Patch _save_registry to check written value
            with mock.patch.object(temp, "_save_registry") as mock_save:
                temp.set_feature_password("RADIUS", "testpw")
                args = mock_save.call_args[0][0]
                assert args["RADIUS"]["password"] == "testpw"

    def test_set_feature_password_does_not_overwrite_existing(self):
        json_data = UPDATED_JSON.copy()
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(json_data))), \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr()
            with mock.patch.object(temp, "_save_registry") as mock_save:
                temp.set_feature_password("RADIUS", "should_not_overwrite")
                mock_save.assert_not_called()

    def test_rotate_feature_passwd(self):
        cb_mock = mock.Mock()
        lookup = {"cb1": cb_mock}
        json_data = {
            "RADIUS": {"callbacks": ["cb1"], "password": "radius_secret"}
        }
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(json_data))), \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr(callback_lookup=lookup)
            with mock.patch.object(temp, "_save_registry") as mock_save:
                table_info = {"foo": "bar"}
                temp.rotate_feature_passwd("RADIUS", table_info)
                cb_mock.assert_called_once_with(table_info)

    def test_encrypt_and_decrypt_passkey(self):
        # Use a known password and mock openssl subprocess
        json_data = {
            "RADIUS": {"callbacks": [], "password": "secretpw"}
        }
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(json_data))), \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr()
            # Mock subprocess for encryption
            fake_cipher = b"\x01\x02\x03"
            with mock.patch("subprocess.run") as mock_subproc:
                mock_subproc.return_value = mock.Mock(stdout=fake_cipher)
                encrypted = temp.encrypt_passkey("RADIUS", "plaintext")
                assert base64.b64decode(encrypted) == fake_cipher

            # Mock subprocess for decryption
            with mock.patch("subprocess.run") as mock_subproc:
                mock_subproc.return_value = mock.Mock(stdout=b"plaintext")
                decrypted = temp.decrypt_passkey("RADIUS", base64.b64encode(fake_cipher).decode())
                assert decrypted == "plaintext"

    def test_encrypt_raises_if_no_password(self):
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(DEFAULT_JSON))), \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr()
            with pytest.raises(ValueError):
                temp.encrypt_passkey("RADIUS", "plaintext")

    def test_rotate_feature_passwd_with_new_password(self):
        cb_mock = mock.Mock()
        lookup = {"cb1": cb_mock}
        json_data = {
            "RADIUS": {"callbacks": ["cb1"], "password": "radius_secret"}
        }
        with mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(json_data))), \
             mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr(callback_lookup=lookup)
            with mock.patch.object(temp, "_save_registry") as mock_save:
                table_info = {"foo": "bar"}
                temp.rotate_feature_passwd("RADIUS", table_info, new_password="radius_secret2")
                cb_mock.assert_called_once_with(table_info)
                args = mock_save.call_args[0][0]
                assert args["RADIUS"]["password"] == "radius_secret2"
