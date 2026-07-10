#!/usr/bin/env python3

import hashlib
import os
import syslog

SYSLOG_IDENTIFIER = 'asic_config_checksum'

CHUNK_SIZE = 8192

CONFIG_FILES = {
    os.path.abspath('./src/sonic-swss/swssconfig/sample/'): ['netbouncer.json']
}

OUTPUT_FILE = os.path.abspath('./asic_config_checksum')


def log_info(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()


def log_error(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()


def get_config_files(config_file_map):
    '''
    Generates a list of absolute paths to ASIC config files.
    '''
    config_files = []
    for path, files in config_file_map.items():
        for config_file in files:
            config_files.append(os.path.join(path, config_file))
    return config_files


def generate_checksum(checksum_files):
    '''
    Generates a checksum for a given list of files. Returns None if an error
    occurs while reading the files.

    NOTE: The checksum is performed in the order provided. This function does 
    NOT do any re-ordering of the files before creating the checksum.
    '''
    checksum = hashlib.sha1()
    for checksum_file in checksum_files:
        try:
            with open(checksum_file, 'rb') as f:
                for chunk in iter(lambda: f.read(CHUNK_SIZE), b""):
                    checksum.update(chunk)
        except IOError as e:
            log_error('Error processing ASIC config file ' + checksum_file + ':' + e.strerror)
            return None

    return checksum.hexdigest()


def main():
    config_files = sorted(get_config_files(CONFIG_FILES))
    checksum = generate_checksum(config_files)
    if checksum is None:
        exit(1)

    with open(OUTPUT_FILE, 'w') as output:
        output.write(checksum + '\n')


if __name__ == '__main__':
    main()
