#!/usr/bin/env python3

import os
import re
from collections import defaultdict
from datetime import datetime, date, timedelta

from sonic_py_common.logger import Logger

SYSLOG_IDENTIFIER = 'core_cleanup.py'
CORE_FILE_DIR = '/var/core/'
KERNEL_DUMP_DIR = '/var/dump/'
MAX_CORE_FILES = 4

def delete_file(file_path):
    try:
        os.remove(file_path)
    except e:
        logger.log_error('Unexpected error: {} occured trying to delete {}'.format(e, file_path))

def get_dump_timestamp(file_name):
    match = re.search(r'sonic_dump_.*_(\d\d\d\d)(\d\d)(\d\d)_(\d\d)(\d\d)(\d\d).tar.gz', file_name)
    if match:
        groups = match.groups()
        return datetime(int(groups[0]), int(groups[1]), int(groups[2]), int(groups[3]), int(groups[4]), int(groups[5]))
    
    return None

def main():
    logger = Logger(SYSLOG_IDENTIFIER)
    logger.set_min_log_priority_info()

    if os.getuid() != 0:
        logger.log_error('Root required to clean up core files')
        return

    logger.log_info('Cleaning up core files')
    core_files = [f for f in os.listdir(CORE_FILE_DIR) if os.path.isfile(os.path.join(CORE_FILE_DIR, f))]

    core_files_by_process = defaultdict(list)
    for f in core_files:
        process = f.split('.')[0]
        curr_files = core_files_by_process[process]
        curr_files.append(f)

        if len(curr_files) > MAX_CORE_FILES:
            curr_files.sort(reverse=True, key=lambda x: datetime.utcfromtimestamp(int(x.split('.')[1])))
            oldest_core = curr_files[MAX_CORE_FILES]
            logger.log_info('Deleting {}'.format(oldest_core))
            delete_file(os.path.join(CORE_FILE_DIR, oldest_core))
            core_files_by_process[process] = curr_files[0:MAX_CORE_FILES]

    logger.log_info('Finished cleaning up core files')

    # cleanup kernel dumps
    logger.log_info('Cleaning up kernel dump files')
    kernel_dumps = [f for f in os.listdir(KERNEL_DUMP_DIR) if os.path.isfile(os.path.join(KERNEL_DUMP_DIR, f))]
    # Sort kernel dumps from new to old
    kernel_dumps.sort(reverse = True, key = lambda x: get_dump_timestamp(x))
    not_expired_dumps = []
    for kernel_dump in kernel_dumps:
        # delete expired kernel dump
        dump_date = get_dump_timestamp(kernel_dump)
        if not dump_date:
            # Not a kernel dump file
            continue

        # Only keep recent MAX_CORE_FILES kernel dumps
        if len(not_expired_dumps) < MAX_CORE_FILES:
            not_expired_dumps.append(kernel_dump)
        else:
            logger.log_info('Deleting {}'.format(kernel_dump))
            delete_file(os.path.join(KERNEL_DUMP_DIR, kernel_dump))

    logger.log_info('Finished cleaning up kernel dump files')

if __name__ == '__main__':
    main()
