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
                    file.writelines("TACPLUS : \nRADIUS : \nLDAP :\n")
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

                    os.chmod(self._file_path, 0o777)
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
    
    # Encrypt the passkey
    def encrypt_passkey(self, feature_type, secret, passwd):
        cmd = [ 'openssl', 'enc', '-aes-128-cbc', '-A',  '-a', '-salt', '-pbkdf2', '-pass', 'pass:' + passwd ]
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        outsecret, errs = p.communicate(input=secret)
        if not errs:
            self.__write_passwd_file(feature_type, passwd)

        return outsecret,errs

    # Decrypt the passkey
    def decrypt_passkey(self, feature_type, secret):
        errs = "Passkey Decryption failed"
        passwd = self.__read_passwd_file(feature_type)
        if passwd is not None:
            cmd = "echo " + format(secret) + " | openssl enc -aes-128-cbc -a -d -salt -pbkdf2 -pass pass:" + passwd
            proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
            output, errs = proc.communicate()

            if not errs:
                output = output.decode('utf-8')

        return output, errs

    # Check if the encryption is enabled 
    def is_key_encrypt_enabled(self, table, entry):
        key = 'key_encrypt'
        data = self._config_db.get_entry(table, entry)
        if data:
            if key in data:
                return data[key]
        return False

    def del_cipher_pass(self):
        try:
            # Check if the file exists
            if os.path.exists(self._file_path):
                # Attempt to delete the file
                os.remove(self._file_path)
                syslog.syslog(syslog.LOG_INFO, "del_cipher_pass: {} file has been removed".format((self._file_path)))
            else:
                syslog.syslog(syslog.LOG_INFO, "del_cipher_pass: {} file doesn't exist".format((self._file_path)))
        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "del_cipher_pass: {} Exception occurred: {}".format((e)))

