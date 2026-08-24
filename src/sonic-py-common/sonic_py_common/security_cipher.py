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

    def __new__(cls):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(master_key_mgr, cls).__new__(cls)
                cls._instance._initialized = False
        return cls._instance

    def __init__(self):
        if not self._initialized:
            self._file_path = CIPHER_PASS_FILE
            self._config_db = ConfigDBConnector()
            self._config_db.connect()
            self._initialized = True

    def _load_registry(self):
        """
        Read cipher_pass.json file
        """
        if not os.path.exists(CIPHER_PASS_FILE):
            return {}
        try:
            with open(CIPHER_PASS_FILE, 'r') as f:
                return json.load(f)

        except json.JSONDecodeError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_load_registry: Invalid JSON in {}: {}".format(CIPHER_PASS_FILE, e))
            return {}
        except PermissionError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_load_registry: Permission denied reading {}: {}".format(CIPHER_PASS_FILE, e))
            return {}
        except OSError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_load_registry: OS error reading {}: {}".format(CIPHER_PASS_FILE, e))
            return {}

    def _save_registry(self, data):
        """
        Write cipher_pass.json file
        """
        try:
            with open(CIPHER_PASS_FILE, 'w') as f:
                json.dump(data, f, indent=2)
            os.chmod(self._file_path, 0o600)

        except PermissionError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_save_registry: Permission denied writing {}: {}".format(self._file_path, e))
        except OSError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_save_registry: OS error writing {}: {}".format(self._file_path, e))
        except TypeError as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "_save_registry: Invalid data format, not JSON serializable: {}".format(e))

    def _encrypt_passkey(self, feature_type, secret: str, passwd: str) -> str:
        """
        Encrypts the plaintext using OpenSSL (AES-128-CBC, with salt and pbkdf2, no base64)
        and returns the result as a hex string.
        """
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
            syslog.syslog(syslog.LOG_ERR, "_encrypt_passkey: {} Encryption failed with ERR: {}".format(e))
            return ""

    def _decrypt_passkey(self, feature_type,  b64_encoded: str, passwd: str) -> str:
        """
        Decrypts a hex-encoded encrypted string using OpenSSL (AES-128-CBC, with salt and pbkdf2, no base64).
        Returns the decrypted plaintext.
        """
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

    def register(self, feature_type, table_info):
        """
        Register a table_info for a feature type.
        Feature types: TACPLUS, RADIUS, LDAP etc.
        """
        data = self._load_registry()
        if feature_type not in data:
            data[feature_type] = {"table_info": [], "password": None}
        if table_info not in data[feature_type]["table_info"]:
            data[feature_type]["table_info"].append(table_info)
        self._save_registry(data)
        syslog.syslog(syslog.LOG_INFO, "register: table_info {} attached to {} feature".format(table_info, feature_type))

    def deregister(self, feature_type, table_info):
        """
        Deregister (remove) a table_info string (like "TACPLUS|global") for a feature type.
        If, after removal, there are no more table_info entries for that feature,
        remove the respective password as well.
        """
        data = self._load_registry()
        if feature_type in data:
            if table_info in data[feature_type]["table_info"]:
                data[feature_type]["table_info"].remove(table_info)
                if not data[feature_type]["table_info"]:
                    # No more table_info left; remove password as well
                    data[feature_type]["password"] = None
                    syslog.syslog(syslog.LOG_INFO, "deregister: No more table_info for feature {}. Password also removed.".format(feature_type))
                self._save_registry(data)
                syslog.syslog(syslog.LOG_INFO, "deregister: table_info {} removed from feature {}".format(table_info, feature_type))
            else:
                syslog.syslog(syslog.LOG_ERR, "deregister: table_info {} not found for feature {}".format(table_info, feature_type))
        else:
            syslog.syslog(syslog.LOG_ERR, "deregister: No table_info registered for {}".format(feature_type))

    def set_feature_password(self, feature_type, password):
        """
        Set a new password for a feature type.
        It will not update if already exist.
        """
        data = self._load_registry()
        if feature_type not in data:
            data[feature_type] = {"table_info": [], "password": None}
        if data[feature_type]["password"] is not None:
            syslog.syslog(syslog.LOG_INFO, "set_feature_password: Password already set for feature {}, not updating the new password.".format(feature_type))
            syslog.syslog(syslog.LOG_INFO, "set_feature_password: Note: Make use of rotate_feature_passwd() method for updating the existing pass")
            return
        data[feature_type]["password"] = password
        self._save_registry(data)
        syslog.syslog(syslog.LOG_INFO, "set_feature_password: Password set for feature {}".format(feature_type))

    def rotate_feature_passwd(self, feature_type, new_password):
        """
        For each registered table_info, extract encrypted passkey, decrypt, re-encrypt with new password, and update.
        """
        data = self._load_registry()
        if feature_type not in data:
            syslog.syslog(syslog.LOG_ERR, "No table_info registered for {} Feature".format(feature_type))
            return

        old_password = data[feature_type]["password"]
        table_infos = data[feature_type].get("table_info", [])
        for table_info in table_infos:
            table, entry = table_info.split("|")
            db_entry = self._config_db.get_entry(table, entry)
            encrypted_passkey = db_entry.get("passkey")
            #Rotate only if valid passkey is present and 'key_encrypt' flag is True
            if encrypted_passkey and str(db_entry.get("key_encrypt")).lower() == 'true':
                # Decrypt with old password
                plain_passkey = self._decrypt_passkey(feature_type, encrypted_passkey, old_password)
                # Re-encrypt with new password
                new_encrypted_passkey = self._encrypt_passkey(feature_type, plain_passkey, new_password)
                # Update DB
                db_entry["passkey"] = new_encrypted_passkey
                self._config_db.set_entry(table, entry, db_entry)
                syslog.syslog(syslog.LOG_INFO, "rotate_feature_passwd: Updated passkey for {}".format(table_info))
            else:
                syslog.syslog(syslog.LOG_WARNING, "Either no passkey found or key_encrypt flag is not set to True for {}".format(table_info))

        # Update stored password
        data[feature_type]["password"] = new_password
        self._save_registry(data)
        syslog.syslog(syslog.LOG_INFO, "rotate_feature_passwd: Password for {} Feature has been updated.".format(feature_type))

    def encrypt_passkey(self, feature_type, secret: str) -> str:
        """
        Encrypts the plaintext and returns the result as a hex string.
        """
        # Retrieve password from cipher_pass registry
        data = self._load_registry()
        passwd = None
        if feature_type in data:
            passwd = data[feature_type].get("password")
        if not passwd:
            raise ValueError(f"encrypt_passkey: No password set for feature {feature_type}")

        return self._encrypt_passkey(feature_type, secret, passwd)

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
            raise ValueError(f"decrypt_passkey: No password set for feature {feature_type}")

        return self._decrypt_passkey(feature_type, b64_encoded, passwd)

    # Check if the encryption is enabled
    def is_key_encrypt_enabled(self, table, entry):
        data = self._config_db.get_entry(table, entry)
        if data and 'key_encrypt' in data:
            return data['key_encrypt'].lower() == 'true'
        return False

