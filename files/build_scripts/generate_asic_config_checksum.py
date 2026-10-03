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
SHA256_OUTPUT_FILE = os.path.abspath('./asic_config_checksum.sha256')


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


def generate_checksums(checksum_files):
    '''
    Generates legacy SHA-1 and SHA-256 checksums for a given list of files.
    Returns None if an error
    occurs while reading the files.

    NOTE: The checksums are performed in the order provided. This function does
    NOT do any re-ordering of the files before creating the checksum.
    '''
    # Existing images compare this value during fast reboot, so retain it until
    # all supported images can read the SHA-256 sidecar.
    # nosemgrep: python.lang.security.insecure-hash-algorithms.insecure-hash-algorithm-sha1
    legacy_checksum = hashlib.sha1()  # lgtm[py/weak-hashes]
    sha256_checksum = hashlib.sha256()
    for checksum_file in checksum_files:
        try:
            with open(checksum_file, 'rb') as f:
                for chunk in iter(lambda: f.read(CHUNK_SIZE), b""):
                    legacy_checksum.update(chunk)
                    sha256_checksum.update(chunk)
        except IOError as e:
            log_error('Error processing ASIC config file ' + checksum_file + ':' + e.strerror)
            return None

    return legacy_checksum.hexdigest(), sha256_checksum.hexdigest()


def generate_checksum(checksum_files):
    '''
    Generates the SHA-256 checksum for callers that only need the strong digest.
    '''
    checksums = generate_checksums(checksum_files)
    return checksums[1] if checksums is not None else None


def main():
    config_files = sorted(get_config_files(CONFIG_FILES))
    checksums = generate_checksums(config_files)
    if checksums is None:
        exit(1)

    legacy_checksum, sha256_checksum = checksums
    with open(OUTPUT_FILE, 'w') as output:
        output.write(legacy_checksum + '\n')
    with open(SHA256_OUTPUT_FILE, 'w') as output:
        output.write(sha256_checksum + '\n')


if __name__ == '__main__':
    main()
