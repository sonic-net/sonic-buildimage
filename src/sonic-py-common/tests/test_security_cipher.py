from sonic_py_common.secure_cipher import secure_cipher

temp = security_cipher()

class TestSecurityCipher(object):
    def test_passkey_encryption(self):
        encrypt, err = temp.encrypt_passkey("TACPLUS", "passkey1", "TEST1")
        assert encrypt !=  "passkey1"
        assert err == None 

    def test_passkey_decryption(self):
        encrypt, err = temp.encrypt_passkey("RADIUS", "passkey2", "TEST2")
        assert err == None 
        decrypt, err = temp.decrypt_passkey("RADIUS", encrypt)
        assert err == None 
        assert decrypt == "passkey2"


