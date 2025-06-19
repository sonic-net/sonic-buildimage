'''

A common module for handling the encryption and
decryption of the feature passkey. It also takes
care of storing the secure cipher at root
protected file system

'''

import subprocess
import threading
import syslog
import os
import base64
import json
from swsscommon.swsscommon import ConfigDBConnector

CIPHER_PASS_FILE = "/etc/cipher_pass.json"

class master_key_mgr:
    _instance = None
    _lock = threading.Lock()
    _initialized = False

    def __new__(cls, callback_lookup=None):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(master_key_mgr, cls).__new__(cls)
                cls._instance._initialized = False
        return cls._instance

    def __init__(self, callback_lookup=None):
        if not self._initialized:
            self._file_path = CIPHER_PASS_FILE
            self._config_db = ConfigDBConnector()
            self._config_db.connect()
            if callback_lookup is None:
                callback_lookup = {}
            self.callback_lookup = callback_lookup
            self._initialized = True

    def _load_registry(self):
        if not os.path.exists(CIPHER_PASS_FILE):
            return {}
        try:
            with open(CIPHER_PASS_FILE, 'r') as f:
                return json.load(f)
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "del_cipher_passwd: Exception occurred: {}".format(e))
            return {}

    def _save_registry(self, data):
        with open(CIPHER_PASS_FILE, 'w') as f:
            json.dump(data, f, indent=2)
        os.chmod(self._file_path, 0o640)

    def register(self, feature_type, callback):
        """
        Register a callback for a feature type.
        """
        data = self._load_registry()
        if feature_type not in data:
            data[feature_type] = {"callbacks": [], "password": None}
        cb_name = callback.__name__
        if cb_name not in data[feature_type]["callbacks"]:
            data[feature_type]["callbacks"].append(cb_name)
        self._save_registry(data)
        syslog.syslog(syslog.LOG_INFO, "register: Callback {} attached to feature {}".format(cb_name, feature_type))

    def deregister(self, feature_type, callback):
        """
        Deregister (remove) a callback for a feature type.
        If, after removal, there are no more callbacks for that feature,
        that means there is no one who is going to use the password thus
        remove the respective password too.
        """
        data = self._load_registry()
        if feature_type in data:
            cb_name = callback.__name__
            if cb_name in data[feature_type]["callbacks"]:
                data[feature_type]["callbacks"].remove(cb_name)
                if not data[feature_type]["callbacks"]:
                    # No more callbacks left; remove password as well
                    data[feature_type]["password"] = None
                    syslog.syslog(syslog.LOG_INFO, "deregister: No more callbacks for feature {}. Password also removed.".format(feature_type))
                self._save_registry(data)
                syslog.syslog(syslog.LOG_INFO, "deregister: Callback {} removed from feature {}".format(cb_name, feature_type))
                
            else:
                syslog.syslog(syslog.LOG_ERR, "deregister: Callback {} not found for feature {}".format(cb_name, feature_type))
        else:
            syslog.syslog(syslog.LOG_ERR, "deregister: No callbacks registered for {}".format(feature_type))

    def set_feature_password(self, feature_type, password):
        """
        Set a new password for a feature type.
        It will not update if already exist.
        """
        data = self._load_registry()
        if feature_type not in data:
            data[feature_type] = {"callbacks": [], "password": None}
        if data[feature_type]["password"] is not None:
            syslog.syslog(syslog.LOG_INFO, "set_feature_password: Password already set for feature {}, not updating the new password.".format(feature_type))
            return
        data[feature_type]["password"] = password
        self._save_registry(data)
        syslog.syslog(syslog.LOG_INFO, "set_feature_password: Password set for feature {}".format(feature_type))

    def rotate_feature_passwd(self, feature_type, new_password=None):
        """
        On each call, read JSON data fresh from disk. Update password if provided,
        and call all registered callbacks with the latest password.
        """
        data = self._load_registry()
        if feature_type not in data:
            syslog.syslog(syslog.LOG_ERR, "rotate_feature_passwd: No callbacks registered for {}".format(feature_type))
            return

        if new_password is not None:
            data[feature_type]["password"] = new_password
            self._save_registry(data)
            syslog.syslog(syslog.LOG_INFO, "rotate_feature_passwd: Password for {} updated during rotation.".format(feature_type))

        password = data[feature_type].get("password")
        cb_names = data[feature_type].get("callbacks", [])
        callbacks = [self.callback_lookup[name] for name in cb_names if name in self.callback_lookup]

        if not callbacks:
            syslog.syslog(syslog.LOG_ERR, "rotate_feature_passwd: No callbacks registered for {}".format(feature_type))
            return

        syslog.syslog(syslog.LOG_INFO, "rotate_feature_passwd: Rotating password for feature {} and notifying callbacks...".format(feature_type))
        for cb in callbacks:
            cb(password)

    def encrypt_passkey(self, feature_type, secret: str) -> str:
        """
        Encrypts the plaintext using OpenSSL (AES-128-CBC, with salt and pbkdf2, no base64)
        and returns the result as a hex string.
        """
        # Retrieve password from cipher_pass registry
        data = self._load_registry()
        passwd = None
        if feature_type in data:
            passwd = data[feature_type].get("password")
        if not passwd:
            raise ValueError(f"No password set for feature {feature_type}")

        cmd = [
            "openssl", "enc", "-aes-128-cbc", "-salt", "-pbkdf2",
            "-pass", f"pass:{passwd}"
        ]
        try:
            result = subprocess.run(
                cmd,
                input=secret.encode(),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True
            )
            encrypted_bytes = result.stdout
            b64_encoded = base64.b64encode(encrypted_bytes).decode()
            return b64_encoded
        except subprocess.CalledProcessError as e:
            syslog.syslog(syslog.LOG_ERR, "encrypt_passkey: {} Encryption failed with ERR: {}".format((e)))
            return ""

    def decrypt_passkey(self, feature_type,  b64_encoded: str) -> str:
        """
        Decrypts a hex-encoded encrypted string using OpenSSL (AES-128-CBC, with salt and pbkdf2, no base64).
        Returns the decrypted plaintext.
        """
        # Retrieve password from cipher_pass registry
        data = self._load_registry()
        passwd = None
        if feature_type in data:
            passwd = data[feature_type].get("password")
        if not passwd:
            raise ValueError(f"No password set for feature {feature_type}")

        try:
            encrypted_bytes = base64.b64decode(b64_encoded)

            cmd = [
                "openssl", "enc", "-aes-128-cbc", "-d", "-salt", "-pbkdf2",
                "-pass", f"pass:{passwd}"
            ]
            result = subprocess.run(
                cmd,
                input=encrypted_bytes,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True
            )
            return result.stdout.decode().strip()
        except subprocess.CalledProcessError as e:
            syslog.syslog(syslog.LOG_ERR, "decrypt_passkey: Decryption failed with an ERR: {}".format(e.stderr.decode()))
            return ""

    # Check if the encryption is enabled
    def is_key_encrypt_enabled(self, table, entry):
        key = 'key_encrypt'
        data = self._config_db.get_entry(table, entry)
        if data:
            if key in data:
                return data[key]
        return False

