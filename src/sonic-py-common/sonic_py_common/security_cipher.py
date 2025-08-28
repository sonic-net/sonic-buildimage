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
from swsscommon.swsscommon import ConfigDBConnector

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
            self._file_path = "/etc/cipher_pass"
            self._config_db = ConfigDBConnector()
            self._config_db.connect()
            # Note: Kept 1st index NA intentionally to map it with the cipher_pass file
            # contents. The file has a comment at the 1st row / line
            self._feature_list = ["NA", "TACPLUS", "RADIUS", "LDAP"]
            if not os.path.exists(self._file_path):
                with open(self._file_path, 'w') as file:
                    file.writelines("#Auto generated file for storing the encryption passwords\n")
                    for feature in self._feature_list[1:]:  # Skip the first "NA" entry
                        file.write(f"{feature} : \n")
                    os.chmod(self._file_path, 0o640)
            self._initialized = True

    # Write cipher_pass file
    def __write_passwd_file(self, feature_type, passwd):
        if feature_type == 'NA':
            syslog.syslog(syslog.LOG_ERR, "__write_passwd_file: Invalid feature type: {}".format(feature_type))
            return

        if feature_type in self._feature_list:
            try:
                with open(self._file_path, 'r') as file:
                    lines = file.readlines()
                    # Update the password for given feature
                    lines[self._feature_list.index(feature_type)] = feature_type + ' : ' + passwd + '\n'

                    os.chmod(self._file_path, 0o640)
                    with open(self._file_path, 'w') as file:
                        file.writelines(lines)
                    os.chmod(self._file_path, 0o640)
            except FileNotFoundError:
                syslog.syslog(syslog.LOG_ERR, "__write_passwd_file: File {} no found".format(self._file_path))
            except PermissionError:
                syslog.syslog(syslog.LOG_ERR, "__write_passwd_file: Read permission denied: {}".format(self._file_path))


    # Read cipher pass file and return the feature specifc
    # password
    def __read_passwd_file(self, feature_type):
        passwd = None
        if feature_type == 'NA':
            syslog.syslog(syslog.LOG_ERR, "__read_passwd_file: Invalid feature type: {}".format(feature_type))
            return passwd 

        if feature_type in self._feature_list:
           try:
               os.chmod(self._file_path, 0o644)
               with open(self._file_path, "r") as file:
                   lines = file.readlines()
                   for line in lines:
                       if feature_type in line:
                           passwd = line.split(' : ')[1]
               os.chmod(self._file_path, 0o640)
           except FileNotFoundError:
                syslog.syslog(syslog.LOG_ERR, "__read_passwd_file: File {} no found".format(self._file_path))
           except PermissionError:
                syslog.syslog(syslog.LOG_ERR, "__read_passwd_file: Read permission denied: {}".format(self._file_path))

        return passwd
    

    def encrypt_passkey(self, feature_type, secret: str, passwd: str) -> str:
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
            self.__write_passwd_file(feature_type, passwd)
            return b64_encoded 
        except subprocess.CalledProcessError as e:
            syslog.syslog(syslog.LOG_ERR, "encrypt_passkey: {} Encryption failed with ERR: {}".format((e)))
            return ""


    def decrypt_passkey(self, feature_type,  b64_encoded: str) -> str:
        """
        Decrypts a hex-encoded encrypted string using OpenSSL (AES-128-CBC, with salt and pbkdf2, no base64).
        Returns the decrypted plaintext.
        """

        passwd = self.__read_passwd_file(feature_type).strip()
        if passwd is None:
            syslog.syslog(syslog.LOG_ERR, "decrypt_passkey: Enpty password for {} feature type".format(feature_type))
            return ""

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


    def del_cipher_pass(self, feature_type):
        """
        Removes only the password for the given feature_type while keeping the file structure intact.
        """
        try:
            os.chmod(self._file_path, 0o640)
            with open(self._file_path, "r") as file:
                lines = file.readlines()

            updated_lines = []
            for line in lines:
                if line.strip().startswith(f"{feature_type} :"):
                    updated_lines.append(f"{feature_type} : \n")  # Remove password but keep format
                else:
                    updated_lines.append(line)

            with open(self._file_path, 'w') as file:
                file.writelines(updated_lines)
            os.chmod(self._file_path, 0o640)

            syslog.syslog(syslog.LOG_INFO, "del_cipher_pass: Password for {} has been removed".format((feature_type)))

        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "del_cipher_pass: {} Exception occurred: {}".format((e)))

