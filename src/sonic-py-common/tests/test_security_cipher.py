import sys
import json
import base64
import pytest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from sonic_py_common.security_cipher import master_key_mgr
from .mock_swsscommon import ConfigDBConnector

# TODO: Remove this if/else block once we no longer support Python 2
if sys.version_info.major == 3:
    BUILTINS = "builtins"
else:
    BUILTINS = "__builtin__"

DEFAULT_JSON = {
    "TACPLUS": {"table_info": [], "password": None},
    "RADIUS": {"table_info": [], "password": None},
    "LDAP": {"table_info": [], "password": None}
}

UPDATED_JSON = {
    "TACPLUS": {"table_info": [], "password": None},
    "RADIUS": {"table_info": [], "password": "TEST2"},
    "LDAP": {"table_info": [], "password": None}
}

class TestSecurityCipher(object):
    def setup_method(self):
        # Reset singleton for isolation
        master_key_mgr._instance = None

    def test_set_feature_password_sets_password(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(DEFAULT_JSON))), \
             mock.patch("os.path.exists", return_value=True):
            temp = master_key_mgr()
            # Patch _save_registry to check written value
            with mock.patch.object(temp, "_save_registry") as mock_save:
                temp.set_feature_password("RADIUS", "testpw")
                args = mock_save.call_args[0][0]
                assert args["RADIUS"]["password"] == "testpw"

    def test_set_feature_password_does_not_overwrite_existing(self):
        json_data = UPDATED_JSON.copy()
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(json_data))), \
             mock.patch("os.path.exists", return_value=True):
            temp = master_key_mgr()
            with mock.patch.object(temp, "_save_registry") as mock_save:
                temp.set_feature_password("RADIUS", "should_not_overwrite")
                mock_save.assert_not_called()

    def test_register_table_info(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(DEFAULT_JSON))), \
             mock.patch("os.path.exists", return_value=True):
            temp = master_key_mgr()
            with mock.patch.object(temp, "_save_registry") as mock_save:
                temp.register("RADIUS", "RADIUS|global")
                args = mock_save.call_args[0][0]
                assert "RADIUS|global" in args["RADIUS"]["table_info"]

    def test_deregister_table_info(self):
        # Use an in-memory registry that can be mutated
        registry = {
            "RADIUS": {"table_info": ["RADIUS|global", "RADIUS|backup"], "password": "radius_secret"}
        }
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
                mock.patch("os.chmod"), \
                mock.patch("{}.open".format(BUILTINS), mock.mock_open()), \
                mock.patch("os.path.exists", return_value=True):

            temp = master_key_mgr()
            # Patch _load_registry to always return current registry
            temp._load_registry = mock.Mock(side_effect=lambda: registry.copy())
            # Patch _save_registry to update our in-memory registry
            def save_registry(data):
                registry.clear()
                registry.update(json.loads(json.dumps(data)))  # Deep copy
            temp._save_registry = mock.Mock(side_effect=save_registry)

            temp.deregister("RADIUS", "RADIUS|global")
            assert registry["RADIUS"]["table_info"] == ["RADIUS|backup"]
            assert registry["RADIUS"]["password"] == "radius_secret"

            temp.deregister("RADIUS", "RADIUS|backup")
            assert registry["RADIUS"]["table_info"] == []
            assert registry["RADIUS"]["password"] is None

    def test_encrypt_and_decrypt_passkey(self):
        # Use a known password and mock openssl subprocess
        json_data = {
            "RADIUS": {"table_info": [], "password": "secretpw"}
        }
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("os.chmod"), \
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
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector), \
             mock.patch("os.chmod"), \
             mock.patch("{}.open".format(BUILTINS), mock.mock_open(read_data=json.dumps(DEFAULT_JSON))), \
             mock.patch("os.path.exists", return_value=True):
            temp = master_key_mgr()
            with pytest.raises(ValueError):
                temp.encrypt_passkey("RADIUS", "plaintext")

    def test_is_key_encrypt_enabled(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector):
            temp = master_key_mgr()
            temp._config_db.get_entry = mock.Mock()

            # Test when key_encrypt is 'True'
            temp._config_db.get_entry.return_value = {"key_encrypt": "True"}
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is True

            # Test when key_encrypt is 'true'
            temp._config_db.get_entry.return_value = {"key_encrypt": "true"}
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is True

            # Test when key_encrypt is 'False'
            temp._config_db.get_entry.return_value = {"key_encrypt": "False"}
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is False

            # Test when key_encrypt is missing
            temp._config_db.get_entry.return_value = {"foo": "bar"}
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is False

            # Test when entry is empty
            temp._config_db.get_entry.return_value = {}
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is False

            # Test when entry is None
            temp._config_db.get_entry.return_value = None
            assert temp.is_key_encrypt_enabled("TACPLUS", "global") is False
