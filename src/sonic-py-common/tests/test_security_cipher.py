from sonic_py_common.security_cipher import security_cipher
from .mock_swsscommon import ConfigDBConnector 


class TestSecurityCipher(object):
    def test_passkey_encryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector):
            temp = security_cipher()
            encrypt, err = temp.encrypt_passkey("TACPLUS", "passkey1", "TEST1")
            assert encrypt !=  "passkey1"
            assert err == None 

    def test_passkey_decryption(self):
        with mock.patch("sonic_py_common.security_cipher.ConfigDBConnector", new=ConfigDBConnector):
            temp = security_cipher()
            encrypt, err = temp.encrypt_passkey("RADIUS", "passkey2", "TEST2")
            assert err == None 
            decrypt, err = temp.decrypt_passkey("RADIUS", encrypt)
            assert err == None 
            assert decrypt == "passkey2"


